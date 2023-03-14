#include "ModManager.h"
#include <easylogging++.h>
#include <robin_hood.h>
#include <yaml-cpp/yaml.h>

#include "VUtilsResource.h"
#include "VUtilsString.h"
#include "Peer.h"
#include "DataReader.h"
#include "DataWriter.h"
#include "Vector.h"
#include "Quaternion.h"
#include "ZDOID.h"
#include "ValhallaServer.h"
#include "NetSocket.h"
#include "ZDOManager.h"
#include "Method.h"
#include "objects/Ward.h"
#include "objects/Portal.h"
#include "objects/Player.h"
#include "RouteManager.h"
#include "NetManager.h"
#include "DungeonManager.h"
#include "DungeonGenerator.h"

auto MOD_MANAGER(std::make_unique<IModManager>());
IModManager* ModManager() {
    return MOD_MANAGER.get();
}

std::unique_ptr<IModManager::Mod> IModManager::LoadModInfo(const std::string& folderName) {

    YAML::Node loadNode;

    auto modPath = fs::path("mods") / folderName;
    auto modInfoPath = modPath / "modInfo.yml";

    if (auto opt = VUtils::Resource::ReadFileString(modInfoPath)) {
        loadNode = YAML::Load(opt.value());
    }
    else {
        throw std::runtime_error(std::string("unable to open ") + modInfoPath.string());
    }

    auto mod(std::make_unique<Mod>(
        loadNode["name"].as<std::string>(), 
        //sol::environment(m_state, sol::create, m_state.globals()),
        modPath / (loadNode["entry"].as<std::string>() + ".lua"))
    );

    //mod->m_env["_G"] = mod->m_env;

    mod->m_version = loadNode["version"].as<std::string>("");
    mod->m_apiVersion = loadNode["api-version"].as<std::string>("");
    mod->m_description = loadNode["description"].as<std::string>("");
    mod->m_authors = loadNode["authors"].as<std::list<std::string>>(std::list<std::string>());

    if (this->m_mods.contains(mod->m_name)) {
        throw std::runtime_error("mod with duplicate name");
    }

    return mod;
}

