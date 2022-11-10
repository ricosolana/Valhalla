#include <sol/sol.hpp>
#include <easylogging++.h>
#include <optick.h>

#include "ModManager.h"
#include "NetManager.h"
#include "ValhallaServer.h"
#include "ResourceManager.h"
#include "NetRpc.h"
#include "NetHashes.h"



robin_hood::unordered_map<std::string, std::unique_ptr<Mod>> mods;

// pre
// transpile
// post
//std::array<robin_hood::unordered_map<HASH_t, std::vector<EventHandler>>, static_cast<int>(ModEventMode::_MAX)> m_callbacks;
robin_hood::unordered_map<HASH_t, EventHandlerStream> m_callbacks;
std::optional<EventHandlerStream> currentEventStream;

static bool EventHandlerSort(const EventHandler &a,
                             const EventHandler &b) {
    return a.m_priority < b.m_priority;
}

/*
HASH_t EventConcat(const std::initializer_list<std::string>& strings) {
    HASH_t hash = 0;
    for (auto&& str : strings) {
        auto h = Utils::GetStableHashCode(str);
        if (hash == 0)
            hash = h;
        else
            hash ^= h;
    }
    return hash;

    //std::string name;
    //for (auto&& str : strings)
    //    name += std::to_string(Utils::GetStableHashCode(str));
    //return Utils::GetStableHashCode(name);
}*/

namespace ModManager {
    namespace Api {
        //void Register(Mod* mod, HASH_t name, const sol::function& lua_callback, int priority) {
        //    auto&& vec = m_callbacks[name];
        //    vec.emplace_back(EventHandler{mod, lua_callback, priority});
        //    std::sort(vec.begin(), vec.end(), EventHandlerSort);
        //}
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

    void RunModFrom(Mod* mod, const fs::path& luaPath) {
        std::string modCode;
        if (!ResourceManager::ReadFileBytes(fs::path("mods") / luaPath, modCode)) {
            throw std::runtime_error(std::string("Failed to open Lua file ") + luaPath.string());
        }

        auto &&state = mod->m_state;

        state.script(modCode);
    }

    void RunModInfoFrom(const fs::path& dirname,
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



    std::unique_ptr<Mod> PrepareModEnvironment(const std::string& name,
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

        // To register an RPC, must pass a lua stack handler
        // basically, get the lua state to get args passed to RPC invoke
        // https://github.com/ThePhD/sol2/issues/471#issuecomment-320259767
        state.new_usertype<NetRpc>("NetRpc",
            //"Invoke", &NetRpc::Invoke,
            "Register", //[](sol::this_state L, sol::variadic_args args) {
                [](NetRpc& self, sol::variadic_args args) {
                //sol::state_view view(L);
                LOG(INFO) << "Lua Register call args:";
                for (auto&& arg : args) {
                    LOG(INFO) << arg.as<std::string>();
                    //LOG(INFO) << std::string(arg);
                }
            },
            "socket", sol::property([](NetRpc& self) { return self.m_socket; })
        );

        // https://sol2.readthedocs.io/en/latest/api/usertype.html#inheritance-example
        state.new_usertype<ISocket>("ISocket",
            "Close", &ISocket::Close,
            "Connected", &ISocket::Connected,
            "GetHostName", &ISocket::GetHostName,
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

        apiTable["OnEvent"] = [ptr](sol::this_state thisState, sol::variadic_args args) {
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
                auto &&vec = m_callbacks[name].m_callbacks;
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

            /*
            for (auto&& arg : args) {
                auto&& type = arg.get_type();
                if (type == sol::type::string) {
                    //name += std::to_string(Utils::GetStableHashCode(arg.as<std::string>()));
                    //name ^= Utils::
                    auto h = Utils::GetStableHashCode(arg.as<std::string>());
                    if (name == 0) name = h;
                    else name ^= h;
                } else if (type == sol::type::number) {
                    priority = arg.as<int>();
                } else if (type == sol::type::function) {
                    callback = arg.as<sol::function>();
                }
            }
            //Api::Register(ptr, Utils::GetStableHashCode(name), callback, 0);

            auto&& vec = m_callbacks[name];
            vec.emplace_back(EventHandler{ptr, callback, priority});
            std::sort(vec.begin(), vec.end(), EventHandlerSort);*/
        };

        auto thisTable = state["This"].get_or_create<sol::table>();

        //state.new_enum("Mod",
        //               "name", &Mod::m_name,
        //               "version", &Mod::m_version,
        //               "apiVersion", &Mod::m_apiVersion);

        // todo try this
        //thisTable["mod"] = sol::property([ptr]() { return ptr; });

        {
            auto thisModTable = thisTable["mod"].get_or_create<sol::table>();
            thisModTable["name"] = ptr->m_name;
            thisModTable["version"] = ptr->m_version;
            thisModTable["apiVersion"] = ptr->m_apiVersion;
        }

        // get informaiton about the current event if any
        {
            auto thisEventTable = thisTable["event"].get_or_create<sol::table>();

            // cancel might be better being a var
            //thisEventTable["cancel"] = false;

            // so lua usage:

            thisEventTable["Cancel"] = [ptr]() {
                // Intended to be called from within an OnEvent
                // The intended delegate will be cancelled or ignored
                // so somehow grab the current-temp event
                // and flag for it to cancel the delegate in question

            };
            thisEventTable["cancelled"] =
        }

        return mod;
    }



	void Init() {
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
                    RunModFrom(mod.get(), dirname / (entry + ".lua"));

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

	void UnInit() {
        CallEvent("Disable");
        m_callbacks.clear();
		mods.clear();
	}

    std::vector<EventHandler>& GetCallbacks(HASH_t hash) {
        return m_callbacks[hash];
    }

    std::vector<EventHandler>& GetCallbacks(const char* name) {
        return GetCallbacks(Utils::GetStableHashCode(name));
    }

    std::optional<EventHandlerStream> &CurrentEventStream() {
        return currentEventStream;
    }
}
