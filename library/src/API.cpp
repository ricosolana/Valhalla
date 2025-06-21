#include "DungeonManager.h"
#include "ModManager.h"
#include "RouteManager.h"
#include "ValhallaServer.h"
#include "VUtilsResource.h"
#include "ZDOManager.h"
#include <sol/environment.hpp>
#include <sol/forward.hpp>
#include <sol/optional_implementation.hpp>
#include <sol/property.hpp>
#include <sol/types.hpp>

void IModManager::load_userdata()
{
    this->load_userdata_network();
    this->load_userdata_peer();
    this->load_userdata_prefab();
    this->load_userdata_quaternion();
    this->load_userdata_types();
    this->load_userdata_vector();
    this->load_userdata_zdo();
    this->load_userdata_zone();

    this->new_usertype<IValhalla>(
            "IValhalla",
            // server members
            "version", sol::var(VConstants::GAME),// Valheim version
            "delta", sol::property(&IValhalla::delta), "id",
            sol::property([](IValhalla &self) { return Int64Wrapper(self.ID()); }), "nanos",
            sol::property([](IValhalla &self) { return Int64Wrapper(self.Nanos().count()); }), "time",
            sol::property(&IValhalla::Time), "time_multiplier", &IValhalla::m_serverTimeMultiplier,
            // world time functions
            "world_time",
            sol::property(sol::resolve<WorldTime() const>(&IValhalla::GetWorldTime),
                          &IValhalla::SetWorldTime),
            "world_time_multiplier",
            sol::property([](IValhalla &self) { return self.m_worldTimeMultiplier; },
                          [](IValhalla &self, double mul) {
                              if (mul <= 0.001)
                                  throw std::runtime_error("multiplier too small");
                              self.m_worldTimeMultiplier = mul;
                          }),
            "world_ticks", sol::property([](IValhalla &self) { return self.GetWorldTicks(); }), "day",
            sol::property(sol::resolve<int() const>(&IValhalla::GetDay), &IValhalla::SetDay), "time_of_day",
            sol::property(sol::resolve<TimeOfDay() const>(&IValhalla::GetTimeOfDay),
                          &IValhalla::SetTimeOfDay),
            "is_morning", sol::property(sol::resolve<bool() const>(&IValhalla::IsMorning)), "is_day",
            sol::property(sol::resolve<bool() const>(&IValhalla::IsDay)), "is_afternoon",
            sol::property(sol::resolve<bool() const>(&IValhalla::IsAfternoon)), "is_night",
            sol::property(sol::resolve<bool() const>(&IValhalla::IsNight)), "next_morning",
            sol::property(&IValhalla::GetTomorrowMorning), "next_day",
            sol::property(&IValhalla::GetTomorrowDay), "next_afternoon",
            sol::property(&IValhalla::GetTomorrowAfternoon), "next_night",
            sol::property(&IValhalla::GetTomorrowNight),

            "subscribe", [this](IValhalla &self, sol::variadic_args args, sol::this_environment te) {
                sol::environment &env = te;

                auto mod = env["this"].get<Mod *>();

                avledet::util::Hash hash {};
                sol::function func;
                int priority = 0;


                // If priority is present (will be at end)
                int const offset = args[args.size() - 1].get_type() == sol::type::number ? 2 : 1;

                for (int i = 0; i < args.size(); i++) {
                    auto &&arg  = args[i];
                    auto &&type = arg.get_type();

                    if (i + offset < args.size()) {
                        if (type == sol::type::string)
                            hash ^= avledet::util::get_stable_hash(arg.as<std::string>());
                        else if (type == sol::type::number)
                            hash ^= arg.as<avledet::util::Hash>();
                        else {
                            throw std::runtime_error("initial params must be string or hash");
                        }
                    } else {
                        if (i == args.size() - offset && type == sol::type::function) {
                            func = arg;
                        } else if (offset == 2 && i == args.size() - 1 && type == sol::type::number) {
                            priority = arg;
                        } else {
                            throw std::runtime_error("final param must be a function or priority");
                        }
                    }
                }

                auto &&callbacks = m_callbacks[hash];

                callbacks.emplace_back(func, priority);
                callbacks.sort([](EventHandle const &a, EventHandle const &b) {
                    return a.m_priority < b.m_priority;
                });
            });

    this->new_usertype<IModManager>("IModManager", "get_mod",
                                    [](IModManager &self, std::string_view name) {
                                        auto &&find = self.m_mods.find(name);
                                        if (find != self.m_mods.end())
                                            return find->second.get();
                                        return static_cast<Mod *>(nullptr);
                                    }
                                    //"ReloadMod", [](IModManager& self, Mod& mod) {
                                    //    if (!self.m_reload) {
                                    //        mod.m_reload = true;
                                    //        self.m_reload = true;
                                    //    }
                                    //}
    );

    this->new_usertype<Mod>("Mod", "name", sol::readonly(&Mod::m_name),
                            //"entry", sol::readonly(&Mod::m_entry),
                            "version", sol::readonly(&Mod::m_version), "api_version",
                            sol::readonly(&Mod::m_apiVersion), "description",
                            sol::readonly(&Mod::m_description), "authors", sol::readonly(&Mod::m_authors));


    this->new_usertype<IRouteManager>("IRouteManager", "register", &IRouteManager::RegisterLua, "invoke_view",
                                      &IRouteManager::InvokeViewLua, "invoke", &IRouteManager::InvokeLua,
                                      "invoke_all", &IRouteManager::InvokeAllLua);

    // Stl function; Will get copied along with other safe-sandboxed functions
    m_state["print"] = [](sol::variadic_args args, sol::this_environment tenv) {
        sol::environment &env = tenv;
        auto &&tostring(env["tostring"]);
        auto const lua = env.lua_state();

        std::string s;
        int idx = 0;
        for (auto &&arg : args) {
            if (idx++ > 0)
                s += " ";
            s += tostring(arg);
        }

        // https://stackoverflow.com/questions/2555856/current-line-number-in-lua
        lua_Debug ar;
        lua_getstack(lua, 1, &ar);
        lua_getinfo(lua, "nSl", &ar);
        int line = ar.currentline;
        //ar.name

        auto source = ar.source;
        //auto short_src = ar.short_src;

        LOG_INFO(VH_LOGGER, "[{}:{}] {}", source, line, s);

        //LOG_INFO(VH_LOGGER, "[Lua] {}", s);
    };

    //state.new_usertype<IMethod<Peer*>>("IMethodPeer",
    //    "Invoke", &IMethod<Peer*>::Invoke
    //);
}

static std::vector<std::string_view> const safe_functions {// Global objects
                                                           "assert", "error", "ipairs", "next", "pairs",
                                                           "pcall", "print", "select", "tonumber", "tostring",
                                                           "type", "unpack", "_VERSION", "xpcall",

                                                           // Full packages
                                                           "coroutine.*", "string.*", "table.*", "math.*",

                                                           // Partial packages
                                                           "os.clock", "os.date", "os.difftime", "os.time"};

// See Lua Sandboxing and containerized execution
//https://blog.rubenwardy.com/2020/07/26/sol3-script-sandbox/
//https://forums.solar2d.com/t/unloading-a-lua-module-that-was-required/349169/10
//http://lua-users.org/wiki/SandBoxes
//https://ericjmritz.wordpress.com/2015/03/25/creating-and-using-environments-in-lua/
//https://github.com/ThePhD/sol2/blob/develop/examples/source/environments.cpp
sol::environment IModManager::create_sandbox(Mod &mod)
{
    auto env  = sol::environment(m_state, sol::create, m_state.globals());//, api_table);
    env["_G"] = env;// otherwise, will point to our state global table; defeating sandboxing...

    using namespace avledet::util;
    using namespace CSU;

    env["Valhalla"]       = Valhalla();
    env["ModManager"]     = ModManager();
    env["NetManager"]     = NetManager();
    env["PrefabManager"]  = PrefabManager();
    env["ZDOManager"]     = ZDOManager();
    env["DungeonManager"] = DungeonManager();
    env["ZoneManager"]    = ZoneManager();
    env["RouteManager"]   = RouteManager();
    env["this"]           = std::ref(mod);


    //table.new_usertype<IRouteManager::Data>("RouteData",
    //    "sender", &IRouteManager::Data::m_sender,
    //    "target", &IRouteManager::Data::m_target,
    //    "targetZDO", &IRouteManager::Data::m_targetZDO,
    //    "method", &IRouteManager::Data::m_method,
    //    "params", &IRouteManager::Data::m_params
    //);


    {
        auto eventTable = env["event"].get_or_create<sol::table>();

        eventTable["unsubscribe"] = [this]() { this->m_tmp_unsubscribe = true; };
    }


    {
        auto utilsTable = m_state["VUtils"].get_or_create<sol::table>();

        utilsTable["create_bytes"] = []() { return Bytes(); };

        utilsTable["assign"] = sol::overload([](Bytes &replace, Bytes &other) { replace = other; });

        utilsTable["swap"] = sol::overload([](Bytes &a, Bytes &b) { std::swap(a, b); });

        //utilsTable["Move"] = sol::overload(
        //    [](Bytes& a, Bytes& b) { std::swap(a, b); }
        //);

        {
            auto stringUtilsTable = utilsTable["String"].get_or_create<sol::table>();

            //TODO
            //stringUtilsTable["GetStableHashCode"] = get_stable_hash;
        }

        {
            auto resourceUtilsTable = utilsTable["Resource"].get_or_create<sol::table>();

            //resourceUtilsTable["ReadFileBytes"] = sol::resolve<std::optional<Bytes>(const fs::path&)>(VUtils::Resource::ReadFile);
            //resourceUtilsTable["ReadFileString"] = sol::resolve<std::optional<std::string>(const fs::path&)>(VUtils::Resource::ReadFile);
            //resourceUtilsTable["ReadFileLines"] = sol::resolve<std::optional<std::vector<std::string>>(const fs::path&, bool)>(VUtils::Resource::ReadFile);

            resourceUtilsTable["as_bytes"]
                    = [](std::string_view path) { return VUtils::Resource::ReadFile<Bytes>(path); };
            resourceUtilsTable["as_string"]
                    = [](std::string_view path) { return VUtils::Resource::ReadFile<std::string>(path); };
            resourceUtilsTable["as_lines"] = [](std::string_view path) {
                return VUtils::Resource::ReadFile<std::vector<std::string>>(path);
            };

            resourceUtilsTable["write_file"] = sol::overload(
                    sol::resolve<bool(fs::path const &, Bytes const &)>(VUtils::Resource::WriteFile),
                    sol::resolve<bool(fs::path const &, std::string_view)>(VUtils::Resource::WriteFile),
                    sol::resolve<bool(fs::path const &, std::vector<std::string> const &)>(
                            VUtils::Resource::WriteFile),
                    sol::resolve<bool(fs::path const &, std::list<std::string> const &)>(
                            VUtils::Resource::WriteFile));
        }
    }

    /*
        Lua stl sandboxing
    */

    for (auto const &entry : safe_functions) {

        /*
            Entire module loading
        */
        auto idx = entry.rfind(".*");
        if (idx != std::string::npos) {
            // load the package
            auto package_name = entry.substr(0, idx);
            assert(!package_name.contains("."));

            auto copy = env[sol::create_if_nil][package_name];
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

        env[sol::create_if_nil][entry] = m_state[entry].get<sol::object>();
    }

    return env;
}