#include "ModManager.h"

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)

    #include <algorithm>
    #include <cmath>
    #include <filesystem>
    #include <functional>
    #include <memory>
    #include <ranges>
    #include <stdexcept>
    #include <string_view>
    #include <vector>

    #include <lua.h>
    #include <quill/Backend.h>
    #include <quill/Frontend.h>
    #include <quill/LogMacros.h>
    #include <sol/environment.hpp>
    #include <sol/forward.hpp>
    #include <sol/object.hpp>
    #include <sol/optional_implementation.hpp>
    #include <sol/overload.hpp>
    #include <sol/property.hpp>
    #include <sol/resolve.hpp>
    #include <sol/state_view.hpp>
    #include <sol/types.hpp>
    #include <yaml-cpp/yaml.h>

    #include "Method.h"
    #include "NetManager.h"
    #include "Peer.h"
    #include "RouteManager.h"
    #include "Types.h"
    #include "ValhallaServer.h"
    #include "VUtilsResource.h"

auto SCRIPT_MANAGER(std::make_unique<IScriptManager>());

IScriptManager *ScriptManager()
{
    return SCRIPT_MANAGER.get();
}

std::tuple<IScriptManager::ScriptInfo, std::string> IScriptManager::load_file_script(fs::path script_root)
{
    YAML::Node loadNode;

    auto script_info_path = script_root / "scriptInfo.yml";

    if (auto opt = VUtils::Resource::ReadFile<std::string>(script_info_path)) {
        loadNode = YAML::Load(opt.value());
    } else {
        throw std::runtime_error("unable to open " + script_info_path.string());
    }

    auto raw_entry = loadNode["entry"].as<std::string>();
    if (!raw_entry.ends_with(".lua"))
        raw_entry += ".lua";

    auto name = loadNode["name"].as<std::string>();

    auto entry_path = script_root / raw_entry;
    if (!fs::exists(entry_path)) {
        throw std::runtime_error("script entry file not found, skipping...");
    }

    auto code_opt = VUtils::Resource::ReadFile<std::string>(entry_path);

    //if (!code_opt)
    //    throw std::runtime_error("file not found");

    //auto &&insert = this->m_mods.insert({name, std::make_unique<ModInfo>(name, entry_path)});

    //if (!insert.second)
    //throw std::runtime_error("Mod " + name + " already loaded");

    //auto &&mod = insert.first->second;

    ScriptInfo script_info(std::move(name), std::move(entry_path));

    script_info.m_version     = loadNode["version"].as<std::string>("");
    script_info.m_apiVersion  = loadNode["api-version"].as<std::string>("");
    script_info.m_description = loadNode["description"].as<std::string>("");
    script_info.m_authors     = loadNode["authors"].as<avledet::util::Strings>(avledet::util::Strings());

    return {script_info, code_opt.value()};
}

// Unused for now, because ...?
//int LoadFileRequire(lua_State* L) {
//    std::string path = sol::stack::get<std::string>(L);
//
//    // first look in the sub mod dir
//    //  ./mods/MyExampleMod/
//    if (auto opt = VUtils::Resource::ReadFile<std::string>("./mods/" + path + ".lua"))
//        luaL_loadbuffer(L, opt.value().data(), opt.value().size(), path.c_str());
//    else {
//        sol::stack::push(L, "Module '" + path + "' not found");
//    }
//
//    return 1;
//}

void IScriptManager::execute(ScriptInfo const &info, std::string const &code)
{
    //m_scripts[info.m_name] = std::make_unique<ScriptInfo>(info);
    auto &&try_emplace  = m_scripts.try_emplace(info.m_name, std::make_unique<ScriptInfo>(std::move(info)));
    auto &&_plugin_info = *try_emplace.first->second;
    if (!try_emplace.second)
        throw std::runtime_error("tried loading plugin twice! " + _plugin_info.m_name);

    //LOG_INFO(AVL_LOGGER, "Running script {} / {}", info.m_name, info.m_authors);

    auto env = this->create_sandbox();

    env["this"] = std::ref(_plugin_info);// copy

    // Important: loadmode::text
    //  Otherwise, loading raw binary Lua can cause sandbox escapes according to <>
    m_state.script(code, env, info.m_chunk_name, sol::load_mode::text);
}

//TODO
inline void my_panic(sol::optional<std::string> maybe_msg)
{
    LOG_ERROR(AVL_LOGGER, "Lua is in a panic state and will now abort() the application");
    if (maybe_msg) {
        std::string const &msg = maybe_msg.value();
        LOG_ERROR(AVL_LOGGER, "\terror message: {}", msg);
    }
    // When this function exits, Lua will exhibit default behavior and abort(), unless I throw...
    throw std::runtime_error("plugin errored during load");
}

//in my limited usage and experience, no error handler or panic was ever invoked, perhaps because all
//  errors took place INSIDE one of my event handlers...
int my_exception_handler(lua_State *L, sol::optional<std::exception const &> maybe_exception,
                         sol::string_view description)
{
    // L is the lua state, which you can wrap in a state_view if necessary
    // maybe_exception will contain exception, if it exists
    // description will either be the what() of the exception or a description saying that we hit the general-case catch(...)
    LOG_ERROR(AVL_LOGGER, "An exception occurred in a function, here's what it says ");
    if (maybe_exception) {
        LOG_ERROR(AVL_LOGGER, "(straight from the exception): ");
        LOG_ERROR(AVL_LOGGER, "{}", maybe_exception->what());
    } else {
        LOG_ERROR(AVL_LOGGER, "(from the description parameter): ");
        LOG_ERROR(AVL_LOGGER, "{}", description);
    }

    // you must push 1 element onto the stack to be
    // transported through as the error object in Lua
    // note that Lua -- and 99.5% of all Lua users and libraries -- expects a string
    // so we push a single string (in our case, the description of the error)
    return sol::stack::push(L, description);
}

void IScriptManager::PostInit()
{
    LOG_NOTICE(AVL_LOGGER, "Initializing ModManager");

    m_state.set_panic(
            sol::c_call<decltype(&my_panic), &my_panic>);// important to set; otherwise lua will break things
    //m_state.set_exception_handler(&my_exception_handler); // triggers on EVERY exception, which I dont want yet

    // open all, we'll worry about sandboxing later
    //m_state.open_libraries();

    m_state.open_libraries(sol::lib::base,
                           //sol::lib::package, //unsafe; overridden
                           sol::lib::coroutine, sol::lib::string,
                           sol::lib::os,//shell exec; unsafe
                           sol::lib::math, sol::lib::table,
                           //sol::lib::debug, //unsafe, according to lua-users sandboxes
                           //sol::lib::bit32, //deprecated
                           //sol::lib::io, //unpermissive file reading; unsafe; overridden
                           //sol::lib::ffi, //luajit; unsafe;
                           //sol::lib::jit, //luajit; unsafe;
                           sol::lib::utf8);

    // Load globally shared userdata
    load_userdata();

    std::error_code ec;
    fs::create_directories(AVLEDET_SCRIPTS_PATH, ec);

    if (ec)
        return;

    auto sorted = fs::directory_iterator(AVLEDET_SCRIPTS_PATH, ec)
                  | std::views::filter([](fs::directory_entry e) -> bool {
                        return e.is_directory()
                               && !std::string_view(e.path().filename().c_str()).starts_with("--");
                    })
                  | std::ranges::to<std::vector>();
    std::ranges::sort(sorted);

    for (auto const &dir : sorted) {
        try {

            //auto&& absolute = fs::absolute(dir.path());
            //dir.path().
            //auto &&dirname = dir.path().filename().string();

            //auto [info, code] = load_file_script(dirname);
            auto [info, code] = load_file_script(dir.path());
            execute(info, code);

            LOG_NOTICE(AVL_LOGGER, "Loaded script '{}'", info.m_name);
        } catch (std::exception const &e) {
            LOG_ERROR(AVL_LOGGER, "Failed to load script: {}, {}", dir.path().string(), e.what());
        }
    }

    LOG_NOTICE(AVL_LOGGER, "Loaded {} scripts", m_scripts.size());

    AVL_SCRIPT_EVENT(IScriptManager::Events::Enable);
}

void IScriptManager::Uninit()
{
    AVL_SCRIPT_EVENT(IScriptManager::Events::Disable);
    m_callbacks.clear();
    m_scripts.clear();
}

//https://github.com/ThePhD/sol2/issues/980
// to 'reload' a script
//  clear / kill all references to scripts, by manually clearing out listeners / callbacks...

// how to handle script behavior on a reload?
//  a reload is handled not on server start, so some callbacks will never post
//  so rather, run a on_reload() callback that can handle mid-server operations
//  or other way to detect that this script has just been loaded midway through during server operations

