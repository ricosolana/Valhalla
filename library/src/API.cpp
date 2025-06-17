#include "ModManager.h"
#include "RouteManager.h"
#include "ValhallaServer.h"
#include "VUtilsResource.h"
#include <sol/environment.hpp>
#include <sol/forward.hpp>
#include <sol/optional_implementation.hpp>
#include <sol/types.hpp>

sol::table IModManager::load_api_table()
{
    //auto &&state = m_state;

    auto table = sol::table(m_state, sol::create);

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

    //assert(!table["Valhalla"].valid());

    //auto &&vh1 = table.get<sol::optional<IValhalla *>>("Valhalla");//.get_type();
    //auto&& type = table["Valhalla"].get_type();

    auto vh1 = table.get_or<IValhalla *>("Valhalla", nullptr);//.get_type();

    //assert(!table["Valhalla"].get<sol::optional<IValhalla *>>().has_value());

    //assert(!table["IValhalla"].get<sol::optional<IValhalla *>>().has_value());

    // *NOTE: IMPORTANT
    //  See 'version' key
    //      If 'VConstants::GAME' type is changed to anything besides a 'const char*', this breaks compilation
    //      due to oddities with sol::var(...) resolution...
    //sol::state_view(table.lua_state())["Valhalla"] = Valhalla();


    //auto vh2 = table.get_or<IValhalla *>("Valhalla", nullptr);//.get_type();

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