#include "ModManager.h"
#include "ZDOManager.h"

#if VH_IS_ON(VH_USE_MODS)

void avledet::api::init_zdo(sol::table table) {
    LOG_INFO(VH_LOGGER, "Initializing API types - ZDO");

    using namespace avledet::util;

    table.new_usertype<ZDO>("ZDO",
        sol::no_constructor,
        "id", sol::property(&ZDO::GetID),
        "pos", sol::property(&ZDO::GetPosition, &ZDO::SetPosition),
        "zone", sol::property(&ZDO::GetZone),
        "rot", sol::property(&ZDO::GetRotation, &ZDO::SetRotation),
        "prefab", sol::property(&ZDO::GetPrefab),
        "prefab_hash", sol::property(&ZDO::GetPrefabHash),
        "owner", sol::property([](ZDO self) { return Int64Wrapper(self.Owner()); }, [](ZDO self, Int64Wrapper owner) { self.SetOwner((std::int64_t)owner); }),
        "is_owner", &ZDO::IsOwner, // zdo:is_owner(id)
        "is_local", sol::property(&ZDO::IsLocal, &ZDO::SetLocal),
        //"remote", sol::property(&ZDO::IsLocal, &ZDO::SetLocal),
        //"isLocal", sol::property(&ZDO::IsLocal, [](ZDO& self, bool b) { if (b) self.SetLocal(); else self.Disown(); }),
        "owned", sol::property(&ZDO::HasOwner),
        "disown", &ZDO::Disown, //TODO rename?
        "data_rev", sol::property(&ZDO::GetDataRevision), // sol::property([](ZDO self) { return self.Revision().GetDataRevision(); }),
        "owner_rev", sol::property(&ZDO::GetOwnerRevision),
        //"ticksCreated", sol::property([](ZDO& self) { return (Int64Wrapper) self.m_rev.m_ticksCreated.count(); }), // hmm chrono...
        
        // Getters
        "get_float", sol::overload(
            sol::resolve<float(Hash, float) const>(&ZDO::GetFloat),
            sol::resolve<float(Hash) const>(&ZDO::GetFloat),
            sol::resolve<float(std::string_view, float) const>(&ZDO::GetFloat),
            sol::resolve<float(std::string_view) const>(&ZDO::GetFloat)
        ),
        "get_int", sol::overload(
            sol::resolve<std::int32_t(Hash, std::int32_t) const>(&ZDO::GetInt),
            sol::resolve<std::int32_t(Hash) const>(&ZDO::GetInt),
            sol::resolve<std::int32_t(std::string_view, std::int32_t) const>(&ZDO::GetInt),
            sol::resolve<std::int32_t(std::string_view) const>(&ZDO::GetInt)
        ),
        "get_long", sol::overload(
            sol::resolve<Int64Wrapper(Hash, Int64Wrapper) const>(&ZDO::GetLongWrapper),
            sol::resolve<Int64Wrapper(Hash) const>(&ZDO::GetLongWrapper),
            sol::resolve<Int64Wrapper(std::string_view, Int64Wrapper) const>(&ZDO::GetLongWrapper),
            sol::resolve<Int64Wrapper(std::string_view) const>(&ZDO::GetLongWrapper)
        ),
        "get_quat", sol::overload(
            sol::resolve<Quaternion(Hash, Quaternion) const>(&ZDO::GetQuaternion),
            sol::resolve<Quaternion(Hash) const>(&ZDO::GetQuaternion),
            sol::resolve<Quaternion(std::string_view, Quaternion) const>(&ZDO::GetQuaternion),
            sol::resolve<Quaternion(std::string_view) const>(&ZDO::GetQuaternion)
        ),
        "get_vec3", sol::overload(
            sol::resolve<Vector3f (Hash, Vector3f) const>(&ZDO::GetVector3),
            sol::resolve<Vector3f (Hash) const>(&ZDO::GetVector3),
            sol::resolve<Vector3f (std::string_view, Vector3f) const>(&ZDO::GetVector3),
            sol::resolve<Vector3f (std::string_view) const>(&ZDO::GetVector3)
        ),
        "get_string", sol::overload(
            sol::resolve<std::string_view (Hash, std::string_view) const>(&ZDO::GetString),
            sol::resolve<std::string_view (Hash) const>(&ZDO::GetString),
            sol::resolve<std::string_view (std::string_view, std::string_view) const>(&ZDO::GetString),
            sol::resolve<std::string_view (std::string_view) const>(&ZDO::GetString)
        ),
        "get_bytes", sol::overload(
            sol::resolve<const Bytes* (Hash) const>(&ZDO::GetBytes),
            sol::resolve<const Bytes* (std::string_view) const>(&ZDO::GetBytes)
            //[](ZDO& self, Hash key) { auto&& bytes = self.GetBytes(key); return bytes ? std::make_optional(Bytes(*bytes)) : std::nullopt; },
            //[](ZDO& self, std::string_view key) { auto&& bytes = self.GetBytes(key); return bytes ? std::make_optional(Bytes(*bytes)) : std::nullopt; }
        ),
        "get_bool", sol::overload(
            sol::resolve<bool (Hash, bool) const>(&ZDO::GetBool),
            sol::resolve<bool (Hash) const>(&ZDO::GetBool),
            sol::resolve<bool (std::string_view, bool) const>(&ZDO::GetBool),
            sol::resolve<bool (std::string_view) const>(&ZDO::GetBool)
        ),
        "get_zdoid", sol::overload(
            //sol::resolve<ZDOID(Hash, const ZDOID&) const>(&ZDO::GetZDOID),
            //sol::resolve<ZDOID(Hash) const>(&ZDO::GetZDOID),
            sol::resolve<ZDOID(std::string_view, ZDOID) const>(&ZDO::GetZDOID),
            sol::resolve<ZDOID(std::string_view) const>(&ZDO::GetZDOID)
        ),


        // Setters
        "set_float", sol::overload(
            static_cast<void (ZDO::*)(Hash, float)>(&ZDO::Set),
            static_cast<void (ZDO::*)(std::string_view, float)>(&ZDO::Set)
        ),        
        "set_int", sol::overload(
            static_cast<void (ZDO::*)(Hash, std::int32_t)>(&ZDO::Set),
            static_cast<void (ZDO::*)(std::string_view, std::int32_t)>(&ZDO::Set)
        ),
        "set", sol::overload(
            // Quaternion
            static_cast<void (ZDO::*)(Hash, Quaternion)>(&ZDO::Set),
            static_cast<void (ZDO::*)(std::string_view, Quaternion)>(&ZDO::Set),
            // Vector3f
            static_cast<void (ZDO::*)(Hash, Vector3f)>(&ZDO::Set),
            static_cast<void (ZDO::*)(std::string_view, Vector3f)>(&ZDO::Set),
            // std::string
            static_cast<void (ZDO::*)(Hash, std::string)>(&ZDO::Set),
            static_cast<void (ZDO::*)(std::string_view, std::string)>(&ZDO::Set),
            // bool
            static_cast<void (ZDO::*)(Hash, bool)>(&ZDO::Set),
            static_cast<void (ZDO::*)(std::string_view, bool)>(&ZDO::Set),
            // zdoid
            //static_cast<void (ZDO::*)(Hash, Hash, const ZDOID&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(std::string_view, ZDOID)>(&ZDO::Set),
            // int64 wrapper
            [](ZDO& self, Hash key, Int64Wrapper value) { self.Set(key, (std::int64_t)value); },
            [](ZDO& self, std::string_view key, Int64Wrapper value) { self.Set(key, (std::int64_t)value); }
        )
    );

    // setting meta functions
    // https://sol2.readthedocs.io/en/latest/api/metatable_key.html
    // 
    // TODO figure the number weirdness out...


    

    // TODO turn managers into lua classes that can be indexed
    // but still retrieve with ZDOManager... class usertypes will be named by their class names, like IZDOManager...

    table["ZDOManager"] = ZDOManager();
    table.new_usertype<IZDOManager>("IZDOManager",
        "get_zdo", &IZDOManager::GetZDO,
        "some_zdos", sol::overload(
            sol::resolve<std::list<ZDO::unsafe_value>(Vector3f, float, std::size_t, IZDOManager::pred_t)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(Vector3f, float, std::size_t)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(Vector3f, float, std::size_t, Hash prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent)>(&IZDOManager::SomeZDOs),
            [](IZDOManager& self, const Vector3f& pos, float radius, std::size_t max, std::string_view name) { return self.SomeZDOs(pos, radius, max, get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },
            
            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, std::size_t, IZDOManager::pred_t)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, std::size_t)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, std::size_t, Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::SomeZDOs),
            [](IZDOManager& self, const ZoneID& zone, std::size_t max, std::string_view name) { return self.SomeZDOs(zone, max, get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },

            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, std::size_t, Vector3f, float)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, std::size_t, Vector3f, float, Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::SomeZDOs),
            [](IZDOManager& self, ZoneID zone, std::size_t max, Vector3f pos, float radius, std::string_view name) { return self.SomeZDOs(zone, max, pos, radius, get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); }
        ),
        "get_zdos", sol::overload(
            sol::resolve<std::list<ZDO::unsafe_value>(IZDOManager::pred_t)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(Hash)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, std::string_view name) { return self.GetZDOs(get_stable_hash(name)); },

            sol::resolve<std::list<ZDO::unsafe_value>(Vector3f, float, IZDOManager::pred_t)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(Vector3f, float)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(Vector3f, float, Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, Vector3f pos, float radius, std::string_view name) { return self.GetZDOs(pos, radius, get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },

            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, IZDOManager::pred_t)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID)>(&IZDOManager::GetZDOs),

            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, ZoneID zone, std::string_view name) { return self.GetZDOs(zone, get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },
            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, Vector3f, float)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, Vector3f, float, Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, ZoneID zone, Vector3f pos, float radius, std::string_view name) { return self.GetZDOs(zone, pos, radius, get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); }
        ),
        "any_zdo", sol::overload(
            sol::resolve<ZDO::unsafe_optional (Vector3f, float, Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::AnyZDO),
            [](IZDOManager& self, Vector3f pos, float radius, std::string_view name) { return self.AnyZDO(pos, radius, get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },

            sol::resolve<ZDO::unsafe_optional (ZoneID, Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::AnyZDO),
            [](IZDOManager& self, ZoneID zone, std::string_view name) { return self.AnyZDO(zone, get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); }
        ),
        "nearest_zdo", sol::overload(
            sol::resolve<ZDO::unsafe_optional (Vector3f, float, IZDOManager::pred_t)>(&IZDOManager::NearestZDO),
            sol::resolve<ZDO::unsafe_optional (Vector3f, float, Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::NearestZDO),
            [](IZDOManager& self, Vector3f pos, float radius, std::string_view name) { return self.NearestZDO(pos, radius, get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); }
        ),
        "force_send_zdo", &IZDOManager::ForceSendZDO,
        //"DestroyZDO", sol::resolve<ZDO&>(&IZDOManager::DestroyZDO),
        "destroy_zdo", sol::overload(
            sol::resolve<void (ZDOID)>(&IZDOManager::DestroyZDO),
            sol::resolve<void(ZDO::unsafe_value)>(&IZDOManager::DestroyZDO)
        ),
        "instantiate", sol::overload(
            sol::resolve<ZDO::unsafe_value (Prefab const&, Vector3f)>(&IZDOManager::Instantiate),
            [](IZDOManager& self, std::string_view name, Vector3f pos) { return self.Instantiate(get_stable_hash(name), pos); },
            sol::resolve<ZDO::unsafe_value (Hash, Vector3f)>(&IZDOManager::Instantiate)
            //sol::resolve<ZDO (const ZDO)>(&IZDOManager::Instantiate)
        )

    );
}

#endif
