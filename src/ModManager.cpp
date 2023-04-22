#include <easylogging++.h>
#include <yaml-cpp/yaml.h>
#include <toml++/toml.h>

#include "ModManager.h"
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
#include "RouteManager.h"
#include "NetManager.h"
#include "DungeonManager.h"
#include "DungeonGenerator.h"

auto MOD_MANAGER(std::make_unique<IModManager>());
IModManager* ModManager() {
    return MOD_MANAGER.get();
}

IModManager::Mod& IModManager::LoadModInfo(const std::string& folderName) {
    YAML::Node loadNode;

    auto modPath = fs::path("mods") / folderName;
    auto modInfoPath = modPath / "modInfo.yml";

    if (auto opt = VUtils::Resource::ReadFile<std::string>(modInfoPath)) {
        loadNode = YAML::Load(opt.value());
    }
    else {
        throw std::runtime_error(std::string("unable to open ") + modInfoPath.string());
    }

    auto name = loadNode["name"].as<std::string>();

    auto &&insert = this->m_mods.insert({ name, std::make_unique<Mod>(
        loadNode["name"].as<std::string>(),
        modPath / (loadNode["entry"].as<std::string>() + ".lua"))
    });

    if (!insert.second)
        throw std::runtime_error("Mod " + name + " already loaded");

    auto&& mod = insert.first->second;

    mod->m_version = loadNode["version"].as<std::string>("");
    mod->m_apiVersion = loadNode["api-version"].as<std::string>("");
    mod->m_description = loadNode["description"].as<std::string>("");
    mod->m_authors = loadNode["authors"].as<std::list<std::string>>(std::list<std::string>());
    
    return *mod;
}

int LoadFileRequire(lua_State* L) {
    std::string path = sol::stack::get<std::string>(L);

    // first look in the sub mod dir
    //  ./mods/MyExampleMod/
    if (auto opt = VUtils::Resource::ReadFile<std::string>(std::string("./mods/") + path + ".lua"))
        luaL_loadbuffer(L, opt.value().data(), opt.value().size(), path.c_str());
    else {
        sol::stack::push(L, "Module '" + path + "' not found");
    }

    return 1;
}

void IModManager::LoadAPI() {
    
    m_state.new_usertype<Vector3f>("Vector3f",
        sol::constructors<Vector3f(), Vector3f(float, float, float)>(),
        "ZERO", sol::property(&Vector3f::Zero),
        "x", &Vector3f::x,
        "y", &Vector3f::y,
        "z", &Vector3f::z,
        "magnitude", sol::property(&Vector3f::Magnitude),
        "sqMagnitude", sol::property(&Vector3f::SqMagnitude),
        "normal", sol::property(&Vector3f::Normal),
        "Distance", &Vector3f::Distance,
        "SqDistance", &Vector3f::SqDistance,
        "Dot", &Vector3f::Dot,
        "Cross", &Vector3f::Cross,
        sol::meta_function::addition, &Vector3f::operator+,
        sol::meta_function::subtraction, sol::resolve<Vector3f(const Vector3f&) const>(&Vector3f::operator-),
        sol::meta_function::unary_minus, sol::resolve<Vector3f() const>(&Vector3f::operator-),
        sol::meta_function::multiplication, sol::resolve<Vector3f(const Vector3f&) const>(&Vector3f::operator*),
        sol::meta_function::division, sol::resolve<Vector3f(const Vector3f&) const>(&Vector3f::operator/),
        sol::meta_function::equal_to, &Vector3f::operator==
    );

    m_state.new_usertype<Vector2f>("Vector2f",
        sol::constructors<Vector2f(), Vector2f(float, float)>(),
        "ZERO", sol::property(&Vector2f::Zero),
        "x", &Vector2f::x,
        "y", &Vector2f::y,
        "magnitude", sol::property(&Vector2f::Magnitude),
        "sqMagnitude", sol::property(&Vector2f::SqMagnitude),
        "normal", sol::property(&Vector2f::Normal),
        "Distance", &Vector2f::Distance,
        "SqDistance", &Vector2f::SqDistance,
        "Dot", &Vector2f::Dot,
        sol::meta_function::addition, &Vector2f::operator+,
        sol::meta_function::subtraction, sol::resolve<Vector2f(const Vector2f&) const>(&Vector2f::operator-),
        sol::meta_function::unary_minus, sol::resolve<Vector2f() const>(&Vector2f::operator-),
        sol::meta_function::multiplication, sol::resolve<Vector2f(const Vector2f&) const>(&Vector2f::operator*),
        sol::meta_function::division, sol::resolve<Vector2f(const Vector2f&) const>(&Vector2f::operator/),
        sol::meta_function::equal_to, &Vector2f::operator==
    );

    m_state.new_usertype<Vector2i>("Vector2i",
        sol::constructors<Vector2i(), Vector2i(int32_t, int32_t)>(),
        "ZERO", sol::property(&Vector2i::Zero),
        "x", &Vector2i::x,
        "y", &Vector2i::y,
        "magnitude", sol::property(&Vector2i::Magnitude),
        "sqMagnitude", sol::property(&Vector2i::SqMagnitude),
        "normal", sol::property(&Vector2i::Normal),
        "Distance", &Vector2i::Distance,
        "SqDistance", &Vector2i::SqDistance,
        "Dot", &Vector2i::Dot,
        sol::meta_function::addition, &Vector2i::operator+,
        sol::meta_function::subtraction, sol::resolve<Vector2i(const Vector2i&) const>(&Vector2i::operator-),
        sol::meta_function::unary_minus, sol::resolve<Vector2i() const>(&Vector2i::operator-),
        sol::meta_function::multiplication, sol::resolve<Vector2i(const Vector2i&) const>(&Vector2i::operator*),
        sol::meta_function::division, sol::resolve<Vector2i(const Vector2i&) const>(&Vector2i::operator/),
        sol::meta_function::equal_to, &Vector2i::operator==
    );

    m_state.new_usertype<Quaternion>("Quaternion",
        sol::constructors<Quaternion(), Quaternion(float, float, float, float)>(),
        "IDENTITY", sol::property([]() { return Quaternion::IDENTITY; }),
        "x", &Quaternion::x,
        "y", &Quaternion::y,
        "z", &Quaternion::z,
        "w", &Quaternion::w,
        sol::meta_function::multiplication, sol::resolve<Quaternion(const Quaternion&) const>(&Quaternion::operator*)
    );

    m_state.new_usertype<ZDOID>("ZDOID",
        //sol::constructors<ZDOID(OWNER_t userID, uint32_t id)>(),
        sol::factories([](const Int64Wrapper& uuid, uint32_t id) { return ZDOID(uuid, id); }),
        "NONE", sol::property([]() { return ZDOID::NONE; }),
        "uuid", sol::property([](ZDOID& self) { return (Int64Wrapper)self.GetOwner(); }, [](ZDOID& self, const Int64Wrapper& value) { self.SetOwner(value); }),
        "id", sol::property(&ZDOID::GetUID, &ZDOID::SetUID)
    );

    m_state.new_enum("Type",
        "BOOL", Type::BOOL,

        "STRING", Type::STRING,
        "STRINGS", Type::STRINGS,

        "BYTES", Type::BYTES,
        
        "ZDOID", Type::ZDOID,
        "VECTOR3f", Type::VECTOR3f,
        "VECTOR2i", Type::VECTOR2i,
        "QUATERNION", Type::QUATERNION,
        
        "INT8", Type::INT8,
        "INT16", Type::INT16, "SHORT", Type::INT16,
        "INT32", Type::INT32, "INT", Type::INT32, "HASH", Type::INT32,
        "INT64", Type::INT64, "LONG", Type::INT64,

        "UINT8", Type::UINT8, "BYTE", Type::UINT8,
        "UINT16", Type::UINT16, "USHORT", Type::UINT16,
        "UINT32", Type::UINT32, "UINT", Type::UINT32,
        "UINT64", Type::UINT64, "ULONG", Type::UINT64,

        "FLOAT", Type::FLOAT,
        "DOUBLE", Type::DOUBLE,

        "CHAR", Type::CHAR
    );

    m_state.new_usertype<BYTES_t>("Bytes",
        sol::constructors<BYTES_t(), BYTES_t(const BYTES_t&)>(),
        "Assign", [](BYTES_t& self, const BYTES_t& other) { self = other; },
        "Move", [](BYTES_t& self, BYTES_t& other) { self = std::move(other); },
        "Swap", [](BYTES_t& self, BYTES_t& other) { self.swap(other); }
    );

    m_state.new_usertype<UserProfile>("UserProfile",
        sol::constructors<UserProfile(const std::string&, const std::string&, const std::string&)>(),
        "name", &UserProfile::m_name,
        "ign", &UserProfile::m_gamerTag,
        "nid", &UserProfile::m_networkUserId
    );

    m_state.new_usertype<DataWriter>("DataWriter",
        sol::constructors<DataWriter(BYTES_t&)>(),

        //"ToReader", &DataWriter::ToReader,
        "buf", &DataWriter::m_buf,
        "pos", sol::property(&DataWriter::Position, &DataWriter::SetPos), //& DataWriter::m_pos,

        "Clear", &DataWriter::Clear,

        "Write", sol::overload(
            // templated functions are too complex for resolve
            // https://github.com/ThePhD/sol2/issues/664#issuecomment-396867392
            static_cast<void (DataWriter::*)(bool)>(&DataWriter::Write),

            static_cast<void (DataWriter::*)(std::string_view)>(&DataWriter::Write),
            //static_cast<void (DataWriter::*)(const std::vector<std::string>&)>(&DataWriter::Write),

            static_cast<void (DataWriter::*)(const BYTES_t&)>(&DataWriter::Write),
            
            static_cast<void (DataWriter::*)(const ZDOID&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const Vector3f&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const Vector2i&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const Quaternion&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const UserProfile&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const Int64Wrapper&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const UInt64Wrapper&)>(&DataWriter::Write)
        ),

        "WriteInt8", static_cast<void (DataWriter::*)(int8_t)>(&DataWriter::Write),
        "WriteInt16", static_cast<void (DataWriter::*)(int16_t)>(&DataWriter::Write),
        "WriteInt32", static_cast<void (DataWriter::*)(int32_t)>(&DataWriter::Write),
        "WriteInt64", static_cast<void (DataWriter::*)(const Int64Wrapper&)>(&DataWriter::Write),

        "WriteUInt8", static_cast<void (DataWriter::*)(uint8_t)>(&DataWriter::Write),
        "WriteUInt16", static_cast<void (DataWriter::*)(uint16_t)>(&DataWriter::Write),
        "WriteUInt32", static_cast<void (DataWriter::*)(uint32_t)>(&DataWriter::Write),
        "WriteUInt64", static_cast<void (DataWriter::*)(const UInt64Wrapper&)>(&DataWriter::Write),

        "WriteFloat", static_cast<void (DataWriter::*)(float)>(&DataWriter::Write),
        "WriteDouble", static_cast<void (DataWriter::*)(double)>(&DataWriter::Write),

        "WriteChar", static_cast<void (DataWriter::*)(char16_t)>(&DataWriter::Write),

        "Serialize", sol::overload(
            sol::resolve<void(IModManager::Type, sol::object)>(&DataWriter::SerializeOneLua),
            [](DataWriter& self, const IModManager::Types& types, sol::variadic_args args) { 
                return self.SerializeLua(types, sol::variadic_results(args.begin(), args.end()));
            }
            //sol::resolve<sol::variadic_results(const IModManager::Types&, const sol::variadic_results&)>(&DataWriter::SerializeLuaImpl)
        )
    );

    // Package read/write types
    m_state.new_usertype<DataReader>("DataReader",
        sol::constructors<DataReader(BYTES_t&)>(),

        //"ToWriter", &DataReader::ToWriter,
        "buf", &DataReader::m_buf,
        "pos", sol::property(&DataReader::Position, &DataReader::SetPos), //& DataWriter::m_pos,

        "ReadBool", &DataReader::ReadBool,

        "ReadString", &DataReader::ReadString,
        "ReadStrings", &DataReader::ReadStrings,

        "ReadBytes", &DataReader::ReadBytes,

        "ReadZDOID", &DataReader::ReadZDOID,
        "ReadVector3f", &DataReader::ReadVector3f,
        "ReadVector2i", &DataReader::ReadVector2i,
        "ReadQuaternion", &DataReader::ReadQuaternion,
        "ReadProfile", &DataReader::ReadProfile,

        "ReadInt8", &DataReader::ReadInt8,
        "ReadInt16", &DataReader::ReadInt16,
        "ReadInt32", &DataReader::ReadInt32,
        "ReadInt64", &DataReader::ReadInt64Wrapper,

        "ReadUInt8", &DataReader::ReadUInt8,
        "ReadUInt16", &DataReader::ReadUInt16,
        "ReadUInt32", &DataReader::ReadUInt32,
        "ReadUInt64", &DataReader::ReadUInt64Wrapper,

        "ReadFloat", &DataReader::ReadFloat,
        "ReadDouble", &DataReader::ReadDouble,
                
        "ReadChar", &DataReader::ReadChar,

        "Deserialize", [](DataReader& self, sol::state_view state, sol::variadic_args args) { 
            return self.DeserializeLua(state, IModManager::Types(args.begin(), args.end()));
        }
        
    );

    //m_state.new_usertype<IMethod<Peer*>>("IMethodPeer",
    //    "Invoke", &IMethod<Peer*>::Invoke
    //);

    m_state.new_usertype<ISocket>("Socket",
        "Close", &ISocket::Close,
        "connected", sol::property(&ISocket::Connected),
        "address", sol::property(&ISocket::GetAddress),
        "host", sol::property(&ISocket::GetHostName),
        "sendQueueSize", sol::property(&ISocket::GetSendQueueSize)
    );

    m_state.new_usertype<MethodSig>("MethodSig",
        sol::factories([](const std::string &name, sol::variadic_args types) { return MethodSig{ VUtils::String::GetStableHashCode(name), IModManager::Types(types.begin(), types.end()) }; })
    );

    m_state.new_enum("ChatMsgType",
        "WHISPER", ChatMsgType::Whisper,
        "NORMAL", ChatMsgType::Normal,
        "SHOUT", ChatMsgType::Shout,
        "PING", ChatMsgType::Ping
    );

    //m_state.new_usertype<NetRpc>("RpcClient",
    //    "socket", sol::readonly(&Peer::m_socket)
    //    //"Register", sol::
    //
    //    );

    m_state.new_usertype<Peer>("Peer",
        // member fields
        "visibleOnMap", &Peer::m_visibleOnMap,
        "admin", &Peer::m_admin,
        "characterID", sol::property([](Peer& self) { return self.m_characterID; }), // return copy
        "name", sol::readonly(&Peer::m_name), // strings are immutable in Lua similarly to Java
        "pos", &Peer::m_pos,
        "uuid", sol::property([](Peer& self) { return Int64Wrapper(self.m_uuid); }),
        "socket", sol::readonly(&Peer::m_socket),
        "zdo", sol::property(&Peer::GetZDO),
        // member functions
        "Kick", sol::resolve<void ()>(&Peer::Kick),
        // message functions
        "ChatMessage", sol::overload(
            // TODO fix this
            //sol::resolve<void(const std::string&, ChatMsgType, const Vector3f&, const std::string&, const std::string&)>(&Peer::ChatMessage),
            sol::resolve<void (const std::string&)>(&Peer::ChatMessage)
        ),
        "ConsoleMessage", &Peer::ConsoleMessage,
        "CornerMessage", &Peer::CornerMessage,
        "CenterMessage", &Peer::CenterMessage,
        // misc functions
        "Teleport", sol::overload(
            sol::resolve<void (const Vector3f& pos, const Quaternion& rot, bool animation)>(&Peer::Teleport),
            sol::resolve<void (const Vector3f& pos)>(&Peer::Teleport)
        ),
        //"MoveTo", sol::overload(
        //    sol::resolve<void(const Vector3f& pos, const Quaternion& rot)>(&Peer::MoveTo),
        //    sol::resolve<void(const Vector3f& pos)>(&Peer::MoveTo)
        //),
        "Disconnect", &Peer::Disconnect,
        "InvokeSelf", sol::overload(
            sol::resolve<bool (HASH_t, DataReader&)>(&Peer::InternalInvoke),
            sol::resolve<bool (const std::string&, DataReader&)>(&Peer::InternalInvoke)
        ),

            //static_cast<void (Peer::*)(const std::string&, DataReader)>(&Peer::InvokeSelf), //  &Peer::InvokeSelf,
            //static_cast<void (Peer::*)(HASH_t, DataReader)>(&Peer::InvokeSelf)), //  &Peer::InvokeSelf,
        //"Register", [](Peer& self, const MethodSig &repr, sol::function func) {
        //    self.Register(repr.m_hash, func, repr.m_types);
        //},

        // static_cast<void (DataWriter::*)(const BYTES_t&, size_t)>(&DataWriter::Write),
        "Register", &Peer::RegisterLua,
        //"Register", [](Peer& self, const IModManager::MethodSig& sig, const sol::function& func, sol::this_environment te) { 
        //    sol::environment& env = te;
        //    Mod& mod = env["this"].get<sol::table>().as<Mod&>();
        //    self.RegisterLua(sig, func, &mod); 
        //},
        "Invoke", &Peer::InvokeLua,
        "RouteView", &Peer::RouteViewLua,
        "Route", &Peer::RouteLua
        //sol::overload(
        //    sol::resolve<void(const ZDOID&, const IModManager::MethodSig&, const sol::variadic_args&)>(&Peer::RouteLua),
        //    sol::resolve<void(const IModManager::MethodSig&, const sol::variadic_args&)>(&Peer::RouteLua)
        //),

        //"GetMethod", static_cast<IMethod<Peer*>* (Peer::*)(const std::string&)>(&Peer::GetMethod)
        //"GetMethod", sol::overload(
        //    sol::resolve<IMethod<Peer*>* (HASH_t)>(&Peer::GetMethod),
        //    sol::resolve<IMethod<Peer*>* (const std::string&)>(&Peer::GetMethod)
        //
        //    //static_cast<IMethod<Peer*>* (Peer::*)(const std::string&)>(&Peer::GetMethod)
        //)
    );

    m_state.new_usertype<Prefab>("Prefab",
        sol::no_constructor,
        "name", sol::readonly(&Prefab::m_name),
        "hash", sol::readonly(&Prefab::m_hash),
        "AllFlagsPresent", &Prefab::AllFlagsPresent,
        "AnyFlagsPresent", &Prefab::AnyFlagsPresent,
        "AllFlagsAbsent", &Prefab::AllFlagsAbsent,
        "AnyFlagsAbsent", &Prefab::AnyFlagsAbsent
    );

    // https://commons.wikimedia.org/wiki/File:IEEE754.svg#/media/File:IEEE754.svg
    // When converting flag double to int from lua->c++, double finely represents all integral values with about
    //  32 bits being perfectly represented
    //  when masking and combining about 35+ bits, a double cannot represent this integral number accurately
    //  ive about reached the limit of using bitflags with lua, and will have to opt for a different type (I dont want to use the intwrapper for flags)

    m_state.new_enum("Flag",
        "NONE", Prefab::Flag::NONE,
        "SCALE", Prefab::Flag::SYNC_INITIAL_SCALE,
        "FAR", Prefab::Flag::DISTANT,
        "SESSIONED", Prefab::Flag::SESSIONED,
        "PIECE", Prefab::Flag::PIECE,
        "BED", Prefab::Flag::BED,
        "DOOR", Prefab::Flag::DOOR,
        "CHAIR", Prefab::Flag::CHAIR,
        "SHIP", Prefab::Flag::SHIP,
        "FISH", Prefab::Flag::FISH,
        "PLANT", Prefab::Flag::PLANT,
        "ARMATURE", Prefab::Flag::ARMOR_STAND,
        "ITEM", Prefab::Flag::ITEM_DROP,
        "PICKABLE", Prefab::Flag::PICKABLE,
        "PICKABLE_ITEM", Prefab::Flag::PICKABLE_ITEM,
        "COOKING", Prefab::Flag::COOKING_STATION,
        "CRAFTING", Prefab::Flag::CRAFTING_STATION,
        "SMELTING", Prefab::Flag::SMELTER,
        "BURNING", Prefab::Flag::FIREPLACE,
        "SUPPORT", Prefab::Flag::WEAR_N_TEAR,
        "BREAKABLE", Prefab::Flag::DESTRUCTIBLE,
        "ATTACH", Prefab::Flag::ITEM_STAND,
        "ANIMAL", Prefab::Flag::ANIMAL_AI,
        "MONSTER", Prefab::Flag::MONSTER_AI,
        "TAME", Prefab::Flag::TAMEABLE,
        "BREED", Prefab::Flag::PROCREATION,
        "MINEABLE_OLD", Prefab::Flag::MINE_ROCK,
        "MINEABLE", Prefab::Flag::MINE_ROCK_5,
        "TREE", Prefab::Flag::TREE_BASE,
        "LOG", Prefab::Flag::TREE_LOG,
        "SFX", Prefab::Flag::SFX,
        "VFX", Prefab::Flag::VFX,
        "AOE", Prefab::Flag::AOE,
        "DUNGEON", Prefab::Flag::DUNGEON,
        "PLAYER", Prefab::Flag::PLAYER,
        "TOMBSTONE", Prefab::Flag::TOMBSTONE
    );



    m_state["PrefabManager"] = PrefabManager();
    m_state.new_usertype<IPrefabManager>("IPrefabManager",
        "GetPrefab", sol::overload(
            sol::resolve<const Prefab* (const std::string&)>(&IPrefabManager::GetPrefab),
            sol::resolve<const Prefab* (HASH_t)>(&IPrefabManager::GetPrefab)
        ),
        "Register", sol::overload(
            sol::resolve<void(const std::string&, Prefab::Type, const Vector3f&, Prefab::Flag, bool)>(&IPrefabManager::Register),
            sol::resolve<void(DataReader&, bool)>(&IPrefabManager::Register)
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
        "zone", sol::property(&ZDO::GetZone),
        "rot", sol::property(&ZDO::Rotation, &ZDO::SetRotation),
        "prefab", sol::property(&ZDO::GetPrefab),
        "owner", sol::property([](const ZDO& self) { return Int64Wrapper(self.Owner()); }, [](ZDO& self, const Int64Wrapper &owner) { self.SetOwner(owner); }),
        "IsOwner", &ZDO::IsOwner,
        "IsLocal", &ZDO::IsLocal,
        "SetLocal", &ZDO::SetLocal,
        //"isLocal", sol::property(&ZDO::IsLocal, [](ZDO& self, bool b) { if (b) self.SetLocal(); else self.Disown(); }),
        "HasOwner", &ZDO::HasOwner,
        "Disown", &ZDO::Disown,
        "dataRev", sol::readonly(&ZDO::m_dataRev),
        "ownerRev", sol::property(&ZDO::GetOwnerRevision),
        //"ticksCreated", sol::property([](ZDO& self) { return (Int64Wrapper) self.m_rev.m_ticksCreated.count(); }), // hmm chrono...
        
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
            //[](ZDO& self, HASH_t key, const Int64Wrapper &value) { return (Int64Wrapper) self.GetLong(key, value); },
            //[](ZDO& self, HASH_t key) { return (Int64Wrapper) self.GetLong(key); },
            //[](ZDO& self, const std::string &key, const Int64Wrapper &value) { return (Int64Wrapper) self.GetLong(key, value); },
            //[](ZDO& self, const std::string &key) { return (Int64Wrapper) self.GetLong(key); }
            sol::resolve<Int64Wrapper(HASH_t, const Int64Wrapper&) const>(&ZDO::GetLongWrapper),
            sol::resolve<Int64Wrapper(HASH_t) const>(&ZDO::GetLongWrapper),
            sol::resolve<Int64Wrapper(const std::string&, const Int64Wrapper&) const>(&ZDO::GetLongWrapper),
            sol::resolve<Int64Wrapper(const std::string&) const>(&ZDO::GetLongWrapper)
        ),
        "GetQuaternion", sol::overload(
            sol::resolve<Quaternion(HASH_t, const Quaternion&) const>(&ZDO::GetQuaternion),
            sol::resolve<Quaternion(HASH_t) const>(&ZDO::GetQuaternion),
            sol::resolve<Quaternion(const std::string&, const Quaternion&) const>(&ZDO::GetQuaternion),
            sol::resolve<Quaternion(const std::string&) const>(&ZDO::GetQuaternion)
        ),
        "GetVector3", sol::overload(
            sol::resolve<Vector3f (HASH_t, const Vector3f&) const>(&ZDO::GetVector3),
            sol::resolve<Vector3f (HASH_t) const>(&ZDO::GetVector3),
            sol::resolve<Vector3f (const std::string&, const Vector3f&) const>(&ZDO::GetVector3),
            sol::resolve<Vector3f (const std::string&) const>(&ZDO::GetVector3)
        ),
        "GetString", sol::overload(
            sol::resolve<std::string (HASH_t, const std::string&) const>(&ZDO::GetString),
            sol::resolve<std::string (HASH_t) const>(&ZDO::GetString),
            sol::resolve<std::string (const std::string&, const std::string&) const>(&ZDO::GetString),
            sol::resolve<std::string (const std::string&) const>(&ZDO::GetString)
        ),
        "GetBytes", sol::overload(
            //sol::resolve<const BYTES_t* (HASH_t) const>(&ZDO::GetBytes),
            //sol::resolve<const BYTES_t* (const std::string&) const>(&ZDO::GetBytes)
            [](ZDO& self, HASH_t key) { auto&& bytes = self.GetBytes(key); return bytes ? std::make_optional(BYTES_t(*bytes)) : std::nullopt; },
            [](ZDO& self, const std::string &key) { auto&& bytes = self.GetBytes(key); return bytes ? std::make_optional(BYTES_t(*bytes)) : std::nullopt; }
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


        // Setters
        "SetFloat", sol::overload(
            static_cast<void (ZDO::*)(HASH_t, const float&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const float&)>(&ZDO::Set)
        ),        
        "SetInt", sol::overload(
            static_cast<void (ZDO::*)(HASH_t, const int32_t&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const int32_t&)>(&ZDO::Set)
        ),
        //"SetLong", sol::overload(
        //    [](ZDO& self, HASH_t key, const Int64Wrapper& value) { self.Set(key, (int64_t)value); },
        //    [](ZDO& self, const std::string &key, const Int64Wrapper& value) { self.Set(key, (int64_t)value); }
        //),
        "Set", sol::overload(
            // Quaternion
            static_cast<void (ZDO::*)(HASH_t, const Quaternion&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const Quaternion&)>(&ZDO::Set),
            // Vector3f
            static_cast<void (ZDO::*)(HASH_t, const Vector3f&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const Vector3f&)>(&ZDO::Set),
            // std::string
            static_cast<void (ZDO::*)(HASH_t, const std::string&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const std::string&)>(&ZDO::Set),
            // bool
            static_cast<void (ZDO::*)(HASH_t, bool)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, bool)>(&ZDO::Set),
            // zdoid
            //static_cast<void (ZDO::*)(HASH_t, HASH_t, const ZDOID&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(const std::string&, const ZDOID&)>(&ZDO::Set),
            // int64 wrapper
            [](ZDO& self, HASH_t key, const Int64Wrapper& value) { self.Set(key, (int64_t)value); },
            [](ZDO& self, const std::string& key, const Int64Wrapper& value) { self.Set(key, (int64_t)value); }
        )
        // I might go ahead and ignore these specific functions, because they
        //  are correctly overloaded anyways to accept specific types, types that
        //  are not convertible to different stuff...
        //"SetQuaternion", sol::overload(
        //    static_cast<void (ZDO::*)(HASH_t, const Quaternion&)>(&ZDO::Set),
        //    static_cast<void (ZDO::*)(const std::string&, const Quaternion&)>(&ZDO::Set)
        //),
        //"SetVector3", sol::overload(
        //    static_cast<void (ZDO::*)(HASH_t, const Vector3f&)>(&ZDO::Set),
        //    static_cast<void (ZDO::*)(const std::string&, const Vector3f&)>(&ZDO::Set)
        //),
        //"SetString", sol::overload(
        //    static_cast<void (ZDO::*)(HASH_t, const std::string&)>(&ZDO::Set),
        //    static_cast<void (ZDO::*)(const std::string&, const std::string&)>(&ZDO::Set)
        //),
        //"SetBool", sol::overload(
        //    static_cast<void (ZDO::*)(HASH_t, bool)>(&ZDO::Set),
        //    static_cast<void (ZDO::*)(const std::string&, bool)>(&ZDO::Set)
        //),
        //"SetZDOID", sol::overload(
        //    // zdoid
        //    //static_cast<void (ZDO::*)(HASH_t, HASH_t, const ZDOID&)>(&ZDO::Set),
        //    static_cast<void (ZDO::*)(const std::string&, const ZDOID&)>(&ZDO::Set)
        //)
    );

    // setting meta functions
    // https://sol2.readthedocs.io/en/latest/api/metatable_key.html
    // 
    // TODO figure the number weirdness out...

    m_state.new_usertype<Int64Wrapper>("Int64",
        sol::constructors<Int64Wrapper(), Int64Wrapper(int64_t), 
            Int64Wrapper(uint32_t, uint32_t), Int64Wrapper(const std::string&)>(),

        "tonumber", [](Int64Wrapper& self) { return (int64_t)self; },
        sol::meta_function::addition, &Int64Wrapper::operator+,
        sol::meta_function::subtraction, sol::resolve<Int64Wrapper(const Int64Wrapper&) const>(&Int64Wrapper::operator-),
        sol::meta_function::multiplication, &Int64Wrapper::operator*,
        sol::meta_function::division, &Int64Wrapper::operator/,
        sol::meta_function::floor_division, &Int64Wrapper::__divi,
        sol::meta_function::unary_minus, sol::resolve<Int64Wrapper() const>(&Int64Wrapper::operator-),
        sol::meta_function::equal_to, &Int64Wrapper::operator==,
        sol::meta_function::less_than, &Int64Wrapper::operator<,
        sol::meta_function::less_than_or_equal_to, &Int64Wrapper::operator<=
    );

    m_state.new_usertype<UInt64Wrapper>("UInt64",
        sol::constructors<UInt64Wrapper(), UInt64Wrapper(uint64_t),
        Int64Wrapper(uint32_t, uint32_t), UInt64Wrapper(const std::string&)>(),

        "tonumber", [](UInt64Wrapper& self) { return (uint64_t)self; },
        sol::meta_function::addition, & UInt64Wrapper::operator+,
        sol::meta_function::subtraction, sol::resolve<UInt64Wrapper(const UInt64Wrapper&) const>(&UInt64Wrapper::operator-),
        sol::meta_function::multiplication, & UInt64Wrapper::operator*,
        sol::meta_function::division, & UInt64Wrapper::operator/,
        sol::meta_function::floor_division, & UInt64Wrapper::__divi,
        sol::meta_function::unary_minus, sol::resolve<UInt64Wrapper() const>(&UInt64Wrapper::operator-),
        sol::meta_function::equal_to, & UInt64Wrapper::operator==,
        sol::meta_function::less_than, & UInt64Wrapper::operator<,
        sol::meta_function::less_than_or_equal_to, &UInt64Wrapper::operator<=
    );

    m_state.new_enum("TimeOfDay",
        "MORNING", TIME_MORNING,
        "DAY", TIME_DAY,
        "AFTERNOON", TIME_AFTERNOON,
        "NIGHT", TIME_NIGHT
    );



    m_state["Valhalla"] = Valhalla();
    m_state.new_usertype<IValhalla>("IValhalla",
        // server members
        "version", sol::var(VConstants::GAME), // Valheim version
        "delta", sol::property(&IValhalla::Delta),
        "id", sol::property([](IValhalla& self) { return Int64Wrapper(self.ID()); }),
        "nanos", sol::property([](IValhalla& self) { return Int64Wrapper(self.Nanos().count()); }),
        "time", sol::property(&IValhalla::Time),
        "timeMultiplier", &IValhalla::m_serverTimeMultiplier,
        // world time functions
        "worldTime", sol::property(sol::resolve<WorldTime() const>(&IValhalla::GetWorldTime), &IValhalla::SetWorldTime),
        "worldTimeMultiplier", sol::property([](IValhalla& self) { return self.m_worldTimeMultiplier; }, [](IValhalla& self, double mul) { if (mul <= 0.001) throw std::runtime_error("multiplier too small"); self.m_worldTimeMultiplier = mul; }),
        "worldTicks", sol::property([](IValhalla& self) { return self.GetWorldTicks(); }),
        "day", sol::property(sol::resolve<int() const>(&IValhalla::GetDay), &IValhalla::SetDay),        
        "timeOfDay", sol::property(sol::resolve<TimeOfDay() const>(&IValhalla::GetTimeOfDay), &IValhalla::SetTimeOfDay),
        "isMorning", sol::property(sol::resolve<bool() const>(&IValhalla::IsMorning)),
        "isDay", sol::property(sol::resolve<bool() const>(&IValhalla::IsDay)),
        "isAfternoon", sol::property(sol::resolve<bool() const>(&IValhalla::IsAfternoon)),
        "isNight", sol::property(sol::resolve<bool() const>(&IValhalla::IsNight)),
        "tomorrowMorning", sol::property(&IValhalla::GetTomorrowMorning),
        "tomorrow", sol::property(&IValhalla::GetTomorrowDay),
        "tomorrowAfternoon", sol::property(&IValhalla::GetTomorrowAfternoon),
        "tomorrowNight", sol::property(&IValhalla::GetTomorrowNight),

        "Subscribe", [this](IValhalla& self, sol::variadic_args args) {
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
            
            callbacks.emplace_back(func, priority);
            callbacks.sort([](const EventHandle& a, const EventHandle& b) {
                return a.m_priority < b.m_priority;
            });
        }
    );

    

    // TODO turn managers into lua classes that can be indexed
    // but still retrieve with ZDOManager... class usertypes will be named by their class names, like IZDOManager...

    m_state["ZDOManager"] = ZDOManager();
    m_state.new_usertype<IZDOManager>("IZDOManager",
        "GetZDO", &IZDOManager::GetZDO,
        "SomeZDOs", sol::overload(
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const Vector3f&, float, size_t, const std::function<bool(const ZDO&)>&)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const Vector3f&, float, size_t)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const Vector3f&, float, size_t, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent)>(&IZDOManager::SomeZDOs),
            [](IZDOManager& self, const Vector3f& pos, float radius, size_t max, const std::string& name) { return self.SomeZDOs(pos, radius, max, VUtils::String::GetStableHashCode(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },

            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, size_t, const std::function<bool(const ZDO&)>&)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, size_t)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, size_t, HASH_t, Prefab::Flag, Prefab::Flag)>(&IZDOManager::SomeZDOs),
            [](IZDOManager& self, const ZoneID& zone, size_t max, const std::string& name) { return self.SomeZDOs(zone, max, VUtils::String::GetStableHashCode(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },

            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, size_t, const Vector3f&, float)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, size_t, const Vector3f&, float, HASH_t, Prefab::Flag, Prefab::Flag)>(&IZDOManager::SomeZDOs),
            [](IZDOManager& self, const ZoneID& zone, size_t max, const Vector3f& pos, float radius, const std::string& name) { return self.SomeZDOs(zone, max, pos, radius, VUtils::String::GetStableHashCode(name), Prefab::Flag::NONE, Prefab::Flag::NONE); }
        ),
        "GetZDOs", sol::overload(
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(HASH_t)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, const std::string& name) { return self.GetZDOs(VUtils::String::GetStableHashCode(name)); },

            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const Vector3f&, float, const std::function<bool(const ZDO&)>&)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const Vector3f&, float)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const Vector3f&, float, HASH_t, Prefab::Flag, Prefab::Flag)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, const Vector3f& pos, float radius, const std::string& name) { return self.GetZDOs(pos, radius, VUtils::String::GetStableHashCode(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },

            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, const std::function<bool(const ZDO&)>&)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, HASH_t, Prefab::Flag, Prefab::Flag)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, const ZoneID& zone, const std::string& name) { return self.GetZDOs(zone, VUtils::String::GetStableHashCode(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },

            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, const Vector3f&, float)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<std::reference_wrapper<ZDO>>(const ZoneID&, const Vector3f&, float, HASH_t, Prefab::Flag, Prefab::Flag)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, const ZoneID& zone, const Vector3f& pos, float radius, const std::string& name) { return self.GetZDOs(zone, pos, radius, VUtils::String::GetStableHashCode(name), Prefab::Flag::NONE, Prefab::Flag::NONE); }
        ),
        "AnyZDO", sol::overload(
            sol::resolve<ZDO* (const Vector3f&, float, HASH_t, Prefab::Flag, Prefab::Flag)>(&IZDOManager::AnyZDO),
            [](IZDOManager& self, const Vector3f& pos, float radius, const std::string& name) { return self.AnyZDO(pos, radius, VUtils::String::GetStableHashCode(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },

            sol::resolve<ZDO* (const ZoneID&, HASH_t, Prefab::Flag, Prefab::Flag)>(&IZDOManager::AnyZDO),
            [](IZDOManager& self, const ZoneID& zone, const std::string& name) { return self.AnyZDO(zone, VUtils::String::GetStableHashCode(name), Prefab::Flag::NONE, Prefab::Flag::NONE); }
        ),
        "NearestZDO", sol::overload(
            sol::resolve<ZDO* (const Vector3f&, float, const std::function<bool(const ZDO&)>&)>(&IZDOManager::NearestZDO),
            sol::resolve<ZDO* (const Vector3f&, float, HASH_t, Prefab::Flag, Prefab::Flag)>(&IZDOManager::NearestZDO),
            [](IZDOManager& self, const Vector3f& pos, float radius, const std::string& name) { return self.NearestZDO(pos, radius, VUtils::String::GetStableHashCode(name), Prefab::Flag::NONE, Prefab::Flag::NONE); }
        ),
        "ForceSendZDO", &IZDOManager::ForceSendZDO,
        //"DestroyZDO", sol::resolve<ZDO&>(&IZDOManager::DestroyZDO),
        "DestroyZDO", sol::overload(
            sol::resolve<void (const ZDOID&)>(&IZDOManager::DestroyZDO),
            sol::resolve<void(ZDO&)>(&IZDOManager::DestroyZDO)
        ),
        "Instantiate", sol::overload(
            sol::resolve<ZDO& (const Prefab&, const Vector3f&, const Quaternion&)>(&IZDOManager::Instantiate),
            sol::resolve<ZDO& (const Prefab&, const Vector3f&)>(&IZDOManager::Instantiate),
            [](IZDOManager& self, const std::string& name, const Vector3f& pos, const Quaternion& rot) { return self.Instantiate(VUtils::String::GetStableHashCode(name), pos, rot); },
            [](IZDOManager& self, const std::string& name, const Vector3f& pos) { return self.Instantiate(VUtils::String::GetStableHashCode(name), pos); },
            sol::resolve<ZDO& (HASH_t, const Vector3f&, const Quaternion&)>(&IZDOManager::Instantiate),
            sol::resolve<ZDO& (HASH_t, const Vector3f&)>(&IZDOManager::Instantiate),
            sol::resolve<ZDO& (const ZDO&)>(&IZDOManager::Instantiate)
        )
    );



    m_state["NetManager"] = NetManager();
    m_state.new_usertype<INetManager>("INetManager",
        "GetPeer", sol::overload(
            [](INetManager& self, const Int64Wrapper& owner) { return self.GetPeer(owner); },
            //sol::resolve<Peer*(OWNER_t)>(&INetManager::GetPeer),
            sol::resolve<Peer* (const std::string&)>(&INetManager::GetPeerByName)
        ),
        "peers", sol::readonly(&INetManager::m_onlinePeers)
    );



    m_state["ModManager"] = ModManager();
    m_state.new_usertype<IModManager>("IModManager",
        "GetMod", [](IModManager& self, const std::string& name) {
            auto&& find = self.m_mods.find(name);
            if (find != self.m_mods.end())
                return find->second.get();
            return static_cast<Mod*>(nullptr);
        }
        //"ReloadMod", [](IModManager& self, Mod& mod) {
        //    if (!self.m_reload) {
        //        mod.m_reload = true;
        //        self.m_reload = true;
        //    }
        //}
    );



    m_state.new_usertype<Dungeon>("Dungeon",
        sol::no_constructor
        //"Generate", sol::resolve<void(const Vector3f& pos, const Quaternion& rot) const>(&Dungeon::Generate)
    );

    m_state["DungeonManager"] = DungeonManager();
    m_state.new_usertype<IDungeonManager>("IDungeonManager",
        "GetDungeon", [](IDungeonManager& self, const std::string& name) { return self.GetDungeon(VUtils::String::GetStableHashCode(name)); },
        "Generate", [](IDungeonManager& self, Dungeon& dungeon, const Vector3f& pos, const Quaternion& rot) { self.Generate(dungeon, pos, rot); }
    );



    m_state.new_usertype<IZoneManager::Feature::Instance>("FeatureInstance",
        "pos", sol::property([](IZoneManager::Feature::Instance& self) { return self.m_pos; })
    );

    m_state["ZoneManager"] = ZoneManager();
    m_state.new_usertype<IZoneManager>("IZoneManager",
        "PopulateZone", sol::resolve<void(const ZoneID&)>(&IZoneManager::PopulateZone),
        "GetNearestFeature", &IZoneManager::GetNearestFeature,
        //"RegenerateZone", &IZoneManager::RegenerateZone,
        "WorldToZonePos", &IZoneManager::WorldToZonePos,
        "ZoneToWorldPos", &IZoneManager::ZoneToWorldPos,
        "globalKeys", sol::property(&IZoneManager::GlobalKeys)

        //"Register"
    );



    // TODO use properties for immutability
    m_state.new_usertype<Mod>("Mod",
        "name", sol::readonly(&Mod::m_name),
        "version", sol::readonly(&Mod::m_version),
        "apiVersion", sol::readonly(&Mod::m_apiVersion),
        "description", sol::readonly(&Mod::m_description),
        "authors", sol::readonly(&Mod::m_authors)
    );



    //m_state.new_usertype<IRouteManager::Data>("RouteData",
    //    "sender", &IRouteManager::Data::m_sender,
    //    "target", &IRouteManager::Data::m_target,
    //    "targetZDO", &IRouteManager::Data::m_targetZDO,
    //    "method", &IRouteManager::Data::m_method,
    //    "params", &IRouteManager::Data::m_params
    //);

    m_state["RouteManager"] = RouteManager();
    m_state.new_usertype<IRouteManager>("IRouteManager",
        "Register", &IRouteManager::RegisterLua,
        "InvokeView", &IRouteManager::InvokeViewLua,
        "Invoke", &IRouteManager::InvokeLua,
        "InvokeAll", &IRouteManager::InvokeAllLua
    );



    {
        auto eventTable = m_state["event"].get_or_create<sol::table>();

        eventTable["Unsubscribe"] = [this]() { this->m_unsubscribeCurrentEvent = false; };
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

        LOG(INFO) << "[Lua] " << s;
    };



    m_state.new_usertype<ZStdCompressor>("ZStdCompressor",
        sol::constructors<ZStdCompressor(int), ZStdCompressor(), ZStdCompressor(const BYTES_t&)>(),
        "Compress", sol::resolve<std::optional<BYTES_t>(const BYTES_t&)>(&ZStdCompressor::Compress)
        );

    m_state.new_usertype<ZStdDecompressor>("ZStdDecompressor",
        sol::constructors<ZStdDecompressor(), ZStdDecompressor(const BYTES_t&)>(),
        "Decompress", sol::resolve<std::optional<BYTES_t>(const BYTES_t&)>(&ZStdDecompressor::Decompress)
        );
    


    m_state.new_usertype<Deflater>("Deflater",
        "gz", sol::property(sol::resolve<Deflater()>(Deflater::Gz)),
        "zlib", sol::property(sol::resolve<Deflater()>(Deflater::ZLib)),
        "raw", sol::property(sol::resolve<Deflater()>(Deflater::Raw)),
        "Compress", sol::resolve<std::optional<BYTES_t>(const BYTES_t&)>(&Deflater::Compress)
    );

    m_state.new_usertype<Inflater>("Inflater",
        //"any", sol::property(Inflater::Any),
        "zlib", sol::property(Inflater::Gz),
        "gz", sol::property(Inflater::Gz),
        "auto", sol::property(Inflater::Auto),
        "raw", sol::property(Inflater::Raw),
        "Decompress", sol::resolve<std::optional<BYTES_t>(const BYTES_t&)>(&Inflater::Decompress)
    );



    {
        auto utilsTable = m_state["VUtils"].get_or_create<sol::table>();

        utilsTable["CreateBytes"] = []() { return BYTES_t(); };

        utilsTable["Assign"] = sol::overload(
            [](BYTES_t& replace, BYTES_t& other) { replace = other; }
        );

        utilsTable["Swap"] = sol::overload(
            [](BYTES_t& a, BYTES_t& b) { std::swap(a, b); }
        );

        //utilsTable["Move"] = sol::overload(
        //    [](BYTES_t& a, BYTES_t& b) { std::swap(a, b); }
        //);

        {
            auto stringUtilsTable = utilsTable["String"].get_or_create<sol::table>();

            stringUtilsTable["GetStableHashCode"] = sol::resolve<HASH_t(std::string_view)>(VUtils::String::GetStableHashCode);
        }

        {
            auto resourceUtilsTable = utilsTable["Resource"].get_or_create<sol::table>();
            
            //resourceUtilsTable["ReadFileBytes"] = sol::resolve<std::optional<BYTES_t>(const fs::path&)>(VUtils::Resource::ReadFile);
            //resourceUtilsTable["ReadFileString"] = sol::resolve<std::optional<std::string>(const fs::path&)>(VUtils::Resource::ReadFile);
            //resourceUtilsTable["ReadFileLines"] = sol::resolve<std::optional<std::vector<std::string>>(const fs::path&, bool)>(VUtils::Resource::ReadFile);
            
            resourceUtilsTable["ReadFileBytes"] = [](const std::string& path) { return VUtils::Resource::ReadFile<BYTES_t>(path); };
            resourceUtilsTable["ReadFileString"] = [](const std::string& path) { return VUtils::Resource::ReadFile<std::string>(path); };
            resourceUtilsTable["ReadFileLines"] = [](const std::string& path) { return VUtils::Resource::ReadFile<std::vector<std::string>>(path); };

            resourceUtilsTable["WriteFile"] = sol::overload(
                sol::resolve<bool(const fs::path&, const BYTES_t&)>(VUtils::Resource::WriteFile),
                sol::resolve<bool(const fs::path&, const std::string&)>(VUtils::Resource::WriteFile),
                sol::resolve<bool(const fs::path&, const std::vector<std::string>&)>(VUtils::Resource::WriteFile),
                sol::resolve<bool(const fs::path&, const std::list<std::string>&)>(VUtils::Resource::WriteFile)
            );
        }
    }
}

void IModManager::LoadMod(Mod& mod) {
    auto path(mod.m_entry);
    if (auto opt = VUtils::Resource::ReadFile<std::string>(path)) {
        m_state.safe_script(opt.value(), mod.m_name);
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

void IModManager::PostInit() {
#ifdef VH_OPTION_ENABLE_MODS
    LOG(INFO) << "Initializing ModManager";

    m_state.set_exception_handler(&my_exception_handler);

    m_state.open_libraries();

    LoadAPI();

    std::error_code ec;
    fs::create_directories(VALHALLA_MOD_PATH, ec);
    
    if (ec)
        return;

    for (const auto& dir
        : fs::directory_iterator(VALHALLA_MOD_PATH, ec)) {

        try {
            if (dir.exists(ec) && dir.is_directory(ec)) {
                auto&& dirname = dir.path().filename().string();

                if (dirname.starts_with("--"))
                    continue;

                auto&& mod = LoadModInfo(dirname);
                LoadMod(mod);

                LOG(INFO) << "Loaded mod '" << mod.m_name << "'";
            }
        }
        catch (const std::exception& e) {
            LOG(ERROR) << "Failed to load mod: " << e.what() << " (" << dir.path().c_str() << ")";
        }
    }

    LOG(INFO) << "Loaded " << m_mods.size() << " mods";

    VH_DISPATCH_MOD_EVENT(IModManager::Events::Enable);
#endif
}

void IModManager::Uninit() {
#ifdef VH_OPTION_ENABLE_MODS
    VH_DISPATCH_MOD_EVENT(IModManager::Events::Disable);
    m_callbacks.clear();
    m_mods.clear();
#endif
}
