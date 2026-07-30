#include "CompileSettings.h"
#include "ZDOConnector.h"
#include "ZDOID.h"
#include <sol/raii.hpp>
#include <sol/resolve.hpp>

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    #include <sol/forward.hpp>

    #include "ModManager.h"
    #include "Types.h"
    #include "ZDOManager.h"

void ScriptManager::load_userdata_zdo()
{
    LOG_DEBUG(AVL_LOGGER, "Initializing API types - ZDO");

    using namespace avledet::util;

    // clang-format off

    this->new_enum("ConnectorType", 
        "NONE", ZDOConnector::Type::None, 
        "PORTAL", ZDOConnector::Type::Portal,
        "SYNC_TRANSFORM", ZDOConnector::Type::SyncTransform, 
        "SPAWNED", ZDOConnector::Type::Spawned, 
        "TARGET", ZDOConnector::Type::Target);

    this->new_usertype<ZDOID>("Zdoid",
        sol::constructors<ZDOID(std::int64_t, std::uint32_t)>(), 
        "NONE", sol::var(ZDOID::NONE),
        "user_id", sol::property(&ZDOID::get_user_id, &ZDOID::set_user_id),
        "id", sol::property(&ZDOID::get_id, &ZDOID::set_id)
    );

    this->new_usertype<ZDO>("Zdo", 
        sol::no_constructor, 
        "id", sol::property(&ZDO::get_id), 
        "pos", sol::property(&ZDO::get_position, &ZDO::set_position), 
        "zone", sol::property(&ZDO::get_zone), 
        "rot", sol::property(&ZDO::get_rotation, &ZDO::set_rotation), 
        "prefab", sol::property(&ZDO::get_prefab), // TODO make return a ptr / std ref, not by &reference
        "prefab_hash", sol::property(&ZDO::get_prefab_hash), 
        "owner", sol::property(&ZDO::get_owner, &ZDO::set_owner),
        "is_owner", &ZDO::is_owner,// zdo:is_owner(id)
        "mine", sol::property(&ZDO::owned_by_me, sol::resolve<void(bool)>(&ZDO::set_claimed)),
        //"remote", sol::property(&ZDO::owned_by_me, &ZDO::set_claimed),
        //"isLocal", sol::property(&ZDO::owned_by_me, [](ZDO& self, bool b) { if (b) self.set_claimed(); else self.disown(); }),
        "owned", sol::property(&ZDO::has_owner), 
        "disown", &ZDO::disown,//TODO rename?
        "data_rev",sol::property(&ZDO::get_data_rev),// sol::property([](ZDO self) { return self.Revision().get_data_rev(); }),
        "owner_rev", sol::property(&ZDO::get_owner_rev),
        //"ticksCreated", sol::property([](ZDO& self) { return (Int64Wrapper) self.m_rev.m_ticksCreated.count(); }), // hmm chrono...

        // Getters
        "get_float", sol::overload(
            sol::resolve<float(Hash, float) const>(&ZDO::get_float),
            sol::resolve<float(Hash) const>(&ZDO::get_float),
            sol::resolve<float(std::string_view, float) const>(&ZDO::get_float),
            sol::resolve<float(std::string_view) const>(&ZDO::get_float)),
        "get_int", sol::overload(
            sol::resolve<std::int32_t(Hash, std::int32_t) const>(&ZDO::get_int),
            sol::resolve<std::int32_t(Hash) const>(&ZDO::get_int),
            sol::resolve<std::int32_t(std::string_view, std::int32_t) const>(&ZDO::get_int),
            sol::resolve<std::int32_t(std::string_view) const>(&ZDO::get_int)),
        "get_long", sol::overload(
            sol::resolve<std::int64_t(Hash, std::int64_t) const>(&ZDO::get_long),
            sol::resolve<std::int64_t(Hash) const>(&ZDO::get_long),
            sol::resolve<std::int64_t(std::string_view, std::int64_t) const>(&ZDO::get_long),
            sol::resolve<std::int64_t(std::string_view) const>(&ZDO::get_long)),
        "get_quat", sol::overload(
            sol::resolve<Quaternion(Hash, Quaternion) const>(&ZDO::get_quat),
            sol::resolve<Quaternion(Hash) const>(&ZDO::get_quat),
            sol::resolve<Quaternion(std::string_view, Quaternion) const>(&ZDO::get_quat),
            sol::resolve<Quaternion(std::string_view) const>(&ZDO::get_quat)),
        "get_vec3f", sol::overload(
            sol::resolve<Vector3f(Hash, Vector3f) const>(&ZDO::get_vec3),
            sol::resolve<Vector3f(Hash) const>(&ZDO::get_vec3),
            sol::resolve<Vector3f(std::string_view, Vector3f) const>(&ZDO::get_vec3),
            sol::resolve<Vector3f(std::string_view) const>(&ZDO::get_vec3)),
        "get_string", sol::overload(
            sol::resolve<std::string_view(Hash, std::string_view) const>(&ZDO::get_string),
            sol::resolve<std::string_view(Hash) const>(&ZDO::get_string),
            sol::resolve<std::string_view(std::string_view, std::string_view) const>(&ZDO::get_string),
            sol::resolve<std::string_view(std::string_view) const>(&ZDO::get_string)),
        // TODO DO NOT RETURN POINTERS TO BYTES
            //"get_bytes", sol::overload(
        //    //sol::resolve<Bytes const *(Hash) const>(&ZDO::find_bytes),
        //    //sol::resolve<Bytes const *(std::string_view) const>(&ZDO::find_bytes)
        //    ////[](ZDO& self, Hash key) { auto&& bytes = self.find_bytes(key); return bytes ? std::make_optional(Bytes(*bytes)) : std::nullopt; },
        //    ////[](ZDO& self, std::string_view key) { auto&& bytes = self.find_bytes(key); return bytes ? std::make_optional(Bytes(*bytes)) : std::nullopt; }
        //    ),
        "get_bool", sol::overload(sol::resolve<bool(Hash, bool) const>(&ZDO::get_bool),
            sol::resolve<bool(Hash) const>(&ZDO::get_bool),
            sol::resolve<bool(std::string_view, bool) const>(&ZDO::get_bool),
            sol::resolve<bool(std::string_view) const>(&ZDO::get_bool)),
        "get_zdoid",sol::overload(
            //sol::resolve<ZDOID(Hash, const ZDOID&) const>(&ZDO::get_zdoid),
            //sol::resolve<ZDOID(Hash) const>(&ZDO::get_zdoid),
            sol::resolve<ZDOID(std::string_view, ZDOID) const>(&ZDO::get_zdoid),
            sol::resolve<ZDOID(std::string_view) const>(&ZDO::get_zdoid)),

        // Setters
        "set_float", sol::overload(
            static_cast<bool (ZDO::*)(Hash, float)>(&ZDO::set),
            static_cast<bool (ZDO::*)(std::string_view, float)>(&ZDO::set)),
        "set_int", sol::overload(
            static_cast<bool (ZDO::*)(Hash, std::int32_t)>(&ZDO::set),
            static_cast<bool (ZDO::*)(std::string_view, std::int32_t)>(&ZDO::set)),
        "set", sol::overload(
            // Quaternion
            static_cast<bool (ZDO::*)(Hash, Quaternion)>(&ZDO::set),
            static_cast<bool (ZDO::*)(std::string_view, Quaternion)>(&ZDO::set),
            // Vector3f
            static_cast<bool (ZDO::*)(Hash, Vector3f)>(&ZDO::set),
            static_cast<bool (ZDO::*)(std::string_view, Vector3f)>(&ZDO::set),
            // std::string
            static_cast<bool (ZDO::*)(Hash, std::string)>(&ZDO::set),
            static_cast<bool (ZDO::*)(std::string_view, std::string)>(&ZDO::set),
            // bool
            static_cast<bool (ZDO::*)(Hash, bool)>(&ZDO::set),
            static_cast<bool (ZDO::*)(std::string_view, bool)>(&ZDO::set),
            // zdoid
            //static_cast<void (ZDO::*)(Hash, Hash, const ZDOID&)>(&ZDO::set),
            static_cast<bool (ZDO::*)(std::string_view, ZDOID)>(&ZDO::set)
        ),

        // TODO create enum CONNECTOR,
        // Also, create property accessor for the connected
        "set_connection", &ZDO::set_connection, 
        "get_connection", sol::overload(
            sol::resolve<ZDOID(ZDOConnector::Type) const>(&ZDO::get_connection_zdoid)
            //sol::resolve<ZDOID() const>(&ZDO::get_connection_zdoid)
        )


    );

    // setting meta functions
    // https://sol2.readthedocs.io/en/latest/api/metatable_key.html
    //
    // TODO figure the number weirdness out...


    // TODO turn managers into lua classes that can be indexed
    // but still retrieve with ZDOManager... class usertypes will be named by their class names, like ZdoManager...


    this->new_usertype<ZdoManager>("IZdoManager", 
        sol::no_constructor,
        "find_zdo", &ZdoManager::find_zdo, 
        "some_zdos", sol::overload(
            sol::resolve<ZDO::reference_list(Vector3f const &, float, std::size_t,
                                                ZDO::Filter const &)>(&ZdoManager::SomeZDOs),
            sol::resolve<ZDO::reference_list(Vector3f const &, float, std::size_t)>(
                    &ZdoManager::SomeZDOs),
            sol::resolve<ZDO::reference_list(Vector3f const &, float, std::size_t, Hash prefabHash,
                                                Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent)>(
                    &ZdoManager::SomeZDOs),
            [](ZdoManager &self, Vector3f const &pos, float radius, std::size_t max,
                std::string_view name) {
                return self.SomeZDOs(pos, radius, max, get_stable_hash(name), Prefab::Flag::NONE,
                                        Prefab::Flag::NONE);
            },

            sol::resolve<ZDO::reference_list(ZoneID const &, std::size_t, ZDO::Filter const &)>(
                    &ZdoManager::SomeZDOs),
            sol::resolve<ZDO::reference_list(ZoneID const &, std::size_t)>(&ZdoManager::SomeZDOs),
            sol::resolve<ZDO::reference_list(ZoneID const &, std::size_t, Hash, Prefab::Flag,
                                                Prefab::Flag)>(&ZdoManager::SomeZDOs),
            [](ZdoManager &self, ZoneID const &zone, std::size_t max, std::string_view name) {
                return self.SomeZDOs(zone, max, get_stable_hash(name), Prefab::Flag::NONE,
                                        Prefab::Flag::NONE);
            },

            sol::resolve<ZDO::reference_list(ZoneID const &, std::size_t, Vector3f const &, float)>(
                    &ZdoManager::SomeZDOs),
            sol::resolve<ZDO::reference_list(ZoneID const &, std::size_t, Vector3f const &, float,
                                                Hash, Prefab::Flag, Prefab::Flag)>(
                    &ZdoManager::SomeZDOs),
            [](ZdoManager &self, ZoneID const &zone, std::size_t max, Vector3f const &pos,
                float radius, std::string_view name) {
                return self.SomeZDOs(zone, max, pos, radius, get_stable_hash(name),
                                        Prefab::Flag::NONE, Prefab::Flag::NONE);
            }),
        "get_zdos", sol::overload(
            sol::resolve<ZDO::reference_list(ZDO::Filter const &)>(&ZdoManager::GetZDOs),
            sol::resolve<ZDO::reference_list(Hash)>(&ZdoManager::GetZDOs),
            [](ZdoManager &self, std::string_view name) {
                return self.GetZDOs(get_stable_hash(name));
            },

            sol::resolve<ZDO::reference_list(Vector3f const &, float, ZDO::Filter const &)>(
                    &ZdoManager::GetZDOs),
            sol::resolve<ZDO::reference_list(Vector3f const &, float)>(&ZdoManager::GetZDOs),
            sol::resolve<ZDO::reference_list(Vector3f const &, float, Hash, Prefab::Flag,
                                                Prefab::Flag)>(&ZdoManager::GetZDOs),
            [](ZdoManager &self, Vector3f const &pos, float radius, std::string_view name) {
                return self.GetZDOs(pos, radius, get_stable_hash(name), Prefab::Flag::NONE,
                                    Prefab::Flag::NONE);
            },

            sol::resolve<ZDO::reference_list(ZoneID const &, ZDO::Filter const &)>(
                    &ZdoManager::GetZDOs),
            sol::resolve<ZDO::reference_list(ZoneID const &)>(&ZdoManager::GetZDOs),

            sol::resolve<ZDO::reference_list(ZoneID const &, Hash, Prefab::Flag, Prefab::Flag)>(
                    &ZdoManager::GetZDOs),
            [](ZdoManager &self, ZoneID zone, std::string_view name) {
                return self.GetZDOs(zone, get_stable_hash(name), Prefab::Flag::NONE,
                                    Prefab::Flag::NONE);
            },
            sol::resolve<ZDO::reference_list(ZoneID const &, Vector3f const &, float)>(
                    &ZdoManager::GetZDOs),
            sol::resolve<ZDO::reference_list(ZoneID const &, Vector3f const &, float, Hash,
                                                Prefab::Flag, Prefab::Flag)>(&ZdoManager::GetZDOs),
            [](ZdoManager &self, ZoneID const &zone, Vector3f const &pos, float radius,
                std::string_view name) {
                return self.GetZDOs(zone, pos, radius, get_stable_hash(name), Prefab::Flag::NONE,
                                    Prefab::Flag::NONE);
            }),
        "any_zdo", sol::overload(
            sol::resolve<ZDO::optional(Vector3f const &, float, Hash, Prefab::Flag, Prefab::Flag)>(
                    &ZdoManager::AnyZDO),
            [](ZdoManager &self, Vector3f const &pos, float radius, std::string_view name) {
                return self.AnyZDO(pos, radius, get_stable_hash(name), Prefab::Flag::NONE,
                                    Prefab::Flag::NONE);
            },

            sol::resolve<ZDO::optional(ZoneID const &, Hash, Prefab::Flag, Prefab::Flag)>(
                    &ZdoManager::AnyZDO),
            [](ZdoManager &self, ZoneID const &zone, std::string_view name) {
                return self.AnyZDO(zone, get_stable_hash(name), Prefab::Flag::NONE,
                                    Prefab::Flag::NONE);
            }),
        "nearest_zdo", sol::overload(
            sol::resolve<ZDO::optional(Vector3f const &, float, ZDO::Filter const &)>(
                    &ZdoManager::NearestZDO),
            sol::resolve<ZDO::optional(Vector3f const &, float, Hash, Prefab::Flag, Prefab::Flag)>(
                    &ZdoManager::NearestZDO),
            [](ZdoManager &self, Vector3f const &pos, float radius, std::string_view name) {
                return self.NearestZDO(pos, radius, get_stable_hash(name), Prefab::Flag::NONE,
                                        Prefab::Flag::NONE);
            }),
        "force_send_zdo", &ZdoManager::ForceSendZDO,
        "destroy_zdo",sol::overload(
            sol::resolve<void(ZDOID const &)>(&ZdoManager::DestroyZDO),
            sol::resolve<void(ZDO::reference)>(&ZdoManager::DestroyZDO)),
        "instantiate", sol::overload(
            sol::resolve<ZDO::reference(Prefab::Reference, Vector3f)>(&ZdoManager::Instantiate),
            [](ZdoManager &self, std::string_view name, Vector3f pos) {
                return self.Instantiate(get_stable_hash(name), pos);
            },
            sol::resolve<ZDO::reference(Hash, Vector3f)>(&ZdoManager::Instantiate)
            //sol::resolve<ZDO (const ZDO)>(&ZdoManager::Instantiate)
        )

    );

    // clang-format on
}

#endif