void IScriptManager::update()
{
    ////if (!m_tmp_reload_mods.empty()) {
    ////    assert(false);//TODO

    ////    /*
    ////        Release all associated callbacks
    ////    */

    ////    for (auto &&itr = m_callbacks.begin(); itr != m_callbacks.end();) {
    ////        auto &&callbacks = itr->second;
    ////        for (auto &&itr1 = callbacks.begin(); itr1 != callbacks.end();) {
    ////            auto &&env = sol::get_environment(itr1->m_func);
    ////            //if (itr1->m_func.e.get() == m_tmp_mod_reload) {
    ////            assert(env.valid());

    ////            assert(env["this"].is<ScriptInfo *>());

    ////            auto mod = env["this"].get<ScriptInfo *>();

    ////            bool contains = m_tmp_reload_mods.contains(mod);

    ////            if (contains) {
    ////                itr1 = callbacks.erase(itr1);
    ////            } else {
    ////                ++itr1;
    ////            }
    ////        }

    ////        // Pop callback set for tidy
    ////        if (callbacks.empty()) {
    ////            itr = m_callbacks.erase(itr);
    ////        } else {
    ////            ++itr;
    ////        }
    ////    }

    ////    /*
    ////        Release all registered RPCs
    ////    */

    ////    //for (auto &&peer_pair : NetManager()->m_connectedPeers) {
    ////    //    for (auto &&method_itr = peer_pair->m_methods.begin();
    ////    //         method_itr != peer_pair->m_methods.end();) {
    ////    //        auto &&method = dynamic_cast<MethodImplLua<Peer *> *>(method_itr->second.get());

    ////    //        if (!method) {
    ////    //            ++method_itr;
    ////    //            continue;
    ////    //        }

    ////    //        auto &&env = sol::get_environment(method->m_func);
    ////    //        assert(env.valid());
    ////    //        assert(env["this"].is<Mod *>());
    ////    //        auto mod = env["this"].get<Mod *>();

    ////    //        bool contains = m_tmp_reload_mods.contains(mod);

    ////    //        if (contains) {
    ////    //            // kill it
    ////    //            method_itr = peer_pair->m_methods.erase(method_itr);
    ////    //        } else {
    ////    //            ++method_itr;
    ////    //        }
    ////    //    }
    ////    //}

    ////    //TODO rethink how everything is shaped...

    ////    // Perhaps start on reworking Valhalla,

    ////    // Migrating towards unit tests like in avl, and avoid
    ////    //  repeated pitfalls as before with debug hell...

    ////    // Clang tidy / formatters to look at,
    ////    //  refactor as a whole...

    ////    // Also unload routed rpcs...
    ////    //for (auto&& mod : m_tmp_reload_mods) {
    ////    //    mod->
    ////    //}

    ////    // https://github.com/ricosolana/Valhalla/blob/0121b3db3788c146fc0eda783c3563cb16ef2ca9/src/ModManager.cpp
    ////    // :::::::::::::::::::OLD::::::::::::::::;
    ////    //for (auto&& pair : m_mods) {
    ////    //    auto&& mod = *pair.second.get();
    ////    //    if (mod.m_reload) {
    ////    //        LOG(INFO) << "Reloading mod " << mod.m_name;

    ////    //        for (auto&& pair : NetManager()->GetPeers()) {
    ////    //            auto&& peer = pair.second;
    ////    //            for (auto&& pair1 : peer->m_methods) {
    ////    //                auto&& method = dynamic_cast<MethodImplLua<Peer*>*>(pair1.second.get());
    ////    //                //if (method)
    ////    //                    //method->m_func =
    ////    //            }
    ////    //            //if (auto method = peer->GetMethod()
    ////    //        }

    ////    //        mod.m_env.reset();
    ////    //        LoadMod(mod);
    ////    //        mod.m_reload = false;
    ////    //    }
    ////    //}

    ////    //m_state.collect_gc();

    ////    //m_tmp_mod_reload = nullptr;
    ////}
}

void IScriptManager::unload_script(ScriptInfo &script_info)
{
    assert(false);//MUST TEST
    /*
        Release all associated callbacks
    */
    ////for (auto &&itr = m_callbacks.begin(); itr != m_callbacks.end();) {
    ////    auto &&callbacks = itr->second;
    ////    for (auto &&itr1 = callbacks.begin(); itr1 != callbacks.end();) {
    ////        auto &&env = sol::get_environment(itr1->m_func);
    ////        //if (itr1->m_func.e.get() == m_tmp_mod_reload) {
    ////        assert(env.valid());

    ////        assert(env["this"].is<ScriptInfo *>());

    ////        auto on_mod = env["this"].get<ScriptInfo *>();

    ////        //bool contains = m_tmp_reload_mods.contains(mod);
    ////        bool contains = &mod == on_mod;

    ////        if (contains) {
    ////            itr1 = callbacks.erase(itr1);
    ////        } else {
    ////            ++itr1;
    ////        }
    ////    }

    ////    // Pop callback set for tidy
    ////    if (callbacks.empty()) {
    ////        itr = m_callbacks.erase(itr);
    ////    } else {
    ////        ++itr;
    ////    }
    ////}

    /*
        Release all registered RPCs        
    */

    //for (auto &&peer_pair : NetManager()->m_connectedPeers) {
    //    for (auto &&method_itr = peer_pair->m_methods.begin(); method_itr != peer_pair->m_methods.end();) {
    //        auto &&method = dynamic_cast<MethodImplLua<Peer *> *>(method_itr->second.get());

    //        if (!method) {
    //            ++method_itr;
    //            continue;
    //        }

    //        auto &&env = sol::get_environment(method->m_func);
    //        assert(env.valid());
    //        assert(env["this"].is<Mod *>());
    //        auto on_mod = env["this"].get<Mod *>();

    //        //bool contains = m_tmp_reload_mods.contains(mod);
    //        bool contains = &mod == on_mod;

    //        if (contains) {
    //            // kill it
    //            method_itr = peer_pair->m_methods.erase(method_itr);
    //        } else {
    //            ++method_itr;
    //        }
    //    }
    //}
}

#endif// AVL_ENABLE_SCRIPTING