#include <sol/sol.hpp>
#include <easylogging++.h>
#include <optick.h>

#include "ModManager.h"
#include "NetManager.h"
#include "VServer.h"
#include "ResourceManager.h"
#include "NetRpc.h"
#include "NetHashes.h"

std::unique_ptr<VModManager> VModManager_INSTANCE = std::make_unique<VModManager>();
VModManager* ModManager() {
    return VModManager_INSTANCE.get();
}

bool VModManager::EventHandlerSort(const EventHandler &a,
                             const EventHandler &b) {
    return a.m_priority < b.m_priority;
}



// can get the lua state by passing sol::this_state type at end
// https://sol2.readthedocs.io/en/latest/tutorial/functions.html#any-return-to-and-from-lua

// Load a Lua script starting in relative root 'data/mods/'
sol::state RunLuaFrom(const fs::path& luaPath) {
    std::string modCode;
    if (!ResourceManager::ReadFileBytes(fs::path("mods") / luaPath, modCode)) {
        throw std::runtime_error(std::string("Failed to open Lua file ") + luaPath.string());
    }

    auto state = sol::state();

    state.open_libraries();
    state.script(modCode);

    return state;
}

void RunLuaFrom(sol::state &state, const fs::path& luaPath) {
    std::string modCode;
    if (!ResourceManager::ReadFileBytes(fs::path("mods") / luaPath, modCode)) {
        throw std::runtime_error(std::string("Failed to open Lua file ") + luaPath.string());
    }

    state.script(modCode);
}

void VModManager::RunModInfoFrom(const fs::path& dirname,
                    std::string& outName,
                    std::string& outVersion,
                    int &outApiVersion,
                    std::string& outEntry) {
    auto&& state = RunLuaFrom(dirname / "modInfo.lua");

    auto modInfo = state["modInfo"];

    // required
    sol::optional<std::string> name = modInfo["name"]; // str
    sol::optional<std::string> version = modInfo["version"]; // str
    sol::optional<int> apiVersion = modInfo["apiVersion"];
    sol::optional<std::string> entry = modInfo["entry"];

    if (!(name && version && apiVersion && entry)) {
        throw std::runtime_error("incomplete modInfo");
    }

    if (mods.contains(name.value())) {
        throw std::runtime_error("tried registering mod with duplicate name");
    }

    outName = name.value();
    outVersion = version.value();
    outApiVersion = apiVersion.value();
    outEntry = entry.value();
}



std::unique_ptr<VModManager::Mod> VModManager::PrepareModEnvironment(const std::string& name,
                                           std::string& version,
                                           int apiVersion) {
    auto mod = std::make_unique<Mod>(name, version, apiVersion);
    auto ptr = mod.get();

    auto&& state = mod->m_state;
    state.open_libraries();

    state.globals()["print"] = [ptr](sol::variadic_args args) {
        auto tostring(ptr->m_state["tostring"]);

        std::string s;
        int idx = 0;
        for (auto&& arg : args) {
            if (idx++ > 0)
                s += " ";
            s += tostring(arg);
        }

        LOG(INFO) << "[" << ptr->m_name << "] " << s;
    };

    //state.new_usertype<Task>("Task",
    //	"at", &Task::at,
    //	"Cancel", &Task::Cancel,
    //	"period", &Task::period,
    //	"Repeats", &Task::Repeats,
    //	"function", &Task::function); // dummy

    // https://sol2.readthedocs.io/en/latest/api/property.html
    // pass the mod later when calling lua-side functions
    // https://github.com/LaG1924/AltCraft/blob/267eeeb94ad4863683dc96dc392e0e30ef16a39e/src/Plugin.cpp#L242
    state.new_usertype<NetPeer>("NetPeer",
        "Kick", &NetPeer::Kick,
        "characterID", &NetPeer::m_characterID,
        "name", &NetPeer::m_name,
        "visibleOnMap", &NetPeer::m_visibleOnMap,
        "pos", &NetPeer::m_pos,
        //"Rpc", [](NetPeer& self) { return self.m_rpc.get(); }, // &NetPeer::m_rpc, // [](sol::this_state L) {},
        "rpc", sol::property([](NetPeer& self) { return self.m_rpc.get(); }),
        //[](NetPeer& self) { return self.m_rpc.get(); }, // &NetPeer::m_rpc, // [](sol::this_state L) {},
        "uuid", &NetPeer::m_uuid);

    state.new_enum("PkgType",
                   "BYTE_ARRAY", PkgType::BYTE_ARRAY,
                   "PKG", PkgType::PKG,
                   "STRING", PkgType::STRING,
                   "NET_ID", PkgType::NET_ID,
                   "VECTOR3", PkgType::VECTOR3,
                   "VECTOR2i", PkgType::VECTOR2i,
                   "QUATERNION", PkgType::QUATERNION,
                   "STRING_ARRAY", PkgType::STRING_ARRAY,
                   "INT8", PkgType::INT8, "UINT8", PkgType::UINT8,
                        "CHAR", PkgType::INT8, "UCHAR", PkgType::UINT8, "BYTE", PkgType::UINT8, "BOOL", PkgType::UINT8,
                   "INT16", PkgType::INT16, "UINT16", PkgType::UINT16,
                        "SHORT", PkgType::INT16, "USHORT", PkgType::UINT16,
                   "INT32", PkgType::INT32, "UINT32", PkgType::UINT32,
                        "INT", PkgType::INT32, "UINT", PkgType::UINT32,
                   "INT64", PkgType::INT64, "UINT64", PkgType::UINT64,
                        "LONG", PkgType::INT64, "ULONG", PkgType::UINT64, "OWNER_t", PkgType::UINT64,
                   "FLOAT", PkgType::FLOAT,
                   "DOUBLE", PkgType::DOUBLE
                   );

    // To register an RPC, must pass a lua stack handler
    // basically, get the lua state to get args passed to RPC invoke
    // https://github.com/ThePhD/sol2/issues/471#issuecomment-320259767
    state.new_usertype<NetRpc>("NetRpc",
        //"Invoke", &NetRpc::Invoke,
        //"Register", [](NetRpc& self, sol::variadic_args args) {
        //    LOG(INFO) << "Lua Register call args:";
        //    for (auto&& arg : args) {
        //        LOG(INFO) << arg.as<std::string>();
        //    }
        //},

        // rpc:Register("rpcname", function(pkg)
        //     print("Received rpcname!")
        // end

        // rpc:Register("rpcname", PkgType.BOOL, function(enabled)
        //     print("Received rpcname!")
        // end

        "Register", [](NetRpc& self, sol::variadic_args args) {

            HASH_t name = 0; //= Utils::GetStableHashCode(args[0].as<std::string>());
            std::vector<PkgType> types;

            for (int i=0; i < args.size(); i++) {
                auto &&arg = args[i];
                auto &&type = arg.get_type();

                if (i == 0) {
                    if (type == sol::type::string) {
                        name = Utils::GetStableHashCode(arg.as<std::string>());
                    } else if (type == sol::type::number) {
                        name = arg.as<HASH_t>();
                    } else {
                        LOG(ERROR) << "Lua invalid register call";
                        break;
                    }
                } else if (i + 1 < args.size()) {
                    // grab middle pkgtypes
                    types.push_back(arg.as<PkgType>());
                } else {
                    if (type == sol::type::function) {
                        auto callback = arg.as<sol::function>();

                        self.Register(name,
                                      std::make_unique<MethodImpl<NetRpc&>>(callback, std::move(types)));
                    } else {
                        LOG(ERROR) << "Last param must be a function";
                    }
                    break;
                }

            }
        },
        "socket", sol::property([](NetRpc& self) { return self.m_socket; })
    );

    // https://sol2.readthedocs.io/en/latest/api/usertype.html#inheritance-example
    state.new_usertype<ISocket>("ISocket",
        "Close", &ISocket::Close,
        "GetHostName", &ISocket::GetHostName,
        "GetAddress", &ISocket::GetAddress,
        "Connected", &ISocket::Connected,
        "GetSendQueueSize", &ISocket::GetSendQueueSize
        //"HasNewData", &ISocket::HasNewData,
        //"Recv", &ISocket::Recv,
        //"Send", &ISocket::Send,
        //"Start", &ISocket::Init
    );

    state.new_usertype<NetPackage>("NetPackage"
        //"", &NetPackage::
    );

    //state.new_usertype<BYTES_t>("Bytes")

    state.new_usertype<NetID>("NetID",
        "uuid", &NetID::m_uuid,
        "id", &NetID::m_id
    );

    state.new_usertype<Vector3>("Vector3",
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

    state.new_usertype<Vector2i>("Vector3",
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

    state.new_usertype<Quaternion>("Quaternion",
        "x", &Quaternion::x,
        "y", &Quaternion::y,
        "z", &Quaternion::z,
        "w", &Quaternion::w
        //"IDENTITY", &Quaternion::IDENTITY
    );



    auto apiTable = state["Valhalla"].get_or_create<sol::table>();

    apiTable["Version"] = VALHEIM_VERSION;

    // method overloading is easy
    // https://sol2.readthedocs.io/en/latest/api/overload.html

    apiTable["OnEvent"] = [this, ptr](sol::this_state thisState, sol::variadic_args args) {
        // match incrementally
        //std::string name;
        HASH_t name = 0;
        sol::function callback;
        int priority = 0;
        for (int i=0; i < args.size(); i++) {
            auto&& arg = args[i];
            auto&& type = arg.get_type();
            if (type == sol::type::string) {
                auto h = Utils::GetStableHashCode(arg.as<std::string>());
                if (name == 0) name = h;
                else name ^= h;
            } else {
                if (type == sol::type::number) {
                    priority = arg.as<int>();
                }
                // check next element forcefully, it must be a function
                if (i + 1 == args.size()) {
                    if (type == sol::type::function) {
                        callback = arg.as<sol::function>();
                        break;
                    }
                }
            }
        }

        if (name != 0 && callback.valid()) {
            auto &&vec = m_callbacks[name];
            vec.emplace_back(EventHandler{ptr, callback, priority});
            std::sort(vec.begin(), vec.end(), EventHandlerSort);
        } else {
            lua_State *L = thisState.L;

            lua_Debug ar;
            lua_getstack(L, 1, &ar);
            lua_getinfo(L, "nSl", &ar);
            auto line = ar.currentline;

            LOG(ERROR) << "Failed to register Lua event callback for Mod '" << ptr->m_name << "' Line " << line;
        }
    };

    // Get information about this mod
    {
        auto thisModTable = state["Mod"].get_or_create<sol::table>();
        thisModTable["name"] = ptr->m_name;
        thisModTable["version"] = ptr->m_version;
        thisModTable["apiVersion"] = ptr->m_apiVersion;
    }

    // Get information about the current event
    {
        auto thisEventTable = state["Event"].get_or_create<sol::table>();
        thisEventTable["SetCancelled"] = [this](bool c) { m_cancelCurrentEvent = c; };
        thisEventTable["Cancelled"] = [this]() { return m_cancelCurrentEvent; };
    }

    return mod;
}



void VModManager::Init() {
    for (const auto& dir
        : fs::directory_iterator(ResourceManager::GetPath("mods"))) {

        try {
            if (dir.exists() && dir.is_directory()) {
                auto&& dirname = dir.path().filename();

                if (dirname.string().starts_with("--"))
                    continue;

                std::string name, version, entry;
                int apiVersion;
                RunModInfoFrom(dirname, name, version, apiVersion, entry);
                auto mod = PrepareModEnvironment(name, version, apiVersion);
                RunLuaFrom(mod->m_state, dirname / (entry + ".lua"));

                mods.insert({ name, std::move(mod) });

                LOG(INFO) << "Loaded mod '" << name << "'";
            }
        }
        catch (const std::exception& e) {
            LOG(ERROR) << "Failed to load mod: " << e.what();
        }
    }

    LOG(INFO) << "Loaded " << mods.size() << " mods";

    CallEvent("Enable");
}

void VModManager::UnInit() {
    CallEvent("Disable");
    m_callbacks.clear();
    mods.clear();

    VModManager_INSTANCE.reset();
}
