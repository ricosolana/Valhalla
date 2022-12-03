#include <sol/sol.hpp>
#include <easylogging++.h>
#include <optick.h>

#include "ModManager.h"
#include "NetManager.h"
#include "VServer.h"
#include "VUtilsResource.h"
#include "NetRpc.h"
#include "NetHashes.h"



// static definitions
robin_hood::unordered_map<std::string, std::unique_ptr<ModManager::Mod>> ModManager::mods;
robin_hood::unordered_map<HASH_t, std::vector<ModManager::EventHandler>> ModManager::m_callbacks;
EventStatus ModManager::m_eventStatus;



int GetCurrentLuaLine(lua_State* L) {
    lua_Debug ar;
    lua_getstack(L, 1, &ar);
    lua_getinfo(L, "nSl", &ar);

    return ar.currentline;
}

int LoadFileRequire(lua_State* L) {
    std::string path = sol::stack::get<std::string>(L);

    // load locally to the file
    if (auto opt = VUtils::Resource::ReadFileBytes("mods/" + path))
        luaL_loadbuffer(L, reinterpret_cast<const char*>(opt.value().data()), opt.value().size(), path.c_str());
    else
        sol::stack::push(L, "Module '" + path + "' not found");

    return 1;
}

// can get the lua state by passing sol::this_state type at end
// https://sol2.readthedocs.io/en/latest/tutorial/functions.html#any-return-to-and-from-lua

// Load a Lua script starting in relative root 'data/mods/'
void RunStateFrom(sol::state &state, const std::string& luaPath) {
    if (auto opt = VUtils::Resource::ReadFileString("mods/" + luaPath))
        state.script(opt.value());
    else
        throw std::runtime_error(std::string("Failed to open Lua file ") + luaPath);
}

sol::state NewStateFrom(const std::string& luaPath) {
    sol::state state;

    RunStateFrom(state, luaPath);

    return state;
}



bool ModManager::EventHandlerSort(const EventHandler& a,
    const EventHandler& b) {
    return a.m_priority < b.m_priority;
}

void ModManager::Mod::Throw(const char* msg) {
    LOG(ERROR) << m_name << " mod error, line " << GetCurrentLuaLine(m_state.lua_state()) << ", " << msg;
}

void ModManager::RunModInfoFrom(const std::string& dirname,
                    std::string& outName,
                    std::string& outVersion,
                    int &outApiVersion,
                    std::string& outEntry) {
    auto&& state = NewStateFrom(dirname + "/modInfo.lua");
    //m_callbacks[7][0].m_func.
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

std::unique_ptr<ModManager::Mod> ModManager::PrepareModEnvironment(
        const std::string& name,
        const std::string& version,
        int apiVersion) {

    auto mod = std::make_unique<Mod>(name, version, apiVersion);
    auto ptr = mod.get();



    static char errBuf[128] = "";

    auto&& state = mod->m_state;
    //state.open_libraries();
    state.open_libraries(
        sol::lib::base,
        sol::lib::debug,
        sol::lib::io, // override
        sol::lib::math,
        sol::lib::package, // override
        sol::lib::string,
        sol::lib::table,
        sol::lib::utf8
    );
    
    //state["loadfile"]

    //auto&& tostring(ptr->m_state["tostring"]);
    //for (auto&& s : state) {
    //    LOG(INFO) << "Global: " << tostring(s.first).get<std::string>();
    //}

    state["package"]["searchers"] = state.create_table_with(
        1, LoadFileRequire
    );

    state["print"] = [ptr](sol::variadic_args args) {
        auto &&tostring(ptr->m_state["tostring"]);

        std::string s;
        int idx = 0;
        for (auto&& arg : args) {
            if (idx++ > 0)
                s += " ";
            s += tostring(arg);
        }

        LOG(INFO) << "[" << ptr->m_name << "] " << s;
    };

    {
        auto utilsTable = state["VUtils"].get_or_create<sol::table>();

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

            resourceUtilsTable["ReadFileBytes"] = [](const std::string &path) { return VUtils::Resource::ReadFileBytes(path); };
            resourceUtilsTable["ReadFileString"] = [](const std::string& path) { return VUtils::Resource::ReadFileString(path); };
            resourceUtilsTable["ReadFileLines"] = [](const std::string& path) { return VUtils::Resource::ReadFileLines(path); };

            resourceUtilsTable["WriteFileBytes"] = [](const std::string& path, const BYTES_t& bytes) { return VUtils::Resource::WriteFileBytes(path, bytes); };
            resourceUtilsTable["WriteFileString"] = [](const std::string& path, const std::string& s) { return VUtils::Resource::WriteFileString(path, s); };
            resourceUtilsTable["WriteFileLines"] = [](const std::string& path, const std::vector<std::string>& lines) { return VUtils::Resource::WriteFileLines(path, lines); };

            /*
            resourceUtilsTable["ReadFileBytes"] = sol::resolve<std::optional<BYTES_t>(const std::string&)>(VUtils::Resource::ReadFileBytes);
            resourceUtilsTable["ReadFileString"] = sol::resolve<std::optional<std::string>(const std::string&)>(VUtils::Resource::ReadFileString);
            resourceUtilsTable["ReadFileLines"] = sol::resolve<std::optional<std::vector<std::string>>(const std::string&)>(VUtils::Resource::ReadFileLines);

            resourceUtilsTable["WriteFileBytes"] = sol::resolve<bool(const std::string&, const BYTES_t&)>(VUtils::Resource::WriteFileBytes);
            resourceUtilsTable["WriteFileString"] = sol::resolve<bool(const std::string&, const std::string&)>(VUtils::Resource::WriteFileString);
            resourceUtilsTable["WriteFileLines"] = sol::resolve<bool(const std::string&, const std::vector<std::string>&)>(VUtils::Resource::WriteFileLines);
            */
        }
    }

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
                   "BYTE_ARRAY", PkgType::BYTE_ARRAY, "BYTES", PkgType::BYTE_ARRAY,
                   "PKG", PkgType::PKG, "PACKAGE", PkgType::PKG,
                   "STRING", PkgType::STRING,
                   "NET_ID", PkgType::NET_ID,
                   "VECTOR3", PkgType::VECTOR3,
                   "VECTOR2i", PkgType::VECTOR2i,
                   "QUATERNION", PkgType::QUATERNION,
                   "STRING_ARRAY", PkgType::STRING_ARRAY, "STRINGS", PkgType::STRING_ARRAY,
                   "BOOL", PkgType::BOOL,
                   "INT8", PkgType::INT8, "UINT8", PkgType::UINT8,
                        "CHAR", PkgType::INT8, "UCHAR", PkgType::UINT8, "BYTE", PkgType::UINT8,
                   "INT16", PkgType::INT16, "UINT16", PkgType::UINT16,
                        "SHORT", PkgType::INT16, "USHORT", PkgType::UINT16,
                   "INT32", PkgType::INT32, "UINT32", PkgType::UINT32,
                        "INT", PkgType::INT32, "UINT", PkgType::UINT32,
                        "HASH_t", PkgType::INT32,
                   "INT64", PkgType::INT64, "UINT64", PkgType::UINT64,
                        "LONG", PkgType::INT64, "ULONG", PkgType::UINT64, "OWNER_t", PkgType::UINT64,
                   "FLOAT", PkgType::FLOAT,
                   "DOUBLE", PkgType::DOUBLE
    );

    state.new_usertype<Stream>("Stream",
        "pos", sol::property(
                [](Stream& self) { return self.Position(); },
                [](Stream& self, uint32_t pos) { self.SetPos(pos); }),
        "buf", sol::property(
                [](Stream& self) { return std::ref(self.m_buf); },
                [](Stream& self, decltype(Stream::m_buf) buf) { self.m_buf = std::move(buf); })
    );
    
    // Package read/write types
    state.new_usertype<NetPackage>("NetPackage",
        "stream", &NetPackage::m_stream,
        "Write", sol::overload(
            // templated functions are too complex for resolve
            // https://github.com/ThePhD/sol2/issues/664#issuecomment-396867392
            static_cast<void (NetPackage::*)(const BYTES_t&, uint32_t)>(&NetPackage::Write),
            static_cast<void (NetPackage::*)(const BYTES_t&)>(&NetPackage::Write),
            static_cast<void (NetPackage::*)(const NetPackage&)>(&NetPackage::Write),
            static_cast<void (NetPackage::*)(const std::string&)>(&NetPackage::Write),
            static_cast<void (NetPackage::*)(const NetID&)>(&NetPackage::Write),
            static_cast<void (NetPackage::*)(const Vector3&)>(&NetPackage::Write),
            static_cast<void (NetPackage::*)(const Vector2i&)>(&NetPackage::Write),
            static_cast<void (NetPackage::*)(const Quaternion&)>(&NetPackage::Write),
            [](NetPackage& self, std::vector<std::string> in) { self.Write(in); },
            //static_cast<void (NetPackage::*)(const std::vector<std::string>&)>(&NetPackage::Write),
            [ptr](NetPackage& self, PkgType pkgType, LUA_NUMBER val) {
                switch (pkgType) {
                case PkgType::INT8:
                    self.Write<int8_t>(val);
                    break;
                case PkgType::UINT8:
                    self.Write<uint8_t>(val);
                    break;
                case PkgType::INT16:
                    self.Write<int16_t>(val);
                    break;
                case PkgType::UINT16:
                    self.Write<uint16_t>(val);
                    break;
                case PkgType::INT32:
                    self.Write<int32_t>(val);
                    break;
                case PkgType::UINT32:
                    self.Write<uint32_t>(val);
                    break;
                case PkgType::INT64:
                    self.Write<int64_t>(val);
                    break;
                case PkgType::UINT64:
                    self.Write<uint64_t>(val);
                    break;
                case PkgType::FLOAT:
                    self.Write<float>(val);
                    break;
                case PkgType::DOUBLE:
                    self.Write<double>(val);
                    break;
                default:
                    snprintf(errBuf, sizeof(errBuf), "invalid PkgType enum, got: %d",
                        static_cast<std::underlying_type_t<decltype(pkgType)>>(pkgType));
                    return ptr->Throw(errBuf);
                }
            })
    );

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

    // To register an RPC, must pass a lua stack handler
    // basically, get the lua state to get args passed to RPC invoke
    // https://github.com/ThePhD/sol2/issues/471#issuecomment-320259767
    state.new_usertype<NetRpc>("NetRpc",
        "Invoke", [ptr](NetRpc& self, sol::variadic_args args) {
            auto &&tostring(ptr->m_state["tostring"]);

            NetPackage params;
            for (int i=0; i < args.size(); i++) {
                auto &&arg = args[i];
                auto &&type = arg.get_type();

                

                if (i == 0) {
                    if (type == sol::type::string) {
                        params.Write(VUtils::String::GetStableHashCode(arg.as<std::string>()));
                    } else if (type == sol::type::number) {
                        params.Write(arg.as<HASH_t>());
                    } else {
                        sol::object o = tostring(arg);
                        auto result = o.as<std::string>();
                        snprintf(errBuf, sizeof(errBuf), "arg %d; expected string or number, got: %s", i, result.c_str());
                        return ptr->Throw(errBuf);
                    }
                } else {
                    // strict assumptions:
                    // Every first number is assumed to be the PkgType, and the next to be the object
                    if (type == sol::type::number) {
                        PkgType pkgType = arg.as<PkgType>();

                        // bounds check end of array for pairs
                        if (i + 1 < args.size()) {
                            auto &&obj = args[++i];

                            if (obj.get_type() != sol::type::number) {
                                sol::object o = tostring(obj);
                                auto result = o.as<std::string>();
                                snprintf(errBuf, sizeof(errBuf), "arg %d; expected number immediately after PkgType, got: %s", i, result.c_str());
                                return ptr->Throw(errBuf);
                            }

                            switch (pkgType) {
                                // these types do not require PkgType specifiers because they are lightuserdata
                                /*
                                case PkgType::BYTE_ARRAY:
                                    params.Write(obj.as<BYTES_t>());
                                    break;
                                case PkgType::PKG:
                                    params.Write(obj.as<NetPackage>());
                                    break;
                                case PkgType::STRING:
                                    params.Write(obj.as<std::string>());
                                    break;
                                case PkgType::NET_ID:
                                    params.Write(obj.as<NetID>());
                                    break;
                                case PkgType::VECTOR3:
                                    params.Write(obj.as<Vector3>());
                                    break;
                                case PkgType::VECTOR2i:
                                    params.Write(obj.as<Vector2i>());
                                    break;
                                case PkgType::QUATERNION:
                                    params.Write(obj.as<Quaternion>());
                                    break;
                                case PkgType::STRING_ARRAY:
                                    params.Write(obj.as<std::vector<std::string>>());
                                    break;
                                case PkgType::BOOL:
                                    params.Write(obj.as<bool>());
                                    break;
                                */
                                case PkgType::INT8:
                                    params.Write(obj.as<int8_t>());
                                    break;
                                case PkgType::UINT8:
                                    params.Write(obj.as<uint8_t>());
                                    break;
                                case PkgType::INT16:
                                    params.Write(obj.as<int16_t>());
                                    break;
                                case PkgType::UINT16:
                                    params.Write(obj.as<uint16_t>());
                                    break;
                                case PkgType::INT32:
                                    params.Write(obj.as<int32_t>());
                                    break;
                                case PkgType::UINT32:
                                    params.Write(obj.as<uint32_t>());
                                    break;
                                case PkgType::INT64:
                                    params.Write(obj.as<int64_t>());
                                    break;
                                case PkgType::UINT64:
                                    params.Write(obj.as<uint64_t>());
                                    break;
                                case PkgType::FLOAT:
                                    params.Write(obj.as<float>());
                                    break;
                                case PkgType::DOUBLE:
                                    params.Write(obj.as<double>());
                                    break;
                                default:
                                    snprintf(errBuf, sizeof(errBuf), "arg %d; invalid PkgType enum, got: %d", i, static_cast<std::underlying_type_t<decltype(pkgType)>>(pkgType));
                                    return ptr->Throw(errBuf);
                            }
                        } else {
                            snprintf(errBuf, sizeof(errBuf), "arg %d; unknown number type", i);
                            return ptr->Throw(errBuf);
                        }
                    } else {
                        if (type == sol::type::string) {
                            params.Write(arg.as<std::string>());
                        } else if (type == sol::type::boolean) {
                            params.Write(arg.as<bool>());
                        } else {
                            if (arg.is<BYTES_t>()) {
                                params.Write(arg.as<BYTES_t>());
                            } else if (arg.is<NetPackage>()) {
                                params.Write(arg.as<NetPackage>());
                            } else if (arg.is<NetID>()) {
                                params.Write(arg.as<NetID>());
                            } else if (arg.is<Vector3>()) {
                                params.Write(arg.as<Vector3>());
                            } else if (arg.is<Vector2i>()) {
                                params.Write(arg.as<Vector2i>());
                            } else if (arg.is<Vector2i>()) {
                                params.Write(arg.as<Vector2i>());
                            } else if (arg.is<std::vector<std::string>>()) {
                                params.Write(arg.as<std::vector<std::string>>());
                            } else {
                                sol::object o = tostring(arg);
                                auto result = o.as<std::string>();
                                snprintf(errBuf, sizeof(errBuf), "arg %d; expected serializable type, got: %s", i, result.c_str());
                                return ptr->Throw(errBuf);
                            }
                        }
                    }
                }
            }

            self.m_socket->Send(std::move(params));
        },

        "Register", [ptr](NetRpc& self, sol::variadic_args args) {
            HASH_t name = 0;
            std::vector<PkgType> types;

            for (int i=0; i < args.size(); i++) {
                auto &&arg = args[i];
                auto &&type = arg.get_type();

                if (i == 0) {
                    if (type == sol::type::string) {
                        name = VUtils::String::GetStableHashCode(arg.as<std::string>());
                    } else if (type == sol::type::number) {
                        name = arg.as<HASH_t>();
                    } else {
                        return ptr->Throw("first param must be a string or numeric hash");
                    }
                } else if (i + 1 < args.size()) {
                    // grab middle pkgtypes
                    if (type == sol::type::number) {
                        types.push_back(arg.as<PkgType>());
                    } else {
                        return ptr->Throw("middle params must be of PkgType (numeric enum)");
                    }
                } else {
                    if (type == sol::type::function) {
                        auto callback = arg.as<sol::function>();

                        self.Register(name,
                                      std::make_unique<MethodImpl<NetRpc*>>(callback, std::move(types)));
                    } else {
                        return ptr->Throw("fast param must be a function");
                    }
                }
            }
        },
        "socket", sol::property([](NetRpc& self) { return self.m_socket; })
    );

    // https://sol2.readthedocs.io/en/latest/api/usertype.html#inheritance-example
    state.new_usertype<ISocket>("ISocket",
        "Close", &ISocket::Close,
        "Connected", &ISocket::Connected,
        "GetAddress", &ISocket::GetAddress,
        "GetHostName", &ISocket::GetHostName,
        "GetSendQueueSize", &ISocket::GetSendQueueSize
    );



    auto apiTable = state["Valhalla"].get_or_create<sol::table>();

    apiTable["ServerVersion"] = SERVER_VERSION;
    apiTable["ValheimVersion"] = Version::GAME;
    apiTable["Delta"] = []() { return Valhalla()->Delta(); };
    apiTable["ID"] = []() { return Valhalla()->ID(); };
    apiTable["Nanos"] = []() { return Valhalla()->Nanos(); };
    apiTable["Ticks"] = []() { return Valhalla()->Ticks(); };
    apiTable["Time"] = []() { return Valhalla()->Time(); };

    //apiTable["RunTask"] = [](std::function<Task::F> f) { return Valhalla()->RunTask(f.); };

    // method overloading is easy
    // https://sol2.readthedocs.io/en/latest/api/overload.html

    apiTable["OnEvent"] = [ptr](sol::this_state thisState, sol::variadic_args args) {
        // match incrementally
        //std::string name;
        HASH_t name = 0;
        int priority = 0;
        for (int i=0; i < args.size(); i++) {
            auto&& arg = args[i];
            auto&& type = arg.get_type();

            if (i + 1 < args.size()) {
                if (type == sol::type::string) {
                    auto h = VUtils::String::GetStableHashCode(arg.as<std::string>());
                    if (name == 0) name = h;
                    else name ^= h;
                } else if (type == sol::type::number) {
                    auto h = arg.as<HASH_t>();
                    if (name == 0) name = h;
                    else name ^= h;
                } else {
                    return ptr->Throw("first few params must be string or numeric hash");
                }
            } else {
                if (type == sol::type::function) {
                    auto &&vec = m_callbacks[name];
                    vec.emplace_back(EventHandler{ptr, arg.as<sol::function>(), priority});
                    std::sort(vec.begin(), vec.end(), EventHandlerSort);
                } else {
                    return ptr->Throw("last param must be a function");
                }
            }
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
        thisEventTable["Cancel"] =          []() { m_eventStatus = EventStatus::CANCEL; };
        thisEventTable["SetCancelled"] =    [](bool c) { m_eventStatus = c ? EventStatus::CANCEL : EventStatus::PROCEED; };
        thisEventTable["cancelled"] =       []() { return m_eventStatus == EventStatus::CANCEL; };
    }

    return mod;
}



void ModManager::Init() {
    for (const auto& dir
        : fs::directory_iterator("mods")) {

        try {
            if (dir.exists() && dir.is_directory()) {
                auto&& dirname = dir.path().filename().string();

                if (dirname.starts_with("--"))
                    continue;

                std::string name, version, entry;
                int apiVersion;
                RunModInfoFrom(dirname, name, version, apiVersion, entry);
                auto mod = PrepareModEnvironment(name, version, apiVersion);
                RunStateFrom(mod->m_state, dirname + "/" + (entry + ".lua"));

                mods.insert({ name, std::move(mod) });

                LOG(INFO) << "Loaded mod '" << name << "'";
            }
        }
        catch (const std::exception& e) {
            LOG(ERROR) << "Failed to load mod: " << e.what() << " (" << dir.path().c_str() << ")";
        }
    }

    LOG(INFO) << "Loaded " << mods.size() << " mods";

    CallEvent("Enable");
}

void ModManager::UnInit() {
    CallEvent("Disable");
    m_callbacks.clear();
    mods.clear();
}