void IModManager::LoadAPI() {
    m_state.new_usertype<Vector3>("Vector3",
        sol::constructors<Vector3(), Vector3(float, float, float)>(),
        sol::meta_function::addition, &Vector3::operator+,
        sol::meta_function::subtraction, &Vector3::operator-,
        "x", &Vector3::x,
        "y", &Vector3::y,
        "z", &Vector3::z,
        "distance", sol::property(&Vector3::Distance),
        "magnitude", sol::property(&Vector3::Magnitude),
        "Normalize", &Vector3::Normalize,
        "normalized", sol::property(&Vector3::Normalized),
        "sqDistance", sol::property(&Vector3::SqDistance),
        "sqMagnitude", sol::property(&Vector3::SqMagnitude),
        "ZERO", sol::property([]() { return Vector3::ZERO; })
    );

    m_state.new_usertype<Vector2i>("Vector2i",
        sol::constructors<Vector2i(), Vector2i(int32_t, int32_t)>(),
        "x", &Vector2i::x,
        "y", &Vector2i::y,
        "distance", sol::property(&Vector2i::Distance),
        "magnitude", sol::property(&Vector2i::Magnitude),
        "Normalize", &Vector2i::Normalize,
        "normalized", sol::property(&Vector2i::Normalized),
        "sqDistance", sol::property(&Vector2i::SqDistance),
        "sqMagnitude", sol::property(&Vector2i::SqMagnitude),
        "ZERO", sol::property([]() { return Vector2i::ZERO; })
    );

    m_state.new_usertype<Quaternion>("Quaternion",
        sol::constructors<Quaternion(float, float, float, float)>(),
        "x", &Quaternion::x,
        "y", &Quaternion::y,
        "z", &Quaternion::z,
        "w", &Quaternion::w,
        "IDENTITY", sol::property([]() { return Quaternion::IDENTITY; })
    );

    m_state.new_usertype<ZDOID>("ZDOID",
        sol::constructors<ZDOID(OWNER_t userID, uint32_t id)>(),
        "uuid", &ZDOID::m_uuid,
        "id", &ZDOID::m_id,
        "NONE", sol::property([]() { return ZDOID::NONE; })
    );

    m_state.new_enum("DataType",
        "BYTES", DataType::BYTES,
        "STRING", DataType::STRING,
        "ZDOID", DataType::ZDOID,
        "VECTOR3", DataType::VECTOR3,
        "VECTOR2i", DataType::VECTOR2i,
        "QUATERNION", DataType::QUATERNION,
        "STRINGS", DataType::STRINGS,
        "BOOL", DataType::BOOL,
        "BYTE", DataType::INT8, "INT8", DataType::INT8,
        "SHORT", DataType::INT16, "INT16", DataType::INT16,
        "INT", DataType::INT32, "INT32", DataType::INT32, "HASH", DataType::INT32,
        "LONG", DataType::INT64, "INT64", DataType::INT64,
        "FLOAT", DataType::FLOAT,
        "DOUBLE", DataType::DOUBLE
    );

    m_state.new_usertype<DataWriter>("DataWriter",
        sol::constructors<DataWriter(BYTES_t&)>(),
        "provider", &DataWriter::m_provider,
        "pos", &DataWriter::m_pos,
        "Clear", &DataWriter::Clear,
        "Write", sol::overload(
            // templated functions are too complex for resolve
            // https://github.com/ThePhD/sol2/issues/664#issuecomment-396867392
            static_cast<void (DataWriter::*)(const BYTES_t&, size_t)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const BYTES_t&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const std::string&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const ZDOID&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const Vector3&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const Vector2i&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const Quaternion&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const std::vector<std::string>&)>(&DataWriter::Write),
            //[](DataWriter& self, std::vector<std::string> in) { self.Write(in); },
            [](DataWriter& self, DataType type, LUA_NUMBER val) {
                switch (type) {
                case DataType::INT8:
                    self.Write<int8_t>(val);
                    break;
                case DataType::INT16:
                    self.Write<int16_t>(val);
                    break;
                case DataType::INT32:
                    self.Write<int32_t>(val);
                    break;
                case DataType::INT64:
                    self.Write<int64_t>(val);
                    break;
                case DataType::FLOAT:
                    self.Write<float>(val);
                    break;
                case DataType::DOUBLE:
                    self.Write<double>(val);
                    break;
                default:
                    throw std::runtime_error("invalid DataType");
                    //return mod->Error("invalid DataType enum, got: " + std::to_string(std::to_underlying(type)));
                }
            }
        )
    );

    // Package read/write types
    m_state.new_usertype<DataReader>("DataReader",
        sol::constructors<DataReader(BYTES_t&)>(),
        "provider", &DataReader::m_provider,
        "pos", &DataReader::m_pos,
        // TODO make several descriptive reads, ie, ReadString, ReadInt, ReadVector3...
        //  instead of this verbose nightmare
        "Read", [](sol::this_state state, DataReader& self, DataType type) {
            switch (type) {
            case DataType::BYTES:
                return sol::make_object(state, self.Read<BYTES_t>());
            case DataType::STRING:
                return sol::make_object(state, self.Read<std::string>());
            case DataType::ZDOID:
                return sol::make_object(state, self.Read<ZDOID>());
            case DataType::VECTOR3:
                return sol::make_object(state, self.Read<Vector3>());
            case DataType::VECTOR2i:
                return sol::make_object(state, self.Read<Vector2i>());
            case DataType::QUATERNION:
                return sol::make_object(state, self.Read<Quaternion>());
            case DataType::STRINGS:
                return sol::make_object(state, self.Read<std::vector<std::string>>());
            case DataType::INT8:
                return sol::make_object(state, self.Read<int8_t>());
            case DataType::INT16:
                return sol::make_object(state, self.Read<int16_t>());
            case DataType::INT32:
                return sol::make_object(state, self.Read<int32_t>());
            case DataType::INT64:
                return sol::make_object(state, self.Read<int64_t>());
            case DataType::FLOAT:
                return sol::make_object(state, self.Read<float>());
            case DataType::DOUBLE:
                return sol::make_object(state, self.Read<double>());
            default:
                throw std::runtime_error("invalid DataType");
                //mod->Error("invalid DataType enum, got: " + std::to_string(std::to_underlying(type)));
            }
            //return sol::make_object(state, sol::nil);
        }
    );

    //env.new_usertype<IMethod<Peer*>>("IMethod",
    //    "Invoke", &IMethod<Peer*>::Invoke
    //);

    // https://sol2.readthedocs.io/en/latest/api/usertype.html#inheritance-example
    m_state.new_usertype<ISocket>("ISocket",
        "Close", &ISocket::Close,
        "connected", sol::property(&ISocket::Connected),
        "address", sol::property(&ISocket::GetAddress),
        "hostName", sol::property(&ISocket::GetHostName),
        "GetSendQueueSize", &ISocket::GetSendQueueSize
    );

    m_state.new_usertype<MethodSig>("MethodSig",
        sol::factories([](std::string name, sol::variadic_args types) { return MethodSig{ VUtils::String::GetStableHashCode(name), std::vector<DataType>(types.begin(), types.end()) }; })
    );

    m_state.new_enum("ChatMsgType",
        "WHISPER", ChatMsgType::Whisper,
        "NORMAL", ChatMsgType::Normal,
        "SHOUT", ChatMsgType::Shout,
        "PING", ChatMsgType::Ping
    );

    m_state.new_usertype<Peer>("Peer",
        // member fields
        "visibleOnMap", &Peer::m_visibleOnMap,
        "admin", &Peer::m_admin,
        "characterID", sol::readonly(&Peer::m_characterID),
        "name", sol::readonly(&Peer::m_name),
        "pos", &Peer::m_pos,
        "uuid", sol::readonly(&Peer::m_uuid),
        "socket", sol::property([](Peer& self) { return self.m_socket; }),
        "zdo", sol::property(&Peer::GetZDO),
        // member functions
        "Kick", sol::resolve<void()>(&Peer::Kick),
        "Kick", sol::resolve<void(std::string)>(&Peer::Kick),
        // message functions
        "ChatMessage", sol::overload(
            sol::resolve<void(const std::string&, ChatMsgType, const Vector3&, const std::string&, const std::string&)>(&Peer::ChatMessage),
            sol::resolve<void(const std::string&)>(&Peer::ChatMessage)
        ),
        "ConsoleMessage", &Peer::ConsoleMessage,
        "CornerMessage", &Peer::CornerMessage,
        "CenterMessage", &Peer::CenterMessage,
        // misc functions
        "Teleport", sol::overload(
            sol::resolve<void(const Vector3& pos, const Quaternion& rot, bool animation)>(&Peer::Teleport),
            sol::resolve<void(const Vector3& pos)>(&Peer::Teleport)
        ),
        "MoveTo", sol::overload(
            sol::resolve<void(const Vector3& pos, const Quaternion& rot)>(&Peer::MoveTo),
            sol::resolve<void(const Vector3& pos)>(&Peer::MoveTo)
        ),
        "Disconnect", &Peer::Disconnect,
        "InvokeSelf", sol::overload(
            sol::resolve<void (const std::string&, DataReader)>(&Peer::InvokeSelf),
            sol::resolve<void(HASH_t, DataReader)>(&Peer::InvokeSelf)),
            //static_cast<void (Peer::*)(const std::string&, DataReader)>(&Peer::InvokeSelf), //  &Peer::InvokeSelf,
            //static_cast<void (Peer::*)(HASH_t, DataReader)>(&Peer::InvokeSelf)), //  &Peer::InvokeSelf,
        //"Register", [](Peer& self, const MethodSig &repr, sol::function func) {
        //    self.Register(repr.m_hash, func, repr.m_types);
        //},

        // static_cast<void (DataWriter::*)(const BYTES_t&, size_t)>(&DataWriter::Write),
        "Register", static_cast<void (Peer::*)(MethodSig, sol::function)>(&Peer::Register),
        "Invoke", [](Peer& self, const MethodSig &repr, sol::variadic_args args) {
            if (args.size() != repr.m_types.size())
                throw std::runtime_error("incorrect number of args");

            BYTES_t bytes;
            DataWriter params(bytes);

            params.Write(repr.m_hash);

            for (int i = 0; i < args.size(); i++) {
                auto&& arg = args[i];
                auto argType = arg.get_type();
                DataType expectType = repr.m_types[i];

                if (argType == sol::type::number) {
                    switch (expectType) {
                    case DataType::INT8:
                        params.Write(arg.as<int8_t>());
                        break;
                    case DataType::INT16:
                        params.Write(arg.as<int16_t>());
                        break;
                    case DataType::INT32:
                        params.Write(arg.as<int32_t>());
                        break;
                    case DataType::INT64:
                        params.Write(arg.as<int64_t>());
                        break;
                    case DataType::FLOAT:
                        params.Write(arg.as<float>());
                        break;
                    case DataType::DOUBLE:
                        params.Write(arg.as<double>());
                        break;
                    default:
                        throw std::runtime_error("incorrect type at position (or bad DataFlag?)");
                    }
                }
                else if (argType == sol::type::string && expectType == DataType::STRING) {
                    params.Write(arg.as<std::string>());
                }
                else if (argType == sol::type::boolean && expectType == DataType::BOOL) {
                    params.Write(arg.as<bool>());
                }
                else if (arg.is<BYTES_t>() && expectType == DataType::BYTES) {
                    params.Write(arg.as<BYTES_t>());
                }
                else if (arg.is<ZDOID>() && expectType == DataType::ZDOID) {
                    params.Write(arg.as<ZDOID>());
                }
                else if (arg.is<Vector3>() && expectType == DataType::VECTOR3) {
                    params.Write(arg.as<Vector3>());
                }
                else if (arg.is<Vector2i>() && expectType == DataType::VECTOR2i) {
                    params.Write(arg.as<Vector2i>());
                }
                else if (arg.is<Quaternion>() && expectType == DataType::QUATERNION) {
                    params.Write(arg.as<Quaternion>());
                }
                else if (arg.is<std::vector<std::string>>() && expectType == DataType::STRINGS) {
                    params.Write(arg.as<std::vector<std::string>>());
                }
                else {
                    throw std::runtime_error("unsupported type, or incorrect type at position");
                }
            }

            self.m_socket->Send(std::move(bytes));
        }

        //"GetMethod", static_cast<IMethod<Peer*>* (Peer::*)(const std::string&)>(&Peer::GetMethod)
    );

    

    m_state.new_enum("PrefabFlag",
        "NONE", Prefab::Flags::None,
        "SCALE", Prefab::Flags::SyncInitialScale,
        "FAR", Prefab::Flags::Distant,
        "SESSIONED", Prefab::Flags::Sessioned,
        "PIECE", Prefab::Flags::Piece,
        "BED", Prefab::Flags::Bed,
        "DOOR", Prefab::Flags::Door,
        "CHAIR", Prefab::Flags::Chair,
        "SHIP", Prefab::Flags::Ship,
        "FISH", Prefab::Flags::Fish,
        "PLANT", Prefab::Flags::Plant,
        "ARMATURE", Prefab::Flags::ArmorStand,
        "ITEM", Prefab::Flags::ItemDrop,
        "PICKABLE", Prefab::Flags::Pickable,
        "PICKABLE_ITEM", Prefab::Flags::PickableItem,
        "COOKING", Prefab::Flags::CookingStation,
        "CRAFTING", Prefab::Flags::CraftingStation,
        "SMELTING", Prefab::Flags::Smelter,
        "BURNING", Prefab::Flags::Fireplace,
        "SUPPORT", Prefab::Flags::WearNTear,
        "BREAKABLE", Prefab::Flags::Destructible,
        "ATTACH", Prefab::Flags::ItemStand,
        "ANIMAL", Prefab::Flags::AnimalAI,
        "MONSTER", Prefab::Flags::MonsterAI,
        "TAME", Prefab::Flags::Tameable,
        "BREED", Prefab::Flags::Procreation,
        "ROCK", Prefab::Flags::MineRock,
        "ROCK_5", Prefab::Flags::MineRock5,
        "TREE", Prefab::Flags::TreeBase,
        "LOG", Prefab::Flags::TreeLog,
        "SFX", Prefab::Flags::SFX,
        "VFX", Prefab::Flags::VFX,
        "AOE", Prefab::Flags::AOE,
        "DUNGEON", Prefab::Flags::Dungeon,
        "PLAYER", Prefab::Flags::Player,
        "TOMBSTONE", Prefab::Flags::Tombstone
    );

    m_state.new_usertype<Prefab>("Prefab",
        sol::no_constructor,
        "name", &Prefab::m_name,
        "hash", &Prefab::m_hash,
        "FlagsPresent", &Prefab::FlagsPresent,
        "FlagsAbsent", &Prefab::FlagsAbsent
    );



    m_state["PrefabManager"] = PrefabManager();
    m_state.new_usertype<IPrefabManager>("IPrefabManager",
        "GetPrefab", sol::overload(
            sol::resolve<const Prefab*(const std::string&)>(&IPrefabManager::GetPrefab),
            sol::resolve<const Prefab* (HASH_t)>(&IPrefabManager::GetPrefab)
        )
    );

    //auto prefabApiTable = m_state["PrefabManager"].get_or_create<sol::table>();
    //prefabApiTable["GetPrefab"] = sol::overload(
    //    [](const std::string& name) { return PrefabManager()->GetPrefab(name); },
    //    [](HASH_t hash) { return PrefabManager()->GetPrefab(hash); }
    //);


    m_state.new_usertype<ZDO>("ZDO",
        sol::no_constructor,
        "id", sol::property(&ZDO::ID),
        "pos", sol::property(&ZDO::Position, &ZDO::SetPosition),
        "zone", sol::property(&ZDO::Sector),
        "rot", sol::property(&ZDO::Rotation, &ZDO::SetRotation),
        "prefab", sol::property(&ZDO::GetPrefab),
        "owner", sol::property(&ZDO::Owner, &ZDO::SetOwner),
        "IsOwner", &ZDO::IsOwner,
        "IsLocal", &ZDO::IsLocal,
        "HasOwner", &ZDO::HasOwner,
        "SetLocal", &ZDO::SetLocal,
        "Disown", &ZDO::Disown,
        "dataRev", sol::property([](ZDO& self) { return self.m_rev.m_dataRev; }),
        "ownerRev", sol::property([](ZDO& self) { return self.m_rev.m_ownerRev; }),
        "timeCreated", sol::property([](ZDO& self) { return self.m_rev.m_ticksCreated.count(); }), // hmm chrono...
        
        // Getters
        "GetFloat", sol::overload(
            sol::resolve<float(HASH_t, float) const>(&ZDO::GetFloat),
            sol::resolve<float(HASH_t) const>(&ZDO::GetFloat),
            sol::resolve<float(const std::string&, float) const>(&ZDO::GetFloat),
            sol::resolve<float(const std::string&) const>(&ZDO::GetFloat)
        ),
        "GetInt", sol::overload(
            sol::resolve<int32_t(HASH_t, int32_t) const>(&ZDO::GetInt),
            sol::resolve<int32_t(HASH_t) const>(&ZDO::GetInt),
            sol::resolve<int32_t(const std::string&, int32_t) const>(&ZDO::GetInt),
            sol::resolve<int32_t(const std::string&) const>(&ZDO::GetInt)
        ),
        "GetLong", sol::overload(
            sol::resolve<int64_t(HASH_t, int64_t) const>(&ZDO::GetLong),
            sol::resolve<int64_t(HASH_t) const>(&ZDO::GetLong),
            sol::resolve<int64_t(const std::string&, int64_t) const>(&ZDO::GetLong),
            sol::resolve<int64_t(const std::string&) const>(&ZDO::GetLong)
        ),
        "GetQuaternion", sol::overload(
            sol::resolve<const Quaternion&(HASH_t, const Quaternion&) const>(&ZDO::GetQuaternion),
            sol::resolve<const Quaternion&(HASH_t) const>(&ZDO::GetQuaternion),
            sol::resolve<const Quaternion&(const std::string&, const Quaternion&) const>(&ZDO::GetQuaternion),
            sol::resolve<const Quaternion&(const std::string&) const>(&ZDO::GetQuaternion)
        ),
        "GetVector3", sol::overload(
            sol::resolve<const Vector3& (HASH_t, const Vector3&) const>(&ZDO::GetVector3),
            sol::resolve<const Vector3& (HASH_t) const>(&ZDO::GetVector3),
            sol::resolve<const Vector3& (const std::string&, const Vector3&) const>(&ZDO::GetVector3),
            sol::resolve<const Vector3& (const std::string&) const>(&ZDO::GetVector3)
        ),
        "GetString", sol::overload(
            sol::resolve<const std::string& (HASH_t, const std::string&) const>(&ZDO::GetString),
            sol::resolve<const std::string& (HASH_t) const>(&ZDO::GetString),
            sol::resolve<const std::string& (const std::string&, const std::string&) const>(&ZDO::GetString),
            sol::resolve<const std::string& (const std::string&) const>(&ZDO::GetString)
        ),
        "GetBytes", sol::overload(
            sol::resolve<const BYTES_t* (HASH_t) const>(&ZDO::GetBytes),
            sol::resolve<const BYTES_t* (const std::string&) const>(&ZDO::GetBytes)
        ),
        "GetBool", sol::overload(
            sol::resolve<bool (HASH_t, bool) const>(&ZDO::GetBool),
            sol::resolve<bool (HASH_t) const>(&ZDO::GetBool),
            sol::resolve<bool (const std::string&, bool) const>(&ZDO::GetBool),
            sol::resolve<bool (const std::string&) const>(&ZDO::GetBool)
        ),
        "GetZDOID", sol::overload(
            //sol::resolve<ZDOID(HASH_t, const ZDOID&) const>(&ZDO::GetZDOID),
            //sol::resolve<ZDOID(HASH_t) const>(&ZDO::GetZDOID),
            sol::resolve<ZDOID(const std::string&, const ZDOID&) const>(&ZDO::GetZDOID),
            sol::resolve<ZDOID(const std::string&) const>(&ZDO::GetZDOID)
        ),

        "SetFloat", sol::overload(
            static_cast<void (ZDO::*)(HASH_t, const float&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const float&)>(&ZDO::Set)
        ),        
        "SetInt", sol::overload(
            static_cast<void (ZDO::*)(HASH_t, const int32_t&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const int32_t&)>(&ZDO::Set)
        ),
        "SetLong", sol::overload(
            static_cast<void (ZDO::*)(HASH_t, const int64_t&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const int64_t&)>(&ZDO::Set)
        ),
        "Set", sol::overload(
            // Quaternion
            static_cast<void (ZDO::*)(HASH_t, const Quaternion&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const Quaternion&)>(&ZDO::Set),
            // Vector3
            static_cast<void (ZDO::*)(HASH_t, const Vector3&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const Vector3&)>(&ZDO::Set),
            // std::string
            static_cast<void (ZDO::*)(HASH_t, const std::string&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const std::string&)>(&ZDO::Set),
            // bool
            static_cast<void (ZDO::*)(HASH_t, bool)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, bool)>(&ZDO::Set),
            // zdoid
            //static_cast<void (ZDO::*)(HASH_t, HASH_t, const ZDOID&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const ZDOID&)>(&ZDO::Set)
        ),
        "SetQuaternion", sol::overload(
            static_cast<void (ZDO::*)(HASH_t, const Quaternion&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const Quaternion&)>(&ZDO::Set)
        ),
        "SetVector3", sol::overload(
            static_cast<void (ZDO::*)(HASH_t, const Vector3&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const Vector3&)>(&ZDO::Set)
        ),
        "SetString", sol::overload(
            static_cast<void (ZDO::*)(HASH_t, const std::string&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const std::string&)>(&ZDO::Set)
        ),
        "SetBool", sol::overload(
            static_cast<void (ZDO::*)(HASH_t, bool)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, bool)>(&ZDO::Set)
        ),
        "SetZDOID", sol::overload(
            // zdoid
            //static_cast<void (ZDO::*)(HASH_t, HASH_t, const ZDOID&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const ZDOID&)>(&ZDO::Set)
        )
    );

    // setting meta functions
    // https://sol2.readthedocs.io/en/latest/api/metatable_key.html
    // 
    // TODO figure the number weirdness out...

    m_state.new_usertype<UInt64Wrapper>("UInt64",
        sol::constructors<UInt64Wrapper(), UInt64Wrapper(uint32_t, uint32_t)>(),
        sol::meta_function::addition, &UInt64Wrapper::__add,
        sol::meta_function::subtraction, &UInt64Wrapper::__sub,
        sol::meta_function::multiplication, &UInt64Wrapper::__mul,
        sol::meta_function::division, &UInt64Wrapper::__div,
        sol::meta_function::floor_division, &UInt64Wrapper::__divi
    );


    {
        // References will be unwrapped to pointers / visa-versa
        //  Pointers being dereferenced to a T& type is automatic if the function accepts a reference
        // https://sol2.readthedocs.io/en/latest/functions.html#functions-and-argument-passing

        auto viewsTable = m_state["Views"].get_or_create<sol::table>(); // idk a good namespace for this, 'shadow', 'wrapper', ...

        viewsTable.new_usertype<Ward>("Ward",
            sol::factories([](ZDO* zdo) { if (!zdo) throw std::runtime_error("null ZDO"); return Ward(*zdo); }),
            "creatorName", sol::property(&Ward::GetCreatorName, &Ward::SetCreatorName),
            "permitted", sol::property(&Ward::GetPermitted, &Ward::SetPermitted),
            "AddPermitted", &Ward::AddPermitted,
            "RemovePermitted", &Ward::RemovePermitted,
            "enabled", sol::property(&Ward::IsEnabled, &Ward::SetEnabled),
            "IsPermitted", &Ward::IsPermitted,
            "creator", sol::property([](Ward& self, Peer* peer) { if (!peer) throw std::runtime_error("null Peer"); return self.SetCreator(*peer); }),
            "IsAllowed", &Ward::IsAllowed
        );

        viewsTable.new_usertype<Portal>("Portal",
            sol::factories([](ZDO* zdo) { if (!zdo) throw std::runtime_error("null ZDO"); return Portal(*zdo); }),
            "tag", sol::property(&Portal::GetTag, &Portal::SetTag),
            "target", sol::property(&Portal::GetTarget, &Portal::SetTarget),
            "author", sol::property(&Portal::GetAuthor, &Portal::SetAuthor)
        );
    }

    //auto apiTable = m_state["Valhalla"].get_or_create<sol::table>();
    //apiTable["ServerVersion"] = SERVER_VERSION;
    //apiTable["ValheimVersion"] = VConstants::GAME;
    //apiTable["delta"] = sol::property([]() { return Valhalla()->Delta(); });
    //apiTable["id"] = sol::property([]() { return Valhalla()->ID(); });
    //apiTable["nanos"] = sol::property([]() { return Valhalla()->Nanos(); });
    //apiTable["time"] = sol::property([]() { return Valhalla()->Time(); });
    //apiTable["worldTicks"] = sol::property([]() { return Valhalla()->GetWorldTicks(); });
    //apiTable["worldTime"] = sol::property([]() { return Valhalla()->GetWorldTime(); }, [](double worldTime) { Valhalla()->SetWorldTime(worldTime); });
    //apiTable["worldTimeWrapped"] = sol::property([]() { return Valhalla()->GetTimeOfDay(); }, [](double worldTime) { Valhalla()->SetWorldTime(worldTime); });

    m_state.new_enum("TimeOfDay",
        "MORNING", TIME_MORNING,
        "DAY", TIME_DAY,
        "AFTERNOON", TIME_AFTERNOON,
        "NIGHT", TIME_NIGHT
    );

    //m_state.new_usertype<EventHandle>("EventHandle",
    //    "mod", &EventHandle::m_mod,
    //    //sol:meta_function_names()[sol::meta_function::call], &EventHandle::operator(),
    //    //sol::meta_function::call, &EventHandle::operator(),
    //    "priority", sol::readonly(&EventHandle::m_priority)
    //);
    //m_state["EventHandle"][sol::meta_function::call] = &EventHandle::operator();



    m_state["Valhalla"] = Valhalla();
    m_state.new_usertype<IValhalla>("IValhalla",
        // server members
        "version", sol::var(SERVER_VERSION),
        "delta", sol::property(&IValhalla::Delta),
        "id", sol::property(&IValhalla::ID),
        "nanos", sol::property(&IValhalla::Nanos),
        "time", sol::property(&IValhalla::Time),
        // world time functions
        "worldTime", sol::property(sol::resolve<WorldTime() const>(&IValhalla::GetWorldTime), &IValhalla::SetWorldTime),
        "worldTimeMultiplier", sol::property([](IValhalla& self) { return self.m_worldTimeMultiplier; }, [](IValhalla& self, double mul) { if (mul <= 0.001) throw std::runtime_error("multiplier too small"); self.m_worldTimeMultiplier = mul; }),
        "worldTicks", sol::property(&IValhalla::GetWorldTicks),
        "day", sol::property(sol::resolve<int() const>(&IValhalla::GetDay), &IValhalla::SetDay),        
        "timeOfDay", sol::property(sol::resolve<TimeOfDay() const>(&IValhalla::GetTimeOfDay), &IValhalla::SetTimeOfDay),
        "IsMorning", sol::resolve<bool() const>(&IValhalla::IsMorning),
        "IsDay", sol::resolve<bool() const>(&IValhalla::IsDay),
        "IsAfternoon", sol::resolve<bool() const>(&IValhalla::IsAfternoon),
        "IsNight", sol::resolve<bool() const>(&IValhalla::IsNight),
        "tomorrowMorning", sol::property(&IValhalla::GetTomorrowMorning),
        "tomorrow", sol::property(&IValhalla::GetTomorrowDay),
        "tomorrowAfternoon", sol::property(&IValhalla::GetTomorrowAfternoon),
        "tomorrowNight", sol::property(&IValhalla::GetTomorrowNight),

        "Subscribe", [this](IValhalla& self, sol::variadic_args args, sol::this_environment te) {
            sol::environment& env = te;

            HASH_t hash = 0;
            sol::function func;
            int priority = 0;

            // If priority is present (will be at end)
            const int offset = args[args.size() - 1].get_type() == sol::type::number ? 2 : 1;

            for (int i = 0; i < args.size(); i++) {
                auto&& arg = args[i];
                auto&& type = arg.get_type();

                if (i + offset < args.size()) {
                    if (type == sol::type::string)
                        hash ^= VUtils::String::GetStableHashCode(arg.as<std::string>());
                    else if (type == sol::type::number)
                        hash ^= arg.as<HASH_t>();
                    else {
                        throw std::runtime_error("initial params must be string or hash");
                    }
                }
                else {
                    if (i == args.size() - offset && type == sol::type::function) {
                        func = arg;
                    }
                    else if (offset == 2 && i == args.size() - 1 && type == sol::type::number) {
                        priority = arg;
                    }
                    else {
                        throw std::runtime_error("final param must be a function or priority");
                    }
                }
            }

            auto&& callbacks = m_callbacks[hash];
            
            Mod& mod = env["this"].get<sol::table>().as<Mod&>();
            
            callbacks.emplace_back(mod, func, priority);            
            callbacks.sort([](const EventHandle& a, const EventHandle& b) {
                    return a.m_priority < b.m_priority;
                }
            );
        }
    );

    

    // TODO turn managers into lua classes that can be indexed
    // but still retrieve with ZDOManager... class usertypes will be named by their class names, like IZDOManager...

    m_state["ZDOManager"] = ZDOManager();
    m_state.new_usertype<IZDOManager>("IZDOManager",
        "GetZDO", &IZDOManager::GetZDO,
        "SomeZDOs", sol::overload(
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const Vector3&, float, size_t, const std::function<bool(const ZDO&)>&)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const Vector3&, float, size_t)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const Vector3&, float, size_t, HASH_t prefabHash, Prefab::FLAG_t flagsPresent, Prefab::FLAG_t flagsAbsent)>(&IZDOManager::SomeZDOs),
            [](IZDOManager& self, const Vector3& pos, float radius, size_t max, const std::string& name) { return self.SomeZDOs(pos, radius, max, VUtils::String::GetStableHashCode(name), Prefab::Flags::None, Prefab::Flags::None); },

            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, size_t, const std::function<bool(const ZDO&)>&)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, size_t)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, size_t, HASH_t, Prefab::FLAG_t, Prefab::FLAG_t)>(&IZDOManager::SomeZDOs),
            [](IZDOManager& self, const ZoneID& zone, size_t max, const std::string& name) { return self.SomeZDOs(zone, max, VUtils::String::GetStableHashCode(name), Prefab::Flags::None, Prefab::Flags::None); },

            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, size_t, const Vector3&, float)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, size_t, const Vector3&, float, HASH_t, Prefab::FLAG_t, Prefab::FLAG_t)>(&IZDOManager::SomeZDOs),
            [](IZDOManager& self, const ZoneID& zone, size_t max, const Vector3& pos, float radius, const std::string& name) { return self.SomeZDOs(zone, max, pos, radius, VUtils::String::GetStableHashCode(name), Prefab::Flags::None, Prefab::Flags::None); }
        ),
        "GetZDOs", sol::overload(
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(HASH_t)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, const std::string& name) { return self.GetZDOs(VUtils::String::GetStableHashCode(name)); },

            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const Vector3&, float, const std::function<bool(const ZDO&)>&)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const Vector3&, float)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const Vector3&, float, HASH_t, Prefab::FLAG_t, Prefab::FLAG_t)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, const Vector3& pos, float radius, const std::string& name) { return self.GetZDOs(pos, radius, VUtils::String::GetStableHashCode(name), Prefab::Flags::None, Prefab::Flags::None); },

            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, const std::function<bool(const ZDO&)>&)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, HASH_t, Prefab::FLAG_t, Prefab::FLAG_t)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, const ZoneID& zone, const std::string& name) { return self.GetZDOs(zone, VUtils::String::GetStableHashCode(name), Prefab::Flags::None, Prefab::Flags::None); },

            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, const Vector3&, float)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, const Vector3&, float, HASH_t, Prefab::FLAG_t, Prefab::FLAG_t)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, const ZoneID& zone, const Vector3& pos, float radius, const std::string& name) { return self.GetZDOs(zone, pos, radius, VUtils::String::GetStableHashCode(name), Prefab::Flags::None, Prefab::Flags::None); }
        ),
        "AnyZDO", sol::overload(
            sol::resolve<ZDO* (const Vector3&, float, HASH_t, Prefab::FLAG_t, Prefab::FLAG_t)>(&IZDOManager::AnyZDO),
            [](IZDOManager& self, const Vector3& pos, float radius, const std::string& name) { return self.AnyZDO(pos, radius, VUtils::String::GetStableHashCode(name), Prefab::Flags::None, Prefab::Flags::None); },

            sol::resolve<ZDO* (const ZoneID&, HASH_t, Prefab::FLAG_t, Prefab::FLAG_t)>(&IZDOManager::AnyZDO),
            [](IZDOManager& self, const ZoneID& zone, const std::string& name) { return self.AnyZDO(zone, VUtils::String::GetStableHashCode(name), Prefab::Flags::None, Prefab::Flags::None); }
        ),
        "NearestZDO", sol::overload(
            sol::resolve<ZDO* (const Vector3&, float, const std::function<bool(const ZDO&)>&)>(&IZDOManager::NearestZDO),
            sol::resolve<ZDO* (const Vector3&, float, HASH_t, Prefab::FLAG_t, Prefab::FLAG_t)>(&IZDOManager::NearestZDO),
            [](IZDOManager& self, const Vector3& pos, float radius, const std::string& name) { return self.NearestZDO(pos, radius, VUtils::String::GetStableHashCode(name), Prefab::Flags::None, Prefab::Flags::None); }
        ),
        "ForceSendZDO", [](IZDOManager& self, const ZDOID& zdoid) { self.ForceSendZDO(zdoid); },
        "DestroyZDO", sol::overload(
            [](IZDOManager& self, ZDO* zdo, bool immediate) { if (!zdo) throw std::runtime_error("null zdo"); self.DestroyZDO(*zdo, immediate); },
            [](IZDOManager& self, ZDO* zdo) { if (!zdo) throw std::runtime_error("null zdo"); self.DestroyZDO(*zdo); }
        )
    );



    m_state["NetManager"] = NetManager();
    m_state.new_usertype<INetManager>("INetManager",
        "GetPeer", sol::overload(
            sol::resolve<Peer*(OWNER_t)>(&INetManager::GetPeer),
            sol::resolve<Peer* (const std::string&)>(&INetManager::GetPeer)
        ),
        "peers", sol::readonly(&INetManager::m_peers)
    );



    m_state["ModManager"] = ModManager();
    m_state.new_usertype<IModManager>("IModManager",
        "GetMod", [](IModManager& self, const std::string& name) {
            auto&& find = self.m_mods.find(name);
            if (find != self.m_mods.end())
                return find->second.get();
            return static_cast<Mod*>(nullptr);
        }
    );



    m_state.new_usertype<Dungeon>("Dungeon",
        sol::no_constructor
        //"Generate", sol::resolve<void(const Vector3& pos, const Quaternion& rot) const>(&Dungeon::Generate)
    );

    m_state["DungeonManager"] = DungeonManager();
    m_state.new_usertype<IDungeonManager>("IDungeonManager",
        "GetDungeon", [](IDungeonManager& self, const std::string& name) { return self.GetDungeon(VUtils::String::GetStableHashCode(name)); },
        "Generate", [](IDungeonManager& self, const Dungeon* dungeon, const Vector3& pos, const Quaternion& rot) { if (!dungeon) throw std::runtime_error("null dungeon"); self.Generate(*dungeon, pos, rot); }
    );



    m_state.new_usertype<IZoneManager::Feature::Instance>("FeatureInstance",
        "pos", sol::property([](IZoneManager::Feature::Instance& self) { return self.m_pos; })
    );

    m_state["ZoneManager"] = ZoneManager();
    m_state.new_usertype<IZoneManager>("IZoneManager",
        "GetNearestFeature", &IZoneManager::GetNearestFeature,
        "RegenerateZone", &IZoneManager::RegenerateZone,
        "WorldToZonePos", &IZoneManager::WorldToZonePos,
        "ZoneToWorldPos", &IZoneManager::ZoneToWorldPos,
        "globalKeys", sol::property(&IZoneManager::GlobalKeys)
    );



    // TODO use properties for immutability
    m_state.new_usertype<Mod>("Mod",
        "name", &Mod::m_name,
        "version", &Mod::m_version,
        "apiVersion", &Mod::m_apiVersion,
        "description", &Mod::m_description,
        "authors", &Mod::m_authors
    );



    m_state.new_usertype<IRouteManager::Data>("RouteData",
        "sender", &IRouteManager::Data::m_sender,
        "target", &IRouteManager::Data::m_target,
        "targetZDO", &IRouteManager::Data::m_targetZDO,
        "method", &IRouteManager::Data::m_method,
        "params", &IRouteManager::Data::m_params
    );

    {
        auto eventTable = m_state["event"].get_or_create<sol::table>();

        eventTable["Cancel"] = [this]() { m_eventStatus = EventStatus::CANCEL; };
        eventTable["SetCancelled"] = [this](bool c) { m_eventStatus = c ? EventStatus::CANCEL : EventStatus::PROCEED; };
        eventTable["cancelled"] = sol::property([this]() { return m_eventStatus == EventStatus::CANCEL; });
        eventTable["Unsubscribe"] = [this]() { m_eventStatus = EventStatus::UNSUBSCRIBE; };
    }

    m_state["print"] = [](sol::this_state ts, sol::variadic_args args) {
        sol::state_view state = ts;

        auto&& tostring(state["tostring"]);

        std::string s;
        int idx = 0;
        for (auto&& arg : args) {
            if (idx++ > 0)
                s += " ";
            s += tostring(arg);
        }

        //LOG(INFO) << "[" << mod->m_name << "] " << s;
        LOG(INFO) << "[mod] " << s;
    };

    {
        auto utilsTable = m_state["VUtils"].get_or_create<sol::table>();

        utilsTable["Compress"] = sol::overload(
            sol::resolve<std::optional<BYTES_t>(const BYTES_t&)>(VUtils::CompressGz),
            sol::resolve<std::optional<BYTES_t>(const BYTES_t&, int)>(VUtils::CompressGz)
        );
        utilsTable["Decompress"] = sol::resolve<std::optional<BYTES_t>(const BYTES_t&)>(VUtils::Decompress);
        utilsTable["Decompress"] = sol::resolve<std::optional<BYTES_t>(const BYTES_t&)>(VUtils::Decompress);
        utilsTable["CreateBytes"] = []() { return BYTES_t(); };

        {
            auto stringUtilsTable = utilsTable["String"].get_or_create<sol::table>();

            stringUtilsTable["GetStableHashCode"] = sol::resolve<HASH_t(const std::string&)>(VUtils::String::GetStableHashCode);
        }

        {
            auto resourceUtilsTable = utilsTable["Resource"].get_or_create<sol::table>();

            resourceUtilsTable["ReadFileBytes"] = VUtils::Resource::ReadFileBytes;
            resourceUtilsTable["ReadFileString"] = VUtils::Resource::ReadFileString;
            resourceUtilsTable["ReadFileLines"] = sol::resolve<std::optional<std::vector<std::string>>(const fs::path&)>(VUtils::Resource::ReadFileLines);
            resourceUtilsTable["WriteFileBytes"] = sol::resolve<bool(const fs::path&, const BYTES_t&)>(VUtils::Resource::WriteFileBytes);
            resourceUtilsTable["WriteFileString"] = VUtils::Resource::WriteFileString;
            resourceUtilsTable["WriteFileLines"] = sol::resolve<bool(const fs::path&, const std::vector<std::string>&)>(VUtils::Resource::WriteFileLines);
        }
    }
}

void IModManager::LoadMod(Mod& mod) {
    auto path(mod.m_entry);
    if (auto opt = VUtils::Resource::ReadFileString(path)) {
        auto&& env = mod.m_env;
        env = sol::environment(m_state, sol::create, m_state.globals());

        env["_G"] = env;
        env["this"] = mod;

        {
            //auto configTable = thisTable["config"].get_or_create<sol::table>();

            // TODO use yamlcpp for config...
        }

        m_state.safe_script(opt.value(), env);
    }
    else
        throw std::runtime_error(std::string("unable to open file ") + path.string());
}



inline void my_panic(sol::optional<std::string> maybe_msg) {
    LOG(ERROR) << "Lua is in a panic state and will now abort() the application";
    if (maybe_msg) {
        const std::string& msg = maybe_msg.value();
        LOG(ERROR) << "\terror message: " << msg;
    }
    // When this function exits, Lua will exhibit default behavior and abort()
}

int my_exception_handler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
    // L is the lua state, which you can wrap in a state_view if necessary
    // maybe_exception will contain exception, if it exists
    // description will either be the what() of the exception or a description saying that we hit the general-case catch(...)
    LOG(ERROR) << "An exception occurred in a function, here's what it says ";
    if (maybe_exception) {
        LOG(ERROR) << "(straight from the exception): ";
        const std::exception& ex = *maybe_exception;
        LOG(ERROR) << ex.what();
    }
    else {
        LOG(ERROR) << "(from the description parameter): ";
        LOG(ERROR) << description;
    }

    // you must push 1 element onto the stack to be
    // transported through as the error object in Lua
    // note that Lua -- and 99.5% of all Lua users and libraries -- expects a string
    // so we push a single string (in our case, the description of the error)
    return sol::stack::push(L, description);
}

void IModManager::Init() {
    LOG(INFO) << "Initializing ModManager";

    //m_state.set_panic(sol::c_call<decltype(&my_panic), &my_panic>);

    m_state.set_exception_handler(&my_exception_handler);

    //sol::main_thread()


    m_state.open_libraries(
        sol::lib::base,
        sol::lib::debug,
        sol::lib::io, // override
        sol::lib::math,
        sol::lib::package, // override
        sol::lib::string,
        sol::lib::table,
        sol::lib::utf8
    );

    LoadAPI();

    for (const auto& dir
        : fs::directory_iterator("mods")) {

        try {
            if (dir.exists() && dir.is_directory()) {
                auto&& dirname = dir.path().filename().string();

                if (dirname.starts_with("--"))
                    continue;

                auto mod = LoadModInfo(dirname);
                LoadMod(*mod.get());

                LOG(INFO) << "Loaded mod '" << mod->m_name << "'";

                m_mods.insert({ mod->m_name, std::move(mod) });
            }
        }
        catch (const std::exception& e) {
            LOG(ERROR) << "Failed to load mod: " << e.what() << " (" << dir.path().c_str() << ")";
        }
    }

    LOG(INFO) << "Loaded " << m_mods.size() << " mods";

    CallEvent("Enable");
}

void IModManager::Uninit() {
    CallEvent("Disable");
    m_callbacks.clear();
    m_mods.clear();
}

void IModManager::Update() {
    ModManager()->CallEvent(EVENT_HASH_Update);

    /*
    if (m_reload) {
        // first release all callbacks associated with the mod
        for (auto&& itr = m_callbacks.begin(); itr != m_callbacks.end();) {
            auto&& callbacks = itr->second;
            for (auto&& itr1 = callbacks.begin(); itr1 != callbacks.end();) {
                if (itr1->m_mod.get().m_reload) {
                    itr1 = callbacks.erase(itr1);
                }
                else
                    ++itr;
            }

            // Pop callback set for tidy
            if (callbacks.empty())
                itr = m_callbacks.erase(itr);
            else
                ++itr;
        }

        for (auto&& pair : m_mods) {
            auto&& mod = *pair.second.get();
            if (mod.m_reload) {
                LOG(INFO) << "Reloading mod " << mod.m_name;

                for (auto&& pair : NetManager()->GetPeers()) {
                    auto&& peer = pair.second;
                    for (auto&& pair1 : peer->m_methods) {
                        auto&& method = dynamic_cast<MethodImplLua<Peer*>*>(pair1.second.get());
                        //if (method)
                            //method->m_func = 
                    }
                    //if (auto method = peer->GetMethod()
                }

                mod.m_env.reset();
                LoadMod(mod);
                mod.m_reload = false;
            }
        }

        m_state.collect_gc();

        m_reload = false;
    }*/
}
