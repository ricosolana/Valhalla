#include "ModManager.h"
#include "RouteManager.h"
#include "VUtilsResource.h"
#include <sol/types.hpp>

sol::table IModManager::load_api_table()
{
    //auto &&state = m_state;

    sol::table table(m_state, sol::create);

    using namespace avledet::util;
    using namespace CSU;

    // Moved the Vectors/Quat to seperate compilation unit to
    //  reduce compilation times for this massive module
    //avledet::util::CSU::init_api(table);

    avledet::api::init_network(table);
    avledet::api::init_peer(table);
    avledet::api::init_prefab(table);
    avledet::api::init_quaternion(table);
    avledet::api::init_types(table);
    avledet::api::init_vector(table);
    avledet::api::init_zdo(table);
    avledet::api::init_zone(table);

    //state.new_usertype<IMethod<Peer*>>("IMethodPeer",
    //    "Invoke", &IMethod<Peer*>::Invoke
    //);


    // *NOTE: IMPORTANT
    //  See 'version' key
    //      If 'VConstants::GAME' type is changed to anything besides a 'const char*', this breaks compilation
    //      due to oddities with sol::var(...) resolution...
    table["Valhalla"] = Valhalla();
    table.new_usertype<IValhalla>(
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

            "subscribe", [this](IValhalla &self, sol::variadic_args args) {
                Hash hash = 0;
                sol::function func;
                int priority = 0;

                // If priority is present (will be at end)
                int const offset = args[args.size() - 1].get_type() == sol::type::number ? 2 : 1;

                for (int i = 0; i < args.size(); i++) {
                    auto &&arg  = args[i];
                    auto &&type = arg.get_type();

                    if (i + offset < args.size()) {
                        if (type == sol::type::string)
                            hash ^= get_stable_hash(arg.as<std::string>());
                        else if (type == sol::type::number)
                            hash ^= arg.as<Hash>();
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


    table["ModManager"] = ModManager();
    table.new_usertype<IModManager>("IModManager", "get_mod",
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


    // TODO use properties for immutability
    table.new_usertype<Mod>("Mod", "name", sol::readonly(&Mod::m_name), "version",
                            sol::readonly(&Mod::m_version), "api_version", sol::readonly(&Mod::m_apiVersion),
                            "description", sol::readonly(&Mod::m_description), "authors",
                            sol::readonly(&Mod::m_authors));


    //table.new_usertype<IRouteManager::Data>("RouteData",
    //    "sender", &IRouteManager::Data::m_sender,
    //    "target", &IRouteManager::Data::m_target,
    //    "targetZDO", &IRouteManager::Data::m_targetZDO,
    //    "method", &IRouteManager::Data::m_method,
    //    "params", &IRouteManager::Data::m_params
    //);

    table["RouteManager"] = RouteManager();
    table.new_usertype<IRouteManager>("IRouteManager", "register", &IRouteManager::RegisterLua, "invoke_view",
                                      &IRouteManager::InvokeViewLua, "invoke", &IRouteManager::InvokeLua,
                                      "invoke_all", &IRouteManager::InvokeAllLua);


    {
        auto eventTable = table["event"].get_or_create<sol::table>();

        eventTable["unsubscribe"] = [this]() { this->m_tmp_unsubscribe = false; };
    }

    //TODO logger ref capture; fix
    table["print"] = [](sol::variadic_args args, sol::this_environment tenv) {
        sol::environment &env = tenv;
        auto &&tostring(env["tostring"]);

        std::string s;
        int idx = 0;
        for (auto &&arg : args) {
            if (idx++ > 0)
                s += " ";
            s += tostring(arg);
        }

        LOG_INFO(VH_LOGGER, "[Lua] {}", s);
    };


    {
        auto utilsTable = table["VUtils"].get_or_create<sol::table>();

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

    return table;
}