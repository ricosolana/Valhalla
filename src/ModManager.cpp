#include "ModManager.h"
#include "Types.h"
#include "UserData.h"
#include <atomic>
#include <cmath>
#include <cstdint>
#include <sol/forward.hpp>
#include <sol/overload.hpp>
#include <sol/property.hpp>

#if VH_IS_ON(VH_USE_MODS)

#include <yaml-cpp/yaml.h>

#include "ModManager.h"
#include "VUtilsResource.h"
#include "VUtilsString.h"
#include "Peer.h"
#include "DataStream.h"
#include "DataStream.h"
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

IModManager::Mod& IModManager::LoadModInfo(std::string_view folderName) {
    YAML::Node loadNode;

    auto modPath = fs::path("mods") / folderName;
    auto modInfoPath = modPath / "modInfo.yml";

    if (auto opt = VUtils::Resource::ReadFile<std::string>(modInfoPath)) {
        loadNode = YAML::Load(opt.value());
    }
    else {
        throw std::runtime_error("unable to open " + modInfoPath.string());
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
    if (auto opt = VUtils::Resource::ReadFile<std::string>("./mods/" + path + ".lua"))
        luaL_loadbuffer(L, opt.value().data(), opt.value().size(), path.c_str());
    else {
        sol::stack::push(L, "Module '" + path + "' not found");
    }

    return 1;
}

void IModManager::LoadAPI() {    
    m_state.new_usertype<Vector3f>("Vector3f",
        sol::constructors<Vector3f(), Vector3f(float, float, float)>(),
        "ZERO", sol::property(&Vector3f::zero),
        "x", &Vector3f::x,
        "y", &Vector3f::y,
        "z", &Vector3f::z,
        "magnitude", sol::property(&Vector3f::magnitude),
        "sq_magnitude", sol::property(&Vector3f::sq_magnitude),
        "normal", sol::property(&Vector3f::normal),
        "distance_to", &Vector3f::distance_to,
        "sq_distance_to", &Vector3f::sq_distance_to,
        "dot", &Vector3f::dot,
        "cross", &Vector3f::cross,
        sol::meta_function::addition, &Vector3f::operator+,
        sol::meta_function::subtraction, sol::resolve<Vector3f(Vector3f const&) const>(&Vector3f::operator-),
        sol::meta_function::unary_minus, sol::resolve<Vector3f() const>(&Vector3f::operator-),
        sol::meta_function::multiplication, sol::resolve<Vector3f(Vector3f const&) const>(&Vector3f::operator*),
        sol::meta_function::division, sol::resolve<Vector3f(Vector3f const&) const>(&Vector3f::operator/),
        sol::meta_function::equal_to, &Vector3f::operator==
    );

    m_state.new_usertype<Vector2f>("Vector2f",
        sol::constructors<Vector2f(), Vector2f(float, float)>(),
        "ZERO", sol::property(&Vector2f::zero),
        "x", &Vector2f::x,
        "y", &Vector2f::y,
        "magnitude", sol::property(&Vector2f::magnitude),
        "sq_magnitude", sol::property(&Vector2f::sq_magnitude),
        "normal", sol::property(&Vector2f::normal),
        "distance_to", &Vector2f::distance_to,
        "sq_distance_to", &Vector2f::sq_distance_to,
        "dot", &Vector2f::dot,
        sol::meta_function::addition, &Vector2f::operator+,
        sol::meta_function::subtraction, sol::resolve<Vector2f(Vector2f const&) const>(&Vector2f::operator-),
        sol::meta_function::unary_minus, sol::resolve<Vector2f() const>(&Vector2f::operator-),
        sol::meta_function::multiplication, sol::resolve<Vector2f(Vector2f const&) const>(&Vector2f::operator*),
        sol::meta_function::division, sol::resolve<Vector2f(Vector2f const&) const>(&Vector2f::operator/),
        sol::meta_function::equal_to, &Vector2f::operator==
    );

    m_state.new_usertype<Vector2i>("Vector2i",
        sol::constructors<Vector2i(), Vector2i(std::int32_t, std::int32_t)>(),
        "ZERO", sol::property(&Vector2i::zero),
        "x", &Vector2i::x,
        "y", &Vector2i::y,
        "magnitude", sol::property(&Vector2i::magnitude),
        "sq_magnitude", sol::property(&Vector2i::sq_magnitude),
        "normal", sol::property(&Vector2i::normal),
        "distance_to", &Vector2i::distance_to,
        "sq_distance_to", &Vector2i::sq_distance_to,
        "dot", &Vector2i::dot,
        sol::meta_function::addition, &Vector2i::operator+,
        sol::meta_function::subtraction, sol::resolve<Vector2i(Vector2i const&) const>(&Vector2i::operator-),
        sol::meta_function::unary_minus, sol::resolve<Vector2i() const>(&Vector2i::operator-),
        sol::meta_function::multiplication, sol::resolve<Vector2i(Vector2i const&) const>(&Vector2i::operator*),
        sol::meta_function::division, sol::resolve<Vector2i(Vector2i const&) const>(&Vector2i::operator/),
        sol::meta_function::equal_to, &Vector2i::operator==
    );

    m_state.new_usertype<Vector2s>("Vector2s",
        sol::constructors<Vector2s(), Vector2s(std::int16_t, std::int16_t)>(),
        "ZERO", sol::property(&Vector2s::zero),
        "x", &Vector2s::x,
        "y", &Vector2s::y,
        "magnitude", sol::property(&Vector2s::magnitude),
        "sq_magnitude", sol::property(&Vector2s::sq_magnitude),
        "normal", sol::property(&Vector2s::normal),
        "distance_to", &Vector2s::distance_to,
        "sq_distance_to", &Vector2s::sq_distance_to,
        "dot", &Vector2s::dot,
        sol::meta_function::addition, &Vector2s::operator+,
        sol::meta_function::subtraction, sol::resolve<Vector2s(Vector2s const&) const>(&Vector2s::operator-),
        sol::meta_function::unary_minus, sol::resolve<Vector2s() const>(&Vector2s::operator-),
        sol::meta_function::multiplication, sol::resolve<Vector2s(Vector2s const&) const>(&Vector2s::operator*),
        sol::meta_function::division, sol::resolve<Vector2s(Vector2s const&) const>(&Vector2s::operator/),
        sol::meta_function::equal_to, &Vector2s::operator==
    );

    m_state.new_usertype<Quaternion>("Quaternion",
        sol::constructors<Quaternion(), Quaternion(float, float, float, float)>(),
        "IDENTITY", sol::var(Quaternion::IDENTITY),
        "x", &Quaternion::x,
        "y", &Quaternion::y,
        "z", &Quaternion::z,
        "w", &Quaternion::w,
        sol::meta_function::multiplication, sol::resolve<Quaternion(Quaternion) const>(&Quaternion::operator*)
    );

    m_state.new_usertype<ZDOID>("ZDOID",
        //sol::constructors<ZDOID(avledet::util::UserID userID, std::uint32_t id)>(),
        sol::factories([](Int64Wrapper uuid, std::uint32_t id) { return ZDOID((std::int64_t)uuid, id); }),
        "NONE", sol::var(ZDOID::NONE), // sol::property([]() { return ZDOID::NONE; }),
        "user_id", sol::property([](ZDOID& self) { return (Int64Wrapper)self.get_user_id(); }, [](ZDOID& self, Int64Wrapper value) { self.set_user_id((std::int64_t)value); }),
        "id", sol::property(&ZDOID::get_id, &ZDOID::set_id)
    );

    m_state.new_enum("Type",
        "BOOL", Type::BOOL,

        "STRING", Type::STRING,
        "STRINGS", Type::STRINGS,

        "BYTES", Type::BYTES,
        
        "ZDOID", Type::ZDOID,
        "VECTOR3f", Type::VECTOR3f, "vec3f", Type::VECTOR3f,
        "VECTOR2i", Type::VECTOR2i, "vec2i", Type::VECTOR2i, 
        "QUATERNION", Type::QUATERNION, "quat", Type::QUATERNION,
        
        "INT8", Type::INT8, "s8", Type::INT8,
        "INT16", Type::INT16, "SHORT", Type::INT16, "s16", Type::INT16,
        "INT32", Type::INT32, "INT", Type::INT32, "HASH", Type::INT32, "s32", Type::INT32,
        "INT64", Type::INT64, "LONG", Type::INT64, "s64", Type::INT64,

        "UINT8", Type::UINT8, "BYTE", Type::UINT8, "u8", Type::UINT8,
        "UINT16", Type::UINT16, "USHORT", Type::UINT16, "u16", Type::UINT16,
        "UINT32", Type::UINT32, "UINT", Type::UINT32, "u32", Type::UINT32,
        "UINT64", Type::UINT64, "ULONG", Type::UINT64, "u64", Type::UINT64,

        "FLOAT", Type::FLOAT,
        "DOUBLE", Type::DOUBLE,

        "CHAR16", Type::CHAR16
    );
    
    // TODO
    //  this seems like some very unsafe / sketchy usage
    m_state.new_usertype<avledet::util::Bytes>("Bytes",
        sol::constructors<avledet::util::Bytes(), avledet::util::Bytes(const avledet::util::Bytes&)>(),
        "assign", [](avledet::util::Bytes& self, const avledet::util::Bytes& other) { self = other; },
        "move", [](avledet::util::Bytes& self, avledet::util::Bytes& other) { self = std::move(other); },
        "swap", [](avledet::util::Bytes& self, avledet::util::Bytes& other) { self.swap(other); }
    );

    // TODO impl
    //m_state.new_usertype<UserProfile>("UserProfile",
    //    sol::constructors<UserProfile(std::string, std::string, std::string)>(),
    //    "name", &UserProfile::m_name,
    //    "tag", &UserProfile::m_gamerTag, // TODO change name
    //    "nid", &UserProfile::m_networkUserId // TODO change name
    //);

    m_state.new_usertype<DataWriter>("DataWriter",
        sol::constructors<DataWriter(avledet::util::Bytes)>(),

        //"ToReader", &DataWriter::ToReader,
        //"buf", &DataWriter::get_buf, //TODO currently unsafe
        "pos", sol::property(&DataWriter::get_pos, &DataWriter::set_pos), //& DataWriter::m_pos,

        //"Clear", &DataWriter::Clear,

        "write_bool", &DataWriter::write<bool>,
        "write_string", &DataWriter::write<std::string_view>,
        "write_bytes", &DataWriter::write<avledet::util::Bytes>,
        "write_zdoid", &DataWriter::write<avledet::util::ZDOID>,
        "write_vec3f", &DataWriter::write<avledet::util::CSU::Vector3f>,
        "write_vec2i", &DataWriter::write<avledet::util::CSU::Vector2i>,
        "write_quat", &DataWriter::write<avledet::util::CSU::Quaternion>,
        //"write_profile", &DataWriter::write<UserProfile>, //TODO impl

        //TODO impl everything
        "write_s8", &DataWriter::write<std::int8_t>, // static_cast<void (DataWriter::*)(std::int8_t)>(&DataWriter::write),
        "write_s16", &DataWriter::write<std::int16_t>,
        "write_s32", &DataWriter::write<std::int32_t>,
        //"write_s64", &DataWriter::write<std::int64_t>,// TODO impl: intwrapper
        "write_u8", &DataWriter::write<std::uint8_t>,
        "write_u16", &DataWriter::write<std::uint16_t>,
        "write_u32", &DataWriter::write<std::uint32_t>,
        "write_u64", &DataWriter::write<std::uint64_t>,
        "write_float", &DataWriter::write<std::float_t>,
        "write_double", &DataWriter::write<std::double_t>,
        "write_char16", &DataWriter::write<char16_t>,
        "write", sol::overload(
            [](DataWriter& self, bool val) { return self.write(val); },
            [](DataWriter& self, std::string_view val) { return self.write(val); },
            [](DataWriter& self, avledet::util::Bytes const& val) { return self.write(val); },
            [](DataWriter& self, avledet::util::ZDOID const& val) { return self.write(val); },
            [](DataWriter& self, avledet::util::CSU::Vector3f const& val) { return self.write(val); },
            [](DataWriter& self, avledet::util::CSU::Vector2i const& val) { return self.write(val); },
            [](DataWriter& self, avledet::util::CSU::Quaternion const& val) { return self.write(val); },
            // Variadic serializers:::
            [](DataWriter &self, IModManager::Type type, sol::object obj) { self.write(type, obj); },
            //{ &DataWriter::write<IModManager::Type, sol::object> },
            [](DataWriter &self, IModManager::Types const& types, sol::variadic_args args) {
                self.write(types, sol::variadic_results(args.begin(), args.end()));
            }
        )
    );

    // Package read/write types
    m_state.new_usertype<DataReader>("DataReader",
        sol::constructors<DataReader(avledet::util::Bytes)>(),

        //"ToWriter", &DataReader::ToWriter,
        //"buf", &DataReader::m_buf,
        /*
        "buf", sol::property(
            sol::overload(
                [](DataReader& self, avledet::util::Bytes& value) { self.m_data = std::ref(value); },
                [](DataReader& self, avledet::util::ByteView value) { self.m_data = value; }
            ),
            [this](DataReader& self) {
                return std::visit(VUtils::Traits::overload{
                    [this](std::reference_wrapper<avledet::util::Bytes> buf) { return sol::make_object(m_state, buf); },
                    [this](avledet::util::ByteView buf) { return sol::make_object(m_state, buf); }
                }, self.m_data);
            }
        ),*/
        //"buf", &DataReader::m_data, // TODO ref change
        "pos", sol::property(&DataReader::get_pos, &DataReader::set_pos), //& DataWriter::m_pos,

        "read_bool", [](DataReader& self) { return self.read<bool>(); },

        "read_string", [](DataReader& self) { return self.read<std::string>(); },
        "read_strings", [](DataReader& self) { return self.read<avledet::util::Strings>(); }, // ReadStrings,

        "read_bytes", [](DataReader& self) { return self.read<avledet::util::Bytes>(); },

        "read_zdoid", [](DataReader& self) { return self.read<avledet::util::ZDOID>(); }, //&DataReader::read<avledet::util::ZDOID>,
        "read_vec3f", [](DataReader& self) { return self.read<avledet::util::CSU::Vector3f>(); }, //&DataReader::read<avledet::util::CSU::Vector3f>,
        "read_vec2i", [](DataReader& self) { return self.read<avledet::util::CSU::Vector2i>(); }, //&DataReader::read<avledet::util::CSU::Vector2i>,
        "read_quat", [](DataReader& self) { return self.read<avledet::util::CSU::Quaternion>(); }, //&DataReader::read<avledet::util::CSU::Quaternion>,
        //"ReadProfile", [](DataReader& self) { return self.read<UserProfile>(); }, //&DataReader::read<UserProfile>, //TODO impl

        "read_s8", [](DataReader& self) { return self.read<std::int8_t>(); }, //&DataReader::read<std::int8_t>,
        "read_s16", [](DataReader& self) { return self.read<std::int16_t>(); }, //&DataReader::read<std::int16_t>,
        "read_s32", [](DataReader& self) { return self.read<std::int32_t>(); }, //&DataReader::read<std::int32_t>,
        //"read_s64", &DataReader::ReadInt64Wrapper, //TODO Streamer impl

        "read_u8", [](DataReader& self) { return self.read<std::uint8_t>(); }, //&DataReader::read<std::uint8_t>,
        "read_u16", [](DataReader& self) { return self.read<std::uint16_t>(); }, //&DataReader::read<std::uint16_t>,
        "read_u32", [](DataReader& self) { return self.read<std::uint32_t>(); }, //&DataReader::read<std::uint32_t>,
        //"read_u64", &DataReader::ReadUInt64Wrapper, //TODO Streamer impl

        "read_float", [](DataReader& self) { return self.read<std::float_t>(); }, //&DataReader::read<std::float_t>,
        "read_double", [](DataReader& self) { return self.read<std::double_t>(); }, //&DataReader::read<std::double_t>,
                
        "read_char16", [](DataReader& self) { return self.read<char16_t>(); }, //&DataReader::read<char16_t>,

        // Generalized variadic read
        //  local reader = Reader.new()
        //  reader:read()
        "read", [](DataReader& self, sol::state_view state, sol::variadic_args args) { 
            return self.read(IModManager::Types(args.begin(), args.end()), state);
        }
        
    );

    //m_state.new_usertype<IMethod<Peer*>>("IMethodPeer",
    //    "Invoke", &IMethod<Peer*>::Invoke
    //);

    m_state.new_usertype<ISocket>("Socket",
        "close", &ISocket::Close,
        "connected", sol::property(&ISocket::Connected),
        "address", sol::property(&ISocket::GetAddress),
        "host", sol::property(&ISocket::GetHostName),
        "send_queue_size", sol::property(&ISocket::GetSendQueueSize)
    );

    m_state.new_usertype<MethodSig>("MethodSig",
        sol::factories([](std::string_view name, sol::variadic_args types) { return MethodSig{ avledet::util::get_stable_hash(name), IModManager::Types(types.begin(), types.end()) }; })
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
        //"visibleOnMap", &Peer::m_visibleOnMap,
        "marker", sol::property(&Peer::IsMapVisible, &Peer::SetMapVisible),
        //"admin", &Peer::m_admin,
        "admin", sol::property(&Peer::IsAdmin, &Peer::SetAdmin),
        "character_id", sol::property([](Peer& self) -> ZDOID { return self.m_characterID; }), // return copy
        "name", sol::readonly(&Peer::m_name), // strings are immutable in Lua similarly to Java
        "pos", &Peer::m_pos,
        //"uuid", sol::property([](Peer& self) { return Int64Wrapper(self.m_uuid); }),
        "socket", sol::readonly(&Peer::m_socket),
        "zdo", sol::property(&Peer::GetZDO),
        // member functions
        "kick", sol::resolve<void ()>(&Peer::Kick),
        // message functions
        "chat_message", static_cast<void (Peer::*)(std::string_view)>(&Peer::ChatMessage),
        "console_message", static_cast<void (Peer::*)(std::string_view)>(&Peer::ConsoleMessage),
        //"ConsoleMessage", sol::resolve<void(std::string_view)>(&Peer::ConsoleMessage),
        //"ConsoleMessage", &Peer::ConsoleMessage,
        "corner_message", static_cast<void (Peer::*)(std::string_view)>(&Peer::CornerMessage),
        "center_message", static_cast<void (Peer::*)(std::string_view)>(&Peer::CenterMessage),
        // misc functions
        "teleport", sol::overload(
            sol::resolve<void (Vector3f pos, Quaternion rot, bool animation)>(&Peer::Teleport),
            sol::resolve<void (Vector3f pos)>(&Peer::Teleport)
        ),
        //"MoveTo", sol::overload(
        //    sol::resolve<void(const Vector3f& pos, const Quaternion& rot)>(&Peer::MoveTo),
        //    sol::resolve<void(const Vector3f& pos)>(&Peer::MoveTo)
        //),
        "disconnect", &Peer::Disconnect,
        "invoke_self", sol::overload(
            sol::resolve<bool (avledet::util::Hash, DataReader&)>(&Peer::InternalInvoke),
            sol::resolve<bool (std::string_view, DataReader&)>(&Peer::InternalInvoke)
        ),

            //static_cast<void (Peer::*)(const std::string&, DataReader)>(&Peer::InvokeSelf), //  &Peer::InvokeSelf,
            //static_cast<void (Peer::*)(avledet::util::Hash, DataReader)>(&Peer::InvokeSelf)), //  &Peer::InvokeSelf,
        //"Register", [](Peer& self, const MethodSig &repr, sol::function func) {
        //    self.Register(repr.m_hash, func, repr.m_types);
        //},

        // static_cast<void (DataWriter::*)(const avledet::util::Bytes&, std::size_t)>(&DataWriter::write),
        "register", &Peer::RegisterLua,
        //"Register", [](Peer& self, const IModManager::MethodSig& sig, const sol::function& func, sol::this_environment te) { 
        //    sol::environment& env = te;
        //    Mod& mod = env["this"].get<sol::table>().as<Mod&>();
        //    self.RegisterLua(sig, func, &mod); 
        //},
        "invoke", &Peer::InvokeLua,
        "route_view", &Peer::RouteViewLua,
        "route", &Peer::RouteLua
        //sol::overload(
        //    sol::resolve<void(const ZDOID&, const IModManager::MethodSig&, const sol::variadic_args&)>(&Peer::RouteLua),
        //    sol::resolve<void(const IModManager::MethodSig&, const sol::variadic_args&)>(&Peer::RouteLua)
        //),

        //"GetMethod", static_cast<IMethod<Peer*>* (Peer::*)(const std::string&)>(&Peer::GetMethod)
        //"GetMethod", sol::overload(
        //    sol::resolve<IMethod<Peer*>* (avledet::util::Hash)>(&Peer::GetMethod),
        //    sol::resolve<IMethod<Peer*>* (const std::string&)>(&Peer::GetMethod)
        //
        //    //static_cast<IMethod<Peer*>* (Peer::*)(const std::string&)>(&Peer::GetMethod)
        //)
    );

    m_state.new_usertype<Prefab>("Prefab",
        sol::no_constructor,
        "name", sol::readonly(&Prefab::m_name),
        "hash", sol::readonly(&Prefab::m_hash),
        "flags_all", &Prefab::AllFlagsPresent,
        "flags_any", &Prefab::AnyFlagsPresent,
        "flags_nall", &Prefab::AllFlagsAbsent,
        "flags_nany", &Prefab::AnyFlagsAbsent
    );

    // https://commons.wikimedia.org/wiki/File:IEEE754.svg#/media/File:IEEE754.svg
    // When converting flag double to int from lua->c++, double finely represents all integral values with about
    //  32 bits being perfectly represented
    //  when masking and combining about 35+ bits, a double cannot represent this integral number accurately
    //  ive about reached the limit of using bitflags with lua, and will have to opt for a different type (I dont want to use the intwrapper for flags)

    m_state.new_enum("Flag",
        "NONE", Prefab::Flag::NONE,

        "SCALE", Prefab::Flag::SYNC_INITIAL_SCALE,
        "DISTANT", Prefab::Flag::DISTANT,
        "PERSISTENT", Prefab::Flag::PERSISTENT,
        "TYPE1", Prefab::Flag::TYPE1,
        "TYPE2", Prefab::Flag::TYPE2,

        "PIECE", Prefab::Flag::PIECE,
        "BED", Prefab::Flag::BED,
        "DOOR", Prefab::Flag::DOOR,
        "CHAIR", Prefab::Flag::CHAIR,
        "SHIP", Prefab::Flag::SHIP,
        "FISH", Prefab::Flag::FISH,
        "PLANT", Prefab::Flag::PLANT,
        "ARMOR_STAND", Prefab::Flag::ARMOR_STAND,

        "PROJECTILE", Prefab::Flag::PROJECTILE,
        "ITEM_DROP", Prefab::Flag::ITEM_DROP,
        "PICKABLE", Prefab::Flag::PICKABLE,
        "PICKABLE_ITEM", Prefab::Flag::PICKABLE_ITEM,

        "CONTAINER", Prefab::Flag::CONTAINER,
        "COOKING_STATION", Prefab::Flag::COOKING_STATION,
        "CRAFTING_STATION", Prefab::Flag::CRAFTING_STATION,
        "SMELTER", Prefab::Flag::SMELTER,
        "FIREPLACE", Prefab::Flag::FIREPLACE,

        "WEAR_N_TEAR", Prefab::Flag::WEAR_N_TEAR,
        "DESTRUCTIBLE", Prefab::Flag::DESTRUCTIBLE,
        "ITEM_STAND", Prefab::Flag::ITEM_STAND,
        
        "ANIMAL_AI", Prefab::Flag::ANIMAL_AI,
        "MONSTER_AI", Prefab::Flag::MONSTER_AI,
        "TAMEABLE", Prefab::Flag::TAMEABLE,
        "PROCREATION", Prefab::Flag::PROCREATION,
        
        "MINE_ROCK_5", Prefab::Flag::MINE_ROCK_5,
        "TREE_BASE", Prefab::Flag::TREE_BASE,
        "TREE_LOG", Prefab::Flag::TREE_LOG,
        
        "DUNGEON", Prefab::Flag::DUNGEON,
        "TERRAIN_MODIFIER", Prefab::Flag::TERRAIN_MODIFIER,
        "CREATURE_SPAWNER", Prefab::Flag::CREATURE_SPAWNER
    );


    m_state["PrefabManager"] = PrefabManager();
    m_state.new_usertype<IPrefabManager>("IPrefabManager",
        "get_prefab", sol::overload(
            sol::resolve<const Prefab*(avledet::util::Hash) const>(&IPrefabManager::GetPrefab),
            sol::resolve<const Prefab*(std::string_view) const>(&IPrefabManager::GetPrefab)
        )
        // TODO restrict prefab registration to startup only
        /*
        "Register", sol::overload(
            sol::resolve<void(std::string_view, avledet::util::ObjectType, Vector3f, Prefab::Flag)>(&IPrefabManager::Register),
            sol::resolve<void(DataReader&)>(&IPrefabManager::Register)
        )*/
    );

    //auto prefabApiTable = m_state["PrefabManager"].get_or_create<sol::table>();
    //prefabApiTable["GetPrefab"] = sol::overload(
    //    [](const std::string& name) { return PrefabManager()->GetPrefab(name); },
    //    [](avledet::util::Hash hash) { return PrefabManager()->GetPrefab(hash); }
    //);


    m_state.new_usertype<ZDO>("ZDO",
        sol::no_constructor,
        "id", sol::property(&ZDO::GetID),
        "pos", sol::property(&ZDO::GetPosition, &ZDO::SetPosition),
        "zone", sol::property(&ZDO::GetZone),
        "rot", sol::property(&ZDO::GetRotation, &ZDO::SetRotation),
        "prefab", sol::property(&ZDO::GetPrefab),
        "prefab_hash", sol::property(&ZDO::GetPrefabHash),
        "owner", sol::property([](ZDO self) { return Int64Wrapper(self.Owner()); }, [](ZDO self, Int64Wrapper owner) { self.SetOwner((std::int64_t)owner); }),
        "is_owner", &ZDO::IsOwner, // zdo:is_owner(id)
        "local", sol::property(&ZDO::IsLocal, &ZDO::SetLocal),
        //"isLocal", sol::property(&ZDO::IsLocal, [](ZDO& self, bool b) { if (b) self.SetLocal(); else self.Disown(); }),
        "owned", sol::property(&ZDO::HasOwner),
        "disown", &ZDO::Disown, //TODO rename?
        "data_rev", sol::property(&ZDO::GetDataRevision), // sol::property([](ZDO self) { return self.Revision().GetDataRevision(); }),
        "owner_rev", sol::property(&ZDO::GetOwnerRevision),
        //"ticksCreated", sol::property([](ZDO& self) { return (Int64Wrapper) self.m_rev.m_ticksCreated.count(); }), // hmm chrono...
        
        // Getters
        "get_float", sol::overload(
            sol::resolve<float(avledet::util::Hash, float) const>(&ZDO::GetFloat),
            sol::resolve<float(avledet::util::Hash) const>(&ZDO::GetFloat),
            sol::resolve<float(std::string_view, float) const>(&ZDO::GetFloat),
            sol::resolve<float(std::string_view) const>(&ZDO::GetFloat)
        ),
        "get_int", sol::overload(
            sol::resolve<std::int32_t(avledet::util::Hash, std::int32_t) const>(&ZDO::GetInt),
            sol::resolve<std::int32_t(avledet::util::Hash) const>(&ZDO::GetInt),
            sol::resolve<std::int32_t(std::string_view, std::int32_t) const>(&ZDO::GetInt),
            sol::resolve<std::int32_t(std::string_view) const>(&ZDO::GetInt)
        ),
        "get_long", sol::overload(
            sol::resolve<Int64Wrapper(avledet::util::Hash, Int64Wrapper) const>(&ZDO::GetLongWrapper),
            sol::resolve<Int64Wrapper(avledet::util::Hash) const>(&ZDO::GetLongWrapper),
            sol::resolve<Int64Wrapper(std::string_view, Int64Wrapper) const>(&ZDO::GetLongWrapper),
            sol::resolve<Int64Wrapper(std::string_view) const>(&ZDO::GetLongWrapper)
        ),
        "get_quat", sol::overload(
            sol::resolve<Quaternion(avledet::util::Hash, Quaternion) const>(&ZDO::GetQuaternion),
            sol::resolve<Quaternion(avledet::util::Hash) const>(&ZDO::GetQuaternion),
            sol::resolve<Quaternion(std::string_view, Quaternion) const>(&ZDO::GetQuaternion),
            sol::resolve<Quaternion(std::string_view) const>(&ZDO::GetQuaternion)
        ),
        "get_vec3", sol::overload(
            sol::resolve<Vector3f (avledet::util::Hash, Vector3f) const>(&ZDO::GetVector3),
            sol::resolve<Vector3f (avledet::util::Hash) const>(&ZDO::GetVector3),
            sol::resolve<Vector3f (std::string_view, Vector3f) const>(&ZDO::GetVector3),
            sol::resolve<Vector3f (std::string_view) const>(&ZDO::GetVector3)
        ),
        "get_string", sol::overload(
            sol::resolve<std::string_view (avledet::util::Hash, std::string_view) const>(&ZDO::GetString),
            sol::resolve<std::string_view (avledet::util::Hash) const>(&ZDO::GetString),
            sol::resolve<std::string_view (std::string_view, std::string_view) const>(&ZDO::GetString),
            sol::resolve<std::string_view (std::string_view) const>(&ZDO::GetString)
        ),
        "get_bytes", sol::overload(
            sol::resolve<const avledet::util::Bytes* (avledet::util::Hash) const>(&ZDO::GetBytes),
            sol::resolve<const avledet::util::Bytes* (std::string_view) const>(&ZDO::GetBytes)
            //[](ZDO& self, avledet::util::Hash key) { auto&& bytes = self.GetBytes(key); return bytes ? std::make_optional(avledet::util::Bytes(*bytes)) : std::nullopt; },
            //[](ZDO& self, std::string_view key) { auto&& bytes = self.GetBytes(key); return bytes ? std::make_optional(avledet::util::Bytes(*bytes)) : std::nullopt; }
        ),
        "get_bool", sol::overload(
            sol::resolve<bool (avledet::util::Hash, bool) const>(&ZDO::GetBool),
            sol::resolve<bool (avledet::util::Hash) const>(&ZDO::GetBool),
            sol::resolve<bool (std::string_view, bool) const>(&ZDO::GetBool),
            sol::resolve<bool (std::string_view) const>(&ZDO::GetBool)
        ),
        "get_zdoid", sol::overload(
            //sol::resolve<ZDOID(avledet::util::Hash, const ZDOID&) const>(&ZDO::GetZDOID),
            //sol::resolve<ZDOID(avledet::util::Hash) const>(&ZDO::GetZDOID),
            sol::resolve<ZDOID(std::string_view, ZDOID) const>(&ZDO::GetZDOID),
            sol::resolve<ZDOID(std::string_view) const>(&ZDO::GetZDOID)
        ),


        // Setters
        "set_float", sol::overload(
            static_cast<void (ZDO::*)(avledet::util::Hash, float)>(&ZDO::Set),
            static_cast<void (ZDO::*)(std::string_view, float)>(&ZDO::Set)
        ),        
        "set_int", sol::overload(
            static_cast<void (ZDO::*)(avledet::util::Hash, std::int32_t)>(&ZDO::Set),
            static_cast<void (ZDO::*)(std::string_view, std::int32_t)>(&ZDO::Set)
        ),
        "set", sol::overload(
            // Quaternion
            static_cast<void (ZDO::*)(avledet::util::Hash, Quaternion)>(&ZDO::Set),
            static_cast<void (ZDO::*)(std::string_view, Quaternion)>(&ZDO::Set),
            // Vector3f
            static_cast<void (ZDO::*)(avledet::util::Hash, Vector3f)>(&ZDO::Set),
            static_cast<void (ZDO::*)(std::string_view, Vector3f)>(&ZDO::Set),
            // std::string
            static_cast<void (ZDO::*)(avledet::util::Hash, std::string)>(&ZDO::Set),
            static_cast<void (ZDO::*)(std::string_view, std::string)>(&ZDO::Set),
            // bool
            static_cast<void (ZDO::*)(avledet::util::Hash, bool)>(&ZDO::Set),
            static_cast<void (ZDO::*)(std::string_view, bool)>(&ZDO::Set),
            // zdoid
            //static_cast<void (ZDO::*)(avledet::util::Hash, avledet::util::Hash, const ZDOID&)>(&ZDO::Set),
            static_cast<void (ZDO::*)(std::string_view, ZDOID)>(&ZDO::Set),
            // int64 wrapper
            [](ZDO& self, avledet::util::Hash key, Int64Wrapper value) { self.Set(key, (std::int64_t)value); },
            [](ZDO& self, std::string_view key, Int64Wrapper value) { self.Set(key, (std::int64_t)value); }
        )
    );

    // setting meta functions
    // https://sol2.readthedocs.io/en/latest/api/metatable_key.html
    // 
    // TODO figure the number weirdness out...

    m_state.new_usertype<Int64Wrapper>("Int64",
        sol::constructors<Int64Wrapper(), Int64Wrapper(std::int64_t), 
            Int64Wrapper(std::uint32_t, std::uint32_t), Int64Wrapper(const std::string&)>(),

        "tonumber", [](Int64Wrapper& self) { return (std::int64_t)self; },
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
        sol::constructors<UInt64Wrapper(), UInt64Wrapper(std::uint64_t),
        Int64Wrapper(std::uint32_t, std::uint32_t), UInt64Wrapper(const std::string&)>(),

        "tonumber", [](UInt64Wrapper& self) { return (std::uint64_t)self; },
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



    // *NOTE: IMPORTANT
    //  See 'version' key
    //      If 'VConstants::GAME' type is changed to anything besides a 'const char*', this breaks compilation
    //      due to oddities with sol::var(...) resolution...
    m_state["Valhalla"] = Valhalla();
    m_state.new_usertype<IValhalla>("IValhalla",
        // server members
        "version", sol::var(VConstants::GAME), // Valheim version
        "delta", sol::property(&IValhalla::Delta),
        "id", sol::property([](IValhalla& self) { return Int64Wrapper(self.ID()); }),
        "nanos", sol::property([](IValhalla& self) { return Int64Wrapper(self.Nanos().count()); }),
        "time", sol::property(&IValhalla::Time),
        "time_multiplier", &IValhalla::m_serverTimeMultiplier,
        // world time functions
        "world_time", sol::property(sol::resolve<WorldTime() const>(&IValhalla::GetWorldTime), &IValhalla::SetWorldTime),
        "world_time_multiplier", sol::property([](IValhalla& self) { return self.m_worldTimeMultiplier; }, [](IValhalla& self, double mul) { if (mul <= 0.001) throw std::runtime_error("multiplier too small"); self.m_worldTimeMultiplier = mul; }),
        "world_ticks", sol::property([](IValhalla& self) { return self.GetWorldTicks(); }),
        "day", sol::property(sol::resolve<int() const>(&IValhalla::GetDay), &IValhalla::SetDay),        
        "time_of_day", sol::property(sol::resolve<TimeOfDay() const>(&IValhalla::GetTimeOfDay), &IValhalla::SetTimeOfDay),
        "is_morning", sol::property(sol::resolve<bool() const>(&IValhalla::IsMorning)),
        "is_day", sol::property(sol::resolve<bool() const>(&IValhalla::IsDay)),
        "is_afternoon", sol::property(sol::resolve<bool() const>(&IValhalla::IsAfternoon)),
        "is_night", sol::property(sol::resolve<bool() const>(&IValhalla::IsNight)),
        "next_morning", sol::property(&IValhalla::GetTomorrowMorning),
        "next_day", sol::property(&IValhalla::GetTomorrowDay),
        "next_afternoon", sol::property(&IValhalla::GetTomorrowAfternoon),
        "next_night", sol::property(&IValhalla::GetTomorrowNight),

        "subscribe", [this](IValhalla& self, sol::variadic_args args) {
            avledet::util::Hash hash = 0;
            sol::function func;
            int priority = 0;

            // If priority is present (will be at end)
            const int offset = args[args.size() - 1].get_type() == sol::type::number ? 2 : 1;

            for (int i = 0; i < args.size(); i++) {
                auto&& arg = args[i];
                auto&& type = arg.get_type();

                if (i + offset < args.size()) {
                    if (type == sol::type::string)
                        hash ^= avledet::util::get_stable_hash(arg.as<std::string>());
                    else if (type == sol::type::number)
                        hash ^= arg.as<avledet::util::Hash>();
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
        "get_zdo", &IZDOManager::GetZDO,
        "some_zdos", sol::overload(
            sol::resolve<std::list<ZDO::unsafe_value>(Vector3f, float, std::size_t, IZDOManager::pred_t)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(Vector3f, float, std::size_t)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(Vector3f, float, std::size_t, avledet::util::Hash prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent)>(&IZDOManager::SomeZDOs),
            [](IZDOManager& self, const Vector3f& pos, float radius, std::size_t max, std::string_view name) { return self.SomeZDOs(pos, radius, max, avledet::util::get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },
            
            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, std::size_t, IZDOManager::pred_t)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, std::size_t)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, std::size_t, avledet::util::Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::SomeZDOs),
            [](IZDOManager& self, const ZoneID& zone, std::size_t max, std::string_view name) { return self.SomeZDOs(zone, max, avledet::util::get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },

            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, std::size_t, Vector3f, float)>(&IZDOManager::SomeZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, std::size_t, Vector3f, float, avledet::util::Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::SomeZDOs),
            [](IZDOManager& self, ZoneID zone, std::size_t max, Vector3f pos, float radius, std::string_view name) { return self.SomeZDOs(zone, max, pos, radius, avledet::util::get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); }
        ),
        "get_zdos", sol::overload(
            sol::resolve<std::list<ZDO::unsafe_value>(IZDOManager::pred_t)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(avledet::util::Hash)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, std::string_view name) { return self.GetZDOs(avledet::util::get_stable_hash(name)); },

            sol::resolve<std::list<ZDO::unsafe_value>(Vector3f, float, IZDOManager::pred_t)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(Vector3f, float)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(Vector3f, float, avledet::util::Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, Vector3f pos, float radius, std::string_view name) { return self.GetZDOs(pos, radius, avledet::util::get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },

            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, IZDOManager::pred_t)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID)>(&IZDOManager::GetZDOs),

            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, avledet::util::Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, ZoneID zone, std::string_view name) { return self.GetZDOs(zone, avledet::util::get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },
            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, Vector3f, float)>(&IZDOManager::GetZDOs),
            sol::resolve<std::list<ZDO::unsafe_value>(ZoneID, Vector3f, float, avledet::util::Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::GetZDOs),
            [](IZDOManager& self, ZoneID zone, Vector3f pos, float radius, std::string_view name) { return self.GetZDOs(zone, pos, radius, avledet::util::get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); }
        ),
        "any_zdo", sol::overload(
            sol::resolve<ZDO::unsafe_optional (Vector3f, float, avledet::util::Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::AnyZDO),
            [](IZDOManager& self, Vector3f pos, float radius, std::string_view name) { return self.AnyZDO(pos, radius, avledet::util::get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); },

            sol::resolve<ZDO::unsafe_optional (ZoneID, avledet::util::Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::AnyZDO),
            [](IZDOManager& self, ZoneID zone, std::string_view name) { return self.AnyZDO(zone, avledet::util::get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); }
        ),
        "nearest_zdo", sol::overload(
            sol::resolve<ZDO::unsafe_optional (Vector3f, float, IZDOManager::pred_t)>(&IZDOManager::NearestZDO),
            sol::resolve<ZDO::unsafe_optional (Vector3f, float, avledet::util::Hash, Prefab::Flag, Prefab::Flag)>(&IZDOManager::NearestZDO),
            [](IZDOManager& self, Vector3f pos, float radius, std::string_view name) { return self.NearestZDO(pos, radius, avledet::util::get_stable_hash(name), Prefab::Flag::NONE, Prefab::Flag::NONE); }
        ),
        "force_send_zdo", &IZDOManager::ForceSendZDO,
        //"DestroyZDO", sol::resolve<ZDO&>(&IZDOManager::DestroyZDO),
        "destroy_zdo", sol::overload(
            sol::resolve<void (ZDOID)>(&IZDOManager::DestroyZDO),
            sol::resolve<void(ZDO::unsafe_value)>(&IZDOManager::DestroyZDO)
        ),
        "instantiate", sol::overload(
            sol::resolve<ZDO::unsafe_value (Prefab const&, Vector3f)>(&IZDOManager::Instantiate),
            [](IZDOManager& self, std::string_view name, Vector3f pos) { return self.Instantiate(avledet::util::get_stable_hash(name), pos); },
            sol::resolve<ZDO::unsafe_value (avledet::util::Hash, Vector3f)>(&IZDOManager::Instantiate)
            //sol::resolve<ZDO (const ZDO)>(&IZDOManager::Instantiate)
        )

    );



    m_state["NetManager"] = NetManager();
    m_state.new_usertype<INetManager>("INetManager",
        "get_peer", sol::overload(
            [](INetManager& self, Int64Wrapper owner) { return self.GetPeerByUserID((std::int64_t)owner); },
            //sol::resolve<Peer*(avledet::util::UserID)>(&INetManager::GetPeer),
            sol::resolve<Peer* (std::string_view)>(&INetManager::GetPeerByName)
        ),
        "peers", sol::readonly(&INetManager::m_onlinePeers)
    );



    m_state["ModManager"] = ModManager();
    m_state.new_usertype<IModManager>("IModManager",
        "get_mod", [](IModManager& self, std::string_view name) {
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


#if VH_IS_ON(VH_ZONE_GENERATION)
    m_state.new_usertype<Dungeon>("Dungeon",
        sol::no_constructor
        //"Generate", sol::resolve<void(const Vector3f& pos, const Quaternion& rot) const>(&Dungeon::Generate)
    );

    m_state["DungeonManager"] = DungeonManager();
    m_state.new_usertype<IDungeonManager>("IDungeonManager",
        "get_dungeon", [](IDungeonManager& self, std::string_view name) { return self.GetDungeon(avledet::util::get_stable_hash(name)); },
        "generate", [](IDungeonManager& self, Dungeon& dungeon, Vector3f pos, Quaternion rot) { self.Generate(dungeon, pos, rot); }
    );



    m_state.new_usertype<IZoneManager::Feature::Instance>("FeatureInstance",
        "pos", sol::property([](IZoneManager::Feature::Instance& self) { return self.m_pos; })
    );
#endif

    m_state["ZoneManager"] = ZoneManager();
    m_state.new_usertype<IZoneManager>("IZoneManager",
#if VH_IS_ON(VH_ZONE_GENERATION)
        "populate_zone", sol::resolve<void(ZoneID)>(&IZoneManager::PopulateZone),
#endif
        "get_nearest_feature", &IZoneManager::GetNearestFeature,
        "to_zone_pos", &IZoneManager::WorldToZonePos,
        "to_world_pos", &IZoneManager::ZoneToWorldPos,
        "global_keys", sol::property(&IZoneManager::GlobalKeys)
    );



    // TODO use properties for immutability
    m_state.new_usertype<Mod>("Mod",
        "name", sol::readonly(&Mod::m_name),
        "version", sol::readonly(&Mod::m_version),
        "api_version", sol::readonly(&Mod::m_apiVersion),
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
        "register", &IRouteManager::RegisterLua,
        "invoke_view", &IRouteManager::InvokeViewLua,
        "invoke", &IRouteManager::InvokeLua,
        "invoke_all", &IRouteManager::InvokeAllLua
    );



    {
        auto eventTable = m_state["event"].get_or_create<sol::table>();

        eventTable["unsubscribe"] = [this]() { this->m_unsubscribeCurrentEvent = false; };
    }

    //TODO logger ref capture; fix
    m_state["print"] = [m_logger = this->m_logger](sol::this_state ts, sol::variadic_args args) {
        sol::state_view state = ts;

        auto&& tostring(state["tostring"]);

        std::string s;
        int idx = 0;
        for (auto&& arg : args) {
            if (idx++ > 0)
                s += " ";
            s += tostring(arg);
        }

        LOG_INFO(m_logger, "[Lua] {}", s);
    };



    m_state.new_usertype<ZStdCompressor>("ZStdCompressor",
        sol::constructors<ZStdCompressor(int), ZStdCompressor(), ZStdCompressor(const avledet::util::Bytes&)>(),
        "compress", sol::resolve<std::optional<avledet::util::Bytes>(const avledet::util::Bytes&)>(&ZStdCompressor::Compress)
        );

    m_state.new_usertype<ZStdDecompressor>("ZStdDecompressor",
        sol::constructors<ZStdDecompressor(), ZStdDecompressor(const avledet::util::Bytes&)>(),
        "decompress", sol::resolve<std::optional<avledet::util::Bytes>(const avledet::util::Bytes&)>(&ZStdDecompressor::Decompress)
        );
    


    m_state.new_usertype<Deflater>("Deflater",
        "gz", sol::property(sol::resolve<Deflater()>(Deflater::Gz)),
        "zlib", sol::property(sol::resolve<Deflater()>(Deflater::ZLib)),
        "raw", sol::property(sol::resolve<Deflater()>(Deflater::Raw)),
        "compress", sol::resolve<std::optional<avledet::util::Bytes>(const avledet::util::Bytes&)>(&Deflater::Compress)
    );

    m_state.new_usertype<Inflater>("Inflater",
        //"any", sol::property(Inflater::Any),
        "zlib", sol::property(Inflater::Gz),
        "gz", sol::property(Inflater::Gz),
        "auto", sol::property(Inflater::Auto),
        "raw", sol::property(Inflater::Raw),
        "decompress", sol::resolve<std::optional<avledet::util::Bytes>(const avledet::util::Bytes&)>(&Inflater::Decompress)
    );



    {
        auto utilsTable = m_state["VUtils"].get_or_create<sol::table>();

        utilsTable["create_bytes"] = []() { return avledet::util::Bytes(); };

        utilsTable["assign"] = sol::overload(
            [](avledet::util::Bytes& replace, avledet::util::Bytes& other) { replace = other; }
        );

        utilsTable["swap"] = sol::overload(
            [](avledet::util::Bytes& a, avledet::util::Bytes& b) { std::swap(a, b); }
        );

        //utilsTable["Move"] = sol::overload(
        //    [](avledet::util::Bytes& a, avledet::util::Bytes& b) { std::swap(a, b); }
        //);

        {
            auto stringUtilsTable = utilsTable["String"].get_or_create<sol::table>();

            //TODO
            //stringUtilsTable["GetStableHashCode"] = avledet::util::get_stable_hash;
        }

        {
            auto resourceUtilsTable = utilsTable["Resource"].get_or_create<sol::table>();
            
            //resourceUtilsTable["ReadFileBytes"] = sol::resolve<std::optional<avledet::util::Bytes>(const fs::path&)>(VUtils::Resource::ReadFile);
            //resourceUtilsTable["ReadFileString"] = sol::resolve<std::optional<std::string>(const fs::path&)>(VUtils::Resource::ReadFile);
            //resourceUtilsTable["ReadFileLines"] = sol::resolve<std::optional<std::vector<std::string>>(const fs::path&, bool)>(VUtils::Resource::ReadFile);
            
            resourceUtilsTable["as_bytes"] = [](std::string_view path) { return VUtils::Resource::ReadFile<avledet::util::Bytes>(path); };
            resourceUtilsTable["as_string"] = [](std::string_view path) { return VUtils::Resource::ReadFile<std::string>(path); };
            resourceUtilsTable["as_lines"] = [](std::string_view path) { return VUtils::Resource::ReadFile<std::vector<std::string>>(path); };

            resourceUtilsTable["write_file"] = sol::overload(
                sol::resolve<bool(const fs::path&, const avledet::util::Bytes&)>(VUtils::Resource::WriteFile),
                sol::resolve<bool(const fs::path&, std::string_view)>(VUtils::Resource::WriteFile),
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



//TODO
//inline void my_panic(sol::optional<std::string> maybe_msg) {
//    LOG_ERROR(m_logger, "Lua is in a panic state and will now abort() the application");
//    if (maybe_msg) {
//        const std::string& msg = maybe_msg.value();
//        LOG_ERROR(m_logger, "\terror message: {}", msg);
//    }
//    // When this function exits, Lua will exhibit default behavior and abort()
//}

//in my limited usage and experience, no error handler or panic was ever invoked, perhaps because all
//  errors took place INSIDE one of my event handlers...
//int my_exception_handler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
//    // L is the lua state, which you can wrap in a state_view if necessary
//    // maybe_exception will contain exception, if it exists
//    // description will either be the what() of the exception or a description saying that we hit the general-case catch(...)
//    LOG_ERROR(m_logger, "An exception occurred in a function, here's what it says ");
//    if (maybe_exception) {
//        LOG_ERROR(m_logger, "(straight from the exception): ");
//        const std::exception& ex = *maybe_exception;
//        LOG_ERROR(m_logger, "{}", ex.what());
//    }
//    else {
//        LOG_ERROR(m_logger, "(from the description parameter): ");
//        LOG_ERROR(m_logger, "{}", description);
//    }
//
//    // you must push 1 element onto the stack to be
//    // transported through as the error object in Lua
//    // note that Lua -- and 99.5% of all Lua users and libraries -- expects a string
//    // so we push a single string (in our case, the description of the error)
//    return sol::stack::push(L, description);
//}

void IModManager::PostInit() {
    m_logger = quill::Frontend::create_or_get_logger("modmanager", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"));

    LOG_INFO(m_logger, "Initializing ModManager");

    //m_state.set_exception_handler(&my_exception_handler);

    m_state.open_libraries();

    this->LoadAPI();

    std::error_code ec;
    fs::create_directories(VH_MOD_PATH, ec);
    
    if (ec)
        return;

    for (const auto& dir
        : fs::directory_iterator(VH_MOD_PATH, ec)) {

        try {
            if (dir.exists(ec) && dir.is_directory(ec)) {
                auto&& dirname = dir.path().filename().string();

                if (dirname.starts_with("--"))
                    continue;

                auto&& mod = LoadModInfo(dirname);
                LoadMod(mod);

                LOG_INFO(m_logger, "Loaded mod '{}'", mod.m_name);
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR(m_logger, "Failed to load mod: {} ({})", e.what(), dir.path().string());
        }
    }

    LOG_INFO(m_logger, "Loaded {} mods", m_mods.size());

    VH_DISPATCH_MOD_EVENT(IModManager::Events::Enable);
}

void IModManager::Uninit() {
    VH_DISPATCH_MOD_EVENT(IModManager::Events::Disable);
    m_callbacks.clear();
    m_mods.clear();
}

#endif // VH_USE_MODS