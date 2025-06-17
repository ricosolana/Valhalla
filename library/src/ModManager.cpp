#include "ModManager.h"
#include <quill/LogMacros.h>

#if VH_IS_ON(VH_USE_MODS)

    #include <algorithm>
    #include <cmath>
    #include <filesystem>
    #include <ranges>
    #include <string_view>
    #include <vector>

    #include <quill/Backend.h>
    #include <quill/Frontend.h>
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
    #include "ModManager.h"
    #include "NetManager.h"
    #include "Peer.h"
    #include "RouteManager.h"
    #include "Types.h"
    #include "ValhallaServer.h"
    #include "VUtilsResource.h"

auto MOD_MANAGER(std::make_unique<IModManager>());

IModManager *ModManager()
{
    return MOD_MANAGER.get();
}

static std::vector<std::string_view> const safe_functions {// Global objects
                                                           "assert", "error", "ipairs", "next", "pairs",
                                                           "pcall", "select", "tonumber", "tostring", "type",
                                                           "unpack", "_VERSION", "xpcall",

                                                           // Full packages
                                                           "coroutine.*", "string.*", "table.*", "math.*",

                                                           // Partial packages
                                                           "os.clock", "os.date", "os.difftime", "os.time"};

IModManager::Mod &IModManager::LoadModInfo(std::string_view folderName)
{
    YAML::Node loadNode;

    auto modPath     = fs::path("mods") / folderName;
    auto modInfoPath = modPath / "modInfo.yml";

    if (auto opt = VUtils::Resource::ReadFile<std::string>(modInfoPath)) {
        loadNode = YAML::Load(opt.value());
    } else {
        throw std::runtime_error("unable to open " + modInfoPath.string());
    }

    auto name = loadNode["name"].as<std::string>();


    // deep copy the 'api', so that we shallow copy all keys, simple strings (immutable reference),
    //  but then the only real thing which MUST remain preserved is the

    //for (auto const& [id_obj, thing] : env) {
    //    std::string id = id_obj.as<std::string>();
    //    LOG_INFO(VH_LOGGER, "got: {}", id);
    //}

    //TODO remove this; TEST ONLY
    //LOG_INFO(VH_LOGGER, "FLUSHED");
    //VH_LOGGER->flush_log(1000);


    auto &&insert = this->m_mods.insert(
            {name, std::make_unique<Mod>(loadNode["name"].as<std::string>(),
                                         modPath / (loadNode["entry"].as<std::string>() + ".lua"))});

    if (!insert.second)
        throw std::runtime_error("Mod " + name + " already loaded");

    auto &&mod = insert.first->second;

    mod->m_version     = loadNode["version"].as<std::string>("");
    mod->m_apiVersion  = loadNode["api-version"].as<std::string>("");
    mod->m_description = loadNode["description"].as<std::string>("");
    mod->m_authors     = loadNode["authors"].as<std::list<std::string>>(std::list<std::string>());

    return *mod;
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

void IModManager::execute_plugin(Mod &mod)
{
    auto path(mod.m_entry);
    if (auto opt = VUtils::Resource::ReadFile<std::string>(path)) {
        // See Lua Sandboxing and containerized execution
        //https://blog.rubenwardy.com/2020/07/26/sol3-script-sandbox/
        //https://forums.solar2d.com/t/unloading-a-lua-module-that-was-required/349169/10
        //http://lua-users.org/wiki/SandBoxes
        //https://ericjmritz.wordpress.com/2015/03/25/creating-and-using-environments-in-lua/
        //https://github.com/ThePhD/sol2/blob/develop/examples/source/environments.cpp

        // Load new API globals personally for this mod
        auto api_table = this->load_api_table();

        auto env    = sol::environment(m_state, sol::create, api_table);
        env["_G"]   = env; // otherwise, will point to our state global table; defeating sandboxing...
        env["this"] = &mod;//TODO TEST

        //sandboxer
        for (auto const &entry : safe_functions) {

            /*
                Entire module loading
            */
            auto idx = entry.rfind(".*");
            if (idx != std::string::npos) {
                // load the package
                auto package_name = entry.substr(0, idx);
                assert(!package_name.contains("."));

                auto copy = env[sol::create_if_nil][package_name];//.get_or_create<sol::table>();
                for (auto [func_name, func] : m_state[package_name].get<sol::table>()) {
                    copy[func_name] = func;

                    assert(env.get<sol::table>(package_name)[func_name].valid());
                }
                continue;
            }

            /*
                Partial module function loading
            */
            idx = entry.find(".");
            if (idx != std::string::npos) {
                // load the partial
                auto package_name = entry.substr(0, idx);
                assert(!package_name.contains("."));

                auto func_name = entry.substr(idx + 1);
                assert(!func_name.contains("."));

                // https://github.com/ThePhD/sol2/blob/develop/examples/source/table_create_if_nil.cpp

                auto func = m_state[package_name][func_name].get<sol::function>();
                assert(func.valid());

                env[sol::create_if_nil][package_name][func_name] = func;

                assert(env.get<sol::table>(package_name)[func_name].valid());
                assert(env.get<sol::table>(package_name)[func_name].get_type() == sol::type::function);

                continue;
            }

            /*
                Global function loading
            */

            //1. works for 99% of types (except _VERSION)
            //env[sol::create_if_nil][entry] = m_state[entry].get<sol::function>();

            env[sol::create_if_nil][entry] = m_state[entry].get<sol::object>();

            //assert(env.get<sol::optional<sol::object>>(entry).has_value()); //fails; cannot find another way to check...
        }

        //TODO might not even need to use environment...

        m_state.safe_script(opt.value(), env, mod.m_entry, sol::load_mode::any);

        //sol::function funcc = result;


        //funcc.se
    } else
        throw std::runtime_error(std::string("unable to open file ") + path.string());
}

//TODO
inline void my_panic(sol::optional<std::string> maybe_msg)
{
    LOG_ERROR(VH_LOGGER, "Lua is in a panic state and will now abort() the application");
    if (maybe_msg) {
        std::string const &msg = maybe_msg.value();
        LOG_ERROR(VH_LOGGER, "\terror message: {}", msg);
    }
    // When this function exits, Lua will exhibit default behavior and abort(), unless I throw...
}

//in my limited usage and experience, no error handler or panic was ever invoked, perhaps because all
//  errors took place INSIDE one of my event handlers...
int my_exception_handler(lua_State *L, sol::optional<std::exception const &> maybe_exception,
                         sol::string_view description)
{
    // L is the lua state, which you can wrap in a state_view if necessary
    // maybe_exception will contain exception, if it exists
    // description will either be the what() of the exception or a description saying that we hit the general-case catch(...)
    LOG_ERROR(VH_LOGGER, "An exception occurred in a function, here's what it says ");
    if (maybe_exception) {
        LOG_ERROR(VH_LOGGER, "(straight from the exception): ");
        LOG_ERROR(VH_LOGGER, "{}", maybe_exception->what());
    } else {
        LOG_ERROR(VH_LOGGER, "(from the description parameter): ");
        LOG_ERROR(VH_LOGGER, "{}", description);
    }

    // you must push 1 element onto the stack to be
    // transported through as the error object in Lua
    // note that Lua -- and 99.5% of all Lua users and libraries -- expects a string
    // so we push a single string (in our case, the description of the error)
    return sol::stack::push(L, description);
}

void IModManager::PostInit()
{
    LOG_NOTICE(VH_LOGGER, "Initializing ModManager");

    m_state.set_panic(sol::c_call<decltype(&my_panic), &my_panic>);
    m_state.set_exception_handler(&my_exception_handler);

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

    std::error_code ec;
    fs::create_directories(VH_MOD_PATH, ec);

    if (ec)
        return;

    auto sorted
            = fs::directory_iterator(VH_MOD_PATH, ec) | std::views::filter([](fs::directory_entry e) -> bool {
                  return e.is_directory() && !std::string_view(e.path().filename().c_str()).starts_with("--");
              })
              | std::ranges::to<std::vector>();
    std::ranges::sort(sorted);

    for (auto const &dir : sorted) {
        try {
            auto &&dirname = dir.path().filename().string();

            auto &&mod = LoadModInfo(dirname);
            execute_plugin(mod);

            LOG_NOTICE(VH_LOGGER, "Loaded mod '{}'", mod.m_name);
        } catch (std::exception const &e) {
            LOG_ERROR(VH_LOGGER, "Failed to load mod: {}", dir.path().string());
            LOG_ERROR(VH_LOGGER, "{}", e.what());
        }
    }

    LOG_NOTICE(VH_LOGGER, "Loaded {} mods", m_mods.size());

    VH_DISPATCH_MOD_EVENT(IModManager::Events::Enable);
}

void IModManager::Uninit()
{
    VH_DISPATCH_MOD_EVENT(IModManager::Events::Disable);
    m_callbacks.clear();
    m_mods.clear();
}

//https://github.com/ThePhD/sol2/issues/980
// to 'reload' a script
//  clear / kill all references to scripts, by manually clearing out listeners / callbacks...

// how to handle script behavior on a reload?
//  a reload is handled not on server start, so some callbacks will never post
//  so rather, run a on_reload() callback that can handle mid-server operations
//  or other way to detect that this script has just been loaded midway through during server operations

void IModManager::update()
{
    if (!m_tmp_reload_mods.empty()) {
        assert(false);//TODO

        /*
            Release all associated callbacks
        */

        for (auto &&itr = m_callbacks.begin(); itr != m_callbacks.end();) {
            auto &&callbacks = itr->second;
            for (auto &&itr1 = callbacks.begin(); itr1 != callbacks.end();) {
                auto &&env = sol::get_environment(itr1->m_func);
                //if (itr1->m_func.e.get() == m_tmp_mod_reload) {
                assert(env.valid());

                assert(env["this"].is<Mod *>());

                auto mod = env["this"].get<Mod *>();

                bool contains = m_tmp_reload_mods.contains(mod);

                if (contains) {
                    itr1 = callbacks.erase(itr1);
                } else {
                    ++itr1;
                }
            }

            // Pop callback set for tidy
            if (callbacks.empty()) {
                itr = m_callbacks.erase(itr);
            } else {
                ++itr;
            }
        }

        /*
            Release all registered RPCs        
        */

        for (auto &&peer_pair : NetManager()->m_connectedPeers) {
            for (auto &&method_itr = peer_pair->m_methods.begin();
                 method_itr != peer_pair->m_methods.end();) {
                auto &&method = dynamic_cast<MethodImplLua<Peer *> *>(method_itr->second.get());

                if (!method) {
                    ++method_itr;
                    continue;
                }

                auto &&env = sol::get_environment(method->m_func);
                assert(env.valid());
                assert(env["this"].is<Mod *>());
                auto mod = env["this"].get<Mod *>();

                bool contains = m_tmp_reload_mods.contains(mod);

                if (contains) {
                    // kill it
                    method_itr = peer_pair->m_methods.erase(method_itr);
                } else {
                    ++method_itr;
                }
            }
        }

        //TODO rethink how everything is shaped...

        // Perhaps start on reworking Valhalla,

        // Migrating towards unit tests like in avl, and avoid
        //  repeated pitfalls as before with debug hell...

        // Clang tidy / formatters to look at,
        //  refactor as a whole...

        // Also unload routed rpcs...
        //for (auto&& mod : m_tmp_reload_mods) {
        //    mod->
        //}

        // https://github.com/ricosolana/Valhalla/blob/0121b3db3788c146fc0eda783c3563cb16ef2ca9/src/ModManager.cpp
        // :::::::::::::::::::OLD::::::::::::::::;
        //for (auto&& pair : m_mods) {
        //    auto&& mod = *pair.second.get();
        //    if (mod.m_reload) {
        //        LOG(INFO) << "Reloading mod " << mod.m_name;

        //        for (auto&& pair : NetManager()->GetPeers()) {
        //            auto&& peer = pair.second;
        //            for (auto&& pair1 : peer->m_methods) {
        //                auto&& method = dynamic_cast<MethodImplLua<Peer*>*>(pair1.second.get());
        //                //if (method)
        //                    //method->m_func =
        //            }
        //            //if (auto method = peer->GetMethod()
        //        }

        //        mod.m_env.reset();
        //        LoadMod(mod);
        //        mod.m_reload = false;
        //    }
        //}

        //m_state.collect_gc();

        //m_tmp_mod_reload = nullptr;
    }
}

#endif// VH_USE_MODS