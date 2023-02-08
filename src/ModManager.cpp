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
#include "NetID.h"
#include "ValhallaServer.h"
#include "NetSocket.h"
#include "ZDOManager.h"

auto MOD_MANAGER(std::make_unique<IModManager>());
IModManager* ModManager() {
    return MOD_MANAGER.get();
}

std::unique_ptr<IModManager::Mod> IModManager::LoadModInfo(const std::string& folderName,
    std::string& outEntry) {

    YAML::Node loadNode;

    auto path = fs::path("mods") / folderName / "modInfo.yml";

    if (auto opt = VUtils::Resource::ReadFileString(path)) {
        loadNode = YAML::Load(opt.value());
    }
    else {
        throw std::runtime_error(std::string("unable to open ") + path.string());
    }

    outEntry = loadNode["entry"].as<std::string>();

    return std::make_unique<Mod>(
        loadNode["name"].as<std::string>(), sol::environment(m_state, sol::create, m_state.globals()));
}

void IModManager::LoadModEntry(Mod* mod) {

    // load all mods from file

    auto&& env = mod->m_env;

    env["print"] = [this, mod](sol::variadic_args args) {
        auto&& tostring(m_state["tostring"]);

        std::string s;
        int idx = 0;
        for (auto&& arg : args) {
            if (idx++ > 0)
                s += " ";
            s += tostring(arg);
        }

        LOG(INFO) << "[" << mod->m_name << "] " << s;
    };

    auto utilsTable = env["VUtils"].get_or_create<sol::table>();

    utilsTable["Compress"] = sol::overload(
        sol::resolve<std::optional<BYTES_t>(const BYTES_t&)>(VUtils::CompressGz),
        sol::resolve<std::optional<BYTES_t>(const BYTES_t&, int)>(VUtils::CompressGz)
    );
    utilsTable["Decompress"] = sol::resolve<std::optional<BYTES_t>(const BYTES_t&)>(VUtils::Decompress);
    utilsTable["Decompress"] = sol::resolve<std::optional<BYTES_t>(const BYTES_t&)>(VUtils::Decompress);

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

    env.new_usertype<Peer>("Peer",
        "Kick", static_cast<void (Peer::*)(bool)>(&Peer::Kick),
        "Kick", static_cast<void (Peer::*)(std::string)>(&Peer::Kick),
        "Disconnect", &Peer::Disconnect,
        //"Invoke", []() {},
        //"Register", &Peer::Register,
        "characterID", &Peer::m_characterID,
        "name", &Peer::m_name,
        "visibleOnMap", &Peer::m_visibleOnMap,
        "pos", &Peer::m_pos,
        "uuid", &Peer::m_uuid);

    env.new_enum("DataType",
        "bytes",    DataType::BYTES,
        "string",   DataType::STRING,
        "zdoid",    DataType::ZDOID,
        "vector3",     DataType::VECTOR3,
        "vector2i",    DataType::VECTOR2i,
        "quaternion",     DataType::QUATERNION,
        "strings",  DataType::STRINGS,
        "bool",     DataType::BOOL,
        "byte",     DataType::INT8,         "int8", DataType::INT8,
        "short",    DataType::INT16,        "int16", DataType::INT16,
        "int",      DataType::INT32,        "int32", DataType::INT32,       "hash", DataType::INT32,
        "long",     DataType::INT64,        "int64", DataType::INT64,       
        "float",    DataType::FLOAT,
        "double",   DataType::DOUBLE
    );

    env.new_usertype<DataWriter>("DataWriter",
        "provider", &DataWriter::m_provider,
        "pos", &DataWriter::m_pos,
        "Write", sol::overload(
            // templated functions are too complex for resolve
            // https://github.com/ThePhD/sol2/issues/664#issuecomment-396867392
            static_cast<void (DataWriter::*)(const BYTES_t&, size_t)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const BYTES_t&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const std::string&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const NetID&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const Vector3&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const Vector2i&)>(&DataWriter::Write),
            static_cast<void (DataWriter::*)(const Quaternion&)>(&DataWriter::Write),
            [](DataWriter& self, std::vector<std::string> in) { self.Write(in); },
            [mod](DataWriter& self, DataType type, LUA_NUMBER val) {
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
                    LOG(ERROR) << "invalid PkgType enum, got: " << std::to_underlying(type);
                }
            })
    );

    // Package read/write types
    env.new_usertype<DataReader>("DataReader",
        "provider", &DataWriter::m_provider,
        "pos", &DataWriter::m_pos,
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
            case DataType::BOOL:
                return sol::make_object(state, self.Read<bool>());
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
                LOG(WARNING) << "invalid DataType enum, got: " << std::to_underlying(type);
                return sol::make_object(state, sol::nil);
            }
        }
    );

    env.new_usertype<NetID>("NetID",
        "uuid", &NetID::m_uuid,
        "id", &NetID::m_id
    );

    env.new_usertype<Vector3>("Vector3",
        "x", &Vector3::x,
        "y", &Vector3::y,
        "z", &Vector3::z,
        "Distance", &Vector3::Distance,
        "Magnitude", &Vector3::Magnitude,
        "Normalize", &Vector3::Normalize,
        "Normalized", &Vector3::Normalized,
        "SqDistance", &Vector3::SqDistance,
        "SqMagnitude", &Vector3::SqMagnitude
        //"ZERO", &Vector3::ZERO
    );

    env.new_usertype<Vector2i>("Vector2i",
        "x", &Vector2i::x,
        "y", &Vector2i::y,
        "Distance", &Vector2i::Distance,
        "Magnitude", &Vector2i::Magnitude,
        "Normalize", &Vector2i::Normalize,
        "Normalized", &Vector2i::Normalized,
        "SqDistance", &Vector2i::SqDistance,
        "SqMagnitude", &Vector2i::SqMagnitude
        //"ZERO", &Vector2i::ZERO
    );

    env.new_usertype<Quaternion>("Quaternion",
        "x", &Quaternion::x,
        "y", &Quaternion::y,
        "z", &Quaternion::z,
        "w", &Quaternion::w
        //"IDENTITY", &Quaternion::IDENTITY
    );

    // https://sol2.readthedocs.io/en/latest/api/usertype.html#inheritance-example
    env.new_usertype<ISocket>("ISocket",
        "Close", &ISocket::Close,
        "Connected", &ISocket::Connected,
        "GetAddress", &ISocket::GetAddress,
        "GetHostName", &ISocket::GetHostName,
        "GetSendQueueSize", &ISocket::GetSendQueueSize
    );



    auto apiTable = env["Valhalla"].get_or_create<sol::table>();
    apiTable["ServerVersion"] = SERVER_VERSION;
    apiTable["ValheimVersion"] = VConstants::GAME;
    apiTable["Delta"] = []() { return Valhalla()->Delta(); };
    apiTable["ID"] = []() { return Valhalla()->ID(); };
    apiTable["Nanos"] = []() { return Valhalla()->Nanos(); };
    apiTable["Ticks"] = []() { return Valhalla()->Ticks(); };
    apiTable["Time"] = []() { return Valhalla()->Time(); };

    auto zdoApiTable = env["ZDOManager"].get_or_create<sol::table>();
    zdoApiTable["GetZDOs"] = [](HASH_t hash) { return ZDOManager()->GetZDOs(hash); };



    apiTable["OnEvent"] = [this, mod](sol::this_state thisState, sol::variadic_args args) {
        // match incrementally
        //std::string name;
        HASH_t cbHash = 0;
        int priority = 0;
        for (int i = 0; i < args.size(); i++) {
            auto&& arg = args[i];
            auto&& type = arg.get_type();

            if (i + 1 < args.size()) {
                HASH_t hash;
                if (type == sol::type::string)
                    hash = VUtils::String::GetStableHashCode(arg.as<std::string>());
                else if (type == sol::type::number)
                    hash = arg.as<HASH_t>();
                else {
                    LOG(WARNING) << "LUA starting parameters must be string or hash";
                    return;
                }

                cbHash ^= hash;
            }
            else {
                if (type == sol::type::function) {
                    auto&& vec = m_callbacks[cbHash];
                    vec.emplace_back( 
                        mod, 
                        arg.as<sol::function>(), 
                        priority
                    );
                    std::sort(vec.begin(), vec.end(), [](const EventHandler& a,
                        const EventHandler& b) {
                        return a.m_priority < b.m_priority;
                    });
                }
                else {
                    LOG(WARNING) << "LUA last param must be a function";
                    return;
                }
            }
        }
    };

}

void IModManager::Init() {
    LOG(INFO) << "Initializing ModManager";

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

    for (const auto& dir
        : fs::directory_iterator("mods")) {

        try {
            if (dir.exists() && dir.is_directory()) {
                auto&& dirname = dir.path().filename().string();

                if (dirname.starts_with("--"))
                    continue;

                std::string entry;
                auto mod = LoadModInfo(dirname, entry);

                LoadModEntry(mod.get());

                auto path(fs::path("mods") / dirname / (entry + ".lua"));
                if (auto opt = VUtils::Resource::ReadFileString(path))
                    m_state.script(opt.value(), mod->m_env);
                else
                    throw std::runtime_error(std::string("unable to open file ") + path.string());

                LOG(INFO) << "Loaded mod '" << mod->m_name << "'";

                mods.insert({ mod->m_name, std::move(mod) });
            }
        }
        catch (const std::exception& e) {
            LOG(ERROR) << "Failed to load mod: " << e.what() << " (" << dir.path().c_str() << ")";
        }
    }

    LOG(INFO) << "Loaded " << mods.size() << " mods";

    CallEvent("Enable");
}

void IModManager::Uninit() {
    CallEvent("Disable");
    m_callbacks.clear();
    mods.clear();
}
