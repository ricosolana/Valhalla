#include "CompileSettings.h"

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    #include <sol/forward.hpp>

    #include "ModManager.h"
    #include "ZDOManager.h"

struct ZDOWrapper
{
    ZDOID m_id;

    //Proxy ZDO:
    //  a simulated zdo, will make on demand changes by map retrieval

    //Unlinked ZDO:
    //  an unattached ZDO, which will NOT make real changes UNTIL applied()
    //  - apply() or sync() or sync_with_manager() or
    //      - apply sounds better, but is ugly worded
    //      - sync perfectly describes this, but this word commonly utilized for zdo operations regarding peers... would become confusing
};

void IScriptManager::load_userdata_zdo()
{
    LOG_DEBUG(AVL_LOGGER, "Initializing API types - ZDO");

    using namespace avledet::util;

    this->new_usertype<ZDO>(
            "ZDO", sol::no_constructor, "id", sol::property(&ZDO::GetID), "pos",
            sol::property(&ZDO::GetPosition, &ZDO::SetPosition), "zone", sol::property(&ZDO::GetZone), "rot",
            sol::property(&ZDO::GetRotation, &ZDO::SetRotation), "prefab", sol::property(&ZDO::GetPrefab),
            "prefab_hash", sol::property(&ZDO::GetPrefabHash), "owner",
            sol::property([](ZDO self) { return Int64Wrapper(self.Owner()); },
                          [](ZDO self, Int64Wrapper owner) { self.SetOwner((std::int64_t) owner); }),
            "is_owner", &ZDO::IsOwner,// zdo:is_owner(id)
            "is_local", sol::property(&ZDO::IsLocal, &ZDO::SetLocal),
            //"remote", sol::property(&ZDO::IsLocal, &ZDO::SetLocal),
            //"isLocal", sol::property(&ZDO::IsLocal, [](ZDO& self, bool b) { if (b) self.SetLocal(); else self.Disown(); }),
            "owned", sol::property(&ZDO::HasOwner), "disown", &ZDO::Disown,//TODO rename?
            "data_rev",
            sol::property(
                    &ZDO::GetDataRevision),// sol::property([](ZDO self) { return self.Revision().GetDataRevision(); }),
            "owner_rev", sol::property(&ZDO::GetOwnerRevision),
            //"ticksCreated", sol::property([](ZDO& self) { return (Int64Wrapper) self.m_rev.m_ticksCreated.count(); }), // hmm chrono...

            // Getters
            "get_float",
            sol::overload(sol::resolve<float(Hash, float) const>(&ZDO::GetFloat),
                          sol::resolve<float(Hash) const>(&ZDO::GetFloat),
                          sol::resolve<float(std::string_view, float) const>(&ZDO::GetFloat),
                          sol::resolve<float(std::string_view) const>(&ZDO::GetFloat)),
            "get_int",
            sol::overload(sol::resolve<std::int32_t(Hash, std::int32_t) const>(&ZDO::GetInt),
                          sol::resolve<std::int32_t(Hash) const>(&ZDO::GetInt),
                          sol::resolve<std::int32_t(std::string_view, std::int32_t) const>(&ZDO::GetInt),
                          sol::resolve<std::int32_t(std::string_view) const>(&ZDO::GetInt)),
            "get_long",
            sol::overload(
                    sol::resolve<Int64Wrapper(Hash, Int64Wrapper) const>(&ZDO::GetLongWrapper),
                    sol::resolve<Int64Wrapper(Hash) const>(&ZDO::GetLongWrapper),
                    sol::resolve<Int64Wrapper(std::string_view, Int64Wrapper) const>(&ZDO::GetLongWrapper),
                    sol::resolve<Int64Wrapper(std::string_view) const>(&ZDO::GetLongWrapper)),
            "get_quat",
            sol::overload(sol::resolve<Quaternion(Hash, Quaternion) const>(&ZDO::GetQuaternion),
                          sol::resolve<Quaternion(Hash) const>(&ZDO::GetQuaternion),
                          sol::resolve<Quaternion(std::string_view, Quaternion) const>(&ZDO::GetQuaternion),
                          sol::resolve<Quaternion(std::string_view) const>(&ZDO::GetQuaternion)),
            "get_vec3",
            sol::overload(sol::resolve<Vector3f(Hash, Vector3f) const>(&ZDO::GetVector3),
                          sol::resolve<Vector3f(Hash) const>(&ZDO::GetVector3),
                          sol::resolve<Vector3f(std::string_view, Vector3f) const>(&ZDO::GetVector3),
                          sol::resolve<Vector3f(std::string_view) const>(&ZDO::GetVector3)),
            "get_string",
            sol::overload(
                    sol::resolve<std::string_view(Hash, std::string_view) const>(&ZDO::GetString),
                    sol::resolve<std::string_view(Hash) const>(&ZDO::GetString),
                    sol::resolve<std::string_view(std::string_view, std::string_view) const>(&ZDO::GetString),
                    sol::resolve<std::string_view(std::string_view) const>(&ZDO::GetString)),
            "get_bytes",
            sol::overload(
                    sol::resolve<Bytes const *(Hash) const>(&ZDO::GetBytes),
                    sol::resolve<Bytes const *(std::string_view) const>(&ZDO::GetBytes)
                    //[](ZDO& self, Hash key) { auto&& bytes = self.GetBytes(key); return bytes ? std::make_optional(Bytes(*bytes)) : std::nullopt; },
                    //[](ZDO& self, std::string_view key) { auto&& bytes = self.GetBytes(key); return bytes ? std::make_optional(Bytes(*bytes)) : std::nullopt; }
                    ),
            "get_bool",
            sol::overload(sol::resolve<bool(Hash, bool) const>(&ZDO::GetBool),
                          sol::resolve<bool(Hash) const>(&ZDO::GetBool),
                          sol::resolve<bool(std::string_view, bool) const>(&ZDO::GetBool),
                          sol::resolve<bool(std::string_view) const>(&ZDO::GetBool)),
            "get_zdoid",
            sol::overload(
                    //sol::resolve<ZDOID(Hash, const ZDOID&) const>(&ZDO::GetZDOID),
                    //sol::resolve<ZDOID(Hash) const>(&ZDO::GetZDOID),
                    sol::resolve<ZDOID(std::string_view, ZDOID) const>(&ZDO::GetZDOID),
                    sol::resolve<ZDOID(std::string_view) const>(&ZDO::GetZDOID)),


            // Setters
            "set_float",
            sol::overload(static_cast<void (ZDO::*)(Hash, float)>(&ZDO::Set),
                          static_cast<void (ZDO::*)(std::string_view, float)>(&ZDO::Set)),
            "set_int",
            sol::overload(static_cast<void (ZDO::*)(Hash, std::int32_t)>(&ZDO::Set),
                          static_cast<void (ZDO::*)(std::string_view, std::int32_t)>(&ZDO::Set)),
            "set",
            sol::overload(
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
                    [](ZDO &self, Hash key, Int64Wrapper value) { self.Set(key, (std::int64_t) value); },
                    [](ZDO &self, std::string_view key, Int64Wrapper value) {
                        self.Set(key, (std::int64_t) value);
                    }));

    // setting meta functions
    // https://sol2.readthedocs.io/en/latest/api/metatable_key.html
    //
    // TODO figure the number weirdness out...


    // TODO turn managers into lua classes that can be indexed
    // but still retrieve with ZDOManager... class usertypes will be named by their class names, like IZDOManager...


    this->new_usertype<IZDOManager>(
            "IZDOManager", "get_zdo", &IZDOManager::GetZDO, "some_zdos",
            sol::overload(
                    sol::resolve<ZDO::reference_list(Vector3f const &, float, std::size_t,
                                                     ZDO::Filter const &)>(&IZDOManager::SomeZDOs),
                    sol::resolve<ZDO::reference_list(Vector3f const &, float, std::size_t)>(
                            &IZDOManager::SomeZDOs),
                    sol::resolve<ZDO::reference_list(Vector3f const &, float, std::size_t, Hash prefabHash,
                                                     Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent)>(
                            &IZDOManager::SomeZDOs),
                    [](IZDOManager &self, Vector3f const &pos, float radius, std::size_t max,
                       std::string_view name) {
                        return self.SomeZDOs(pos, radius, max, get_stable_hash(name), Prefab::Flag::NONE,
                                             Prefab::Flag::NONE);
                    },

                    sol::resolve<ZDO::reference_list(ZoneID const &, std::size_t, ZDO::Filter const &)>(
                            &IZDOManager::SomeZDOs),
                    sol::resolve<ZDO::reference_list(ZoneID const &, std::size_t)>(&IZDOManager::SomeZDOs),
                    sol::resolve<ZDO::reference_list(ZoneID const &, std::size_t, Hash, Prefab::Flag,
                                                     Prefab::Flag)>(&IZDOManager::SomeZDOs),
                    [](IZDOManager &self, ZoneID const &zone, std::size_t max, std::string_view name) {
                        return self.SomeZDOs(zone, max, get_stable_hash(name), Prefab::Flag::NONE,
                                             Prefab::Flag::NONE);
                    },

                    sol::resolve<ZDO::reference_list(ZoneID const &, std::size_t, Vector3f const &, float)>(
                            &IZDOManager::SomeZDOs),
                    sol::resolve<ZDO::reference_list(ZoneID const &, std::size_t, Vector3f const &, float,
                                                     Hash, Prefab::Flag, Prefab::Flag)>(
                            &IZDOManager::SomeZDOs),
                    [](IZDOManager &self, ZoneID const &zone, std::size_t max, Vector3f const &pos,
                       float radius, std::string_view name) {
                        return self.SomeZDOs(zone, max, pos, radius, get_stable_hash(name),
                                             Prefab::Flag::NONE, Prefab::Flag::NONE);
                    }),
            "get_zdos",
            sol::overload(
                    sol::resolve<ZDO::reference_list(ZDO::Filter const &)>(&IZDOManager::GetZDOs),
                    sol::resolve<ZDO::reference_list(Hash)>(&IZDOManager::GetZDOs),
                    [](IZDOManager &self, std::string_view name) {
                        return self.GetZDOs(get_stable_hash(name));
                    },

                    sol::resolve<ZDO::reference_list(Vector3f const &, float, ZDO::Filter const &)>(
                            &IZDOManager::GetZDOs),
                    sol::resolve<ZDO::reference_list(Vector3f const &, float)>(&IZDOManager::GetZDOs),
                    sol::resolve<ZDO::reference_list(Vector3f const &, float, Hash, Prefab::Flag,
                                                     Prefab::Flag)>(&IZDOManager::GetZDOs),
                    [](IZDOManager &self, Vector3f const &pos, float radius, std::string_view name) {
                        return self.GetZDOs(pos, radius, get_stable_hash(name), Prefab::Flag::NONE,
                                            Prefab::Flag::NONE);
                    },

                    sol::resolve<ZDO::reference_list(ZoneID const &, ZDO::Filter const &)>(
                            &IZDOManager::GetZDOs),
                    sol::resolve<ZDO::reference_list(ZoneID const &)>(&IZDOManager::GetZDOs),

                    sol::resolve<ZDO::reference_list(ZoneID const &, Hash, Prefab::Flag, Prefab::Flag)>(
                            &IZDOManager::GetZDOs),
                    [](IZDOManager &self, ZoneID zone, std::string_view name) {
                        return self.GetZDOs(zone, get_stable_hash(name), Prefab::Flag::NONE,
                                            Prefab::Flag::NONE);
                    },
                    sol::resolve<ZDO::reference_list(ZoneID const &, Vector3f const &, float)>(
                            &IZDOManager::GetZDOs),
                    sol::resolve<ZDO::reference_list(ZoneID const &, Vector3f const &, float, Hash,
                                                     Prefab::Flag, Prefab::Flag)>(&IZDOManager::GetZDOs),
                    [](IZDOManager &self, ZoneID const &zone, Vector3f const &pos, float radius,
                       std::string_view name) {
                        return self.GetZDOs(zone, pos, radius, get_stable_hash(name), Prefab::Flag::NONE,
                                            Prefab::Flag::NONE);
                    }),
            "any_zdo",
            sol::overload(
                    sol::resolve<ZDO::optional(Vector3f const &, float, Hash, Prefab::Flag, Prefab::Flag)>(
                            &IZDOManager::AnyZDO),
                    [](IZDOManager &self, Vector3f const &pos, float radius, std::string_view name) {
                        return self.AnyZDO(pos, radius, get_stable_hash(name), Prefab::Flag::NONE,
                                           Prefab::Flag::NONE);
                    },

                    sol::resolve<ZDO::optional(ZoneID const &, Hash, Prefab::Flag, Prefab::Flag)>(
                            &IZDOManager::AnyZDO),
                    [](IZDOManager &self, ZoneID const &zone, std::string_view name) {
                        return self.AnyZDO(zone, get_stable_hash(name), Prefab::Flag::NONE,
                                           Prefab::Flag::NONE);
                    }),
            "nearest_zdo",
            sol::overload(
                    sol::resolve<ZDO::optional(Vector3f const &, float, ZDO::Filter const &)>(
                            &IZDOManager::NearestZDO),
                    sol::resolve<ZDO::optional(Vector3f const &, float, Hash, Prefab::Flag, Prefab::Flag)>(
                            &IZDOManager::NearestZDO),
                    [](IZDOManager &self, Vector3f const &pos, float radius, std::string_view name) {
                        return self.NearestZDO(pos, radius, get_stable_hash(name), Prefab::Flag::NONE,
                                               Prefab::Flag::NONE);
                    }),
            "force_send_zdo", &IZDOManager::ForceSendZDO,
            //"DestroyZDO", sol::resolve<ZDO&>(&IZDOManager::DestroyZDO),
            "destroy_zdo",
            sol::overload(sol::resolve<void(ZDOID const &)>(&IZDOManager::DestroyZDO),
                          sol::resolve<void(ZDO::reference)>(&IZDOManager::DestroyZDO)),
            "instantiate",
            sol::overload(
                    sol::resolve<ZDO::reference(Prefab const &, Vector3f)>(&IZDOManager::Instantiate),
                    [](IZDOManager &self, std::string_view name, Vector3f pos) {
                        return self.Instantiate(get_stable_hash(name), pos);
                    },
                    sol::resolve<ZDO::reference(Hash, Vector3f)>(&IZDOManager::Instantiate)
                    //sol::resolve<ZDO (const ZDO)>(&IZDOManager::Instantiate)
                    )

    );
}

#endif
