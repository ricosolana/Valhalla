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

ModCallbacks m_callbacks;

static bool ModCallbackSort(const ModCallback &a,
                            const ModCallback &b) {
    return a.second.second < b.second.second;
}

namespace ModManager {

    namespace Api {
        void Register(Mod* mod, std::vector<ModCallback> &callbacks, const sol::function& lua_callback, int priority) {
            callbacks.emplace_back(std::make_pair(mod, std::make_pair(lua_callback, priority)));
            std::sort(callbacks.begin(), callbacks.end(), ModCallbackSort);
        }

        void RegisterOnEnable(Mod* mod, const sol::function& lua_callback, int priority) {
            Register(mod, m_callbacks.m_onEnable, lua_callback, priority);
        }

        void RegisterOnDisable(Mod* mod, const sol::function& lua_callback, int priority) {
            Register(mod, m_callbacks.m_onDisable, lua_callback, priority);
        }

        void RegisterOnUpdate(Mod* mod, const sol::function& lua_callback, int priority) {
            Register(mod, m_callbacks.m_onUpdate, lua_callback, priority);
        }

        void RegisterOnPeerInfo(Mod* mod, const sol::function& lua_callback, int priority) {
            Register(mod, m_callbacks.m_onPeerInfo, lua_callback, priority);
        }

        void RegisterOnRpc(Mod* mod, const std::string& name, const sol::function& lua_callback, int priority) {
            Register(mod, m_callbacks.m_onRpc[Utils::GetStableHashCode(name)], lua_callback, priority);
        }

        void RegisterOnRoute(Mod* mod, const std::string& name, const sol::function& lua_callback, int priority) {
            Register(mod, m_callbacks.m_onRoute[Utils::GetStableHashCode(name)], lua_callback, priority);
        }

        void RegisterOnSync(Mod* mod, const std::string& name, const sol::function& lua_callback, int priority) {
            Register(mod, m_callbacks.m_onSync[Utils::GetStableHashCode(name)], lua_callback, priority);
        }
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

    void RunModInfoFrom(const fs::path& dirname, std::string& outName, std::string& outEntry) {
        auto&& state = RunLuaFrom(dirname / "modInfo.lua");

        auto modInfo = state["modInfo"];

        // required
        sol::optional<std::string> name = modInfo["name"]; // str
        sol::optional<int> api_version = modInfo["api_version"];
        sol::optional<std::string> entry = modInfo["entry"];
        sol::optional<std::string> version = modInfo["version"]; // str

        if (!(name && api_version && entry && version)) {
            throw std::runtime_error("modInfo missing required specifiers");
        }

        if (mods.contains(name.value())) {
            throw std::runtime_error("tried registering mod with same name as an existing mod");
        }

        outName = name.value();
        outEntry = entry.value();

        //LOG(INFO) << "Loading mod '" << name.value() << "' " << version.value();
    }



    std::unique_ptr<Mod> PrepareModEnvironment(const std::string& name) {
        auto mod = std::make_unique<Mod>(name);
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

        state.new_usertype<Mod>("Mod",
                                "name", &Mod::m_name
        );

        //state.new_usertype<

        auto apiTable = state["Valhalla"].get_or_create<sol::table>();

        apiTable["Version"] = VALHEIM_VERSION;

        // method overloading is easy
        // https://sol2.readthedocs.io/en/latest/api/overload.html

        apiTable["OnEnable"] = sol::overload([ptr](const sol::function& lua_callback) { Api::RegisterOnEnable(ptr, lua_callback, 0); },
                                             [ptr](const sol::function& lua_callback, int priority) { Api::RegisterOnEnable(ptr, lua_callback, priority); });

        apiTable["OnDisable"] = sol::overload([ptr](const sol::function& lua_callback) { Api::RegisterOnDisable(ptr, lua_callback, 0); },
                                              [ptr](const sol::function& lua_callback, int priority) { Api::RegisterOnDisable(ptr, lua_callback, priority); });

        apiTable["OnUpdate"] = sol::overload([ptr](const sol::function& lua_callback) { Api::RegisterOnUpdate(ptr, lua_callback, 0); },
                                             [ptr](const sol::function& lua_callback, int priority) { Api::RegisterOnUpdate(ptr, lua_callback, priority); });

        apiTable["OnPeerInfo"] = sol::overload([ptr](const sol::function& lua_callback) { Api::RegisterOnPeerInfo(ptr, lua_callback, 0); },
                                               [ptr](const sol::function& lua_callback, int priority) { Api::RegisterOnPeerInfo(ptr, lua_callback, priority); });

        apiTable["OnRpc"] = sol::overload([ptr](const std::string& name, const sol::function& lua_callback) { Api::RegisterOnRpc(ptr, name, lua_callback, 0); },
                                          [ptr](const std::string& name, const sol::function& lua_callback, int priority) { Api::RegisterOnRpc(ptr, name, lua_callback, priority); });

        apiTable["OnRoute"] = sol::overload([ptr](const std::string& name, const sol::function& lua_callback) { Api::RegisterOnRoute(ptr, name, lua_callback, 0); },
                                            [ptr](const std::string& name, const sol::function& lua_callback, int priority) { Api::RegisterOnRoute(ptr, name, lua_callback, priority); });

        apiTable["OnSync"] = sol::overload([ptr](const std::string& name, const sol::function& lua_callback) { Api::RegisterOnSync(ptr, name, lua_callback, 0); },
                                           [ptr](const std::string& name, const sol::function& lua_callback, int priority) { Api::RegisterOnSync(ptr, name, lua_callback, priority); });

        // todo try this
        //state["modThis"] = sol::property([ptr]() { return ptr; });

        //apiTable["OnRouteWatch"] = [ptr](const std::string& name, const sol::function& callback, int priority) {
        //    m_onRouteWatch[Utils::GetStableHashCode(name)].emplace_back(callback, priority);
        //};
        //apiTable["OnSyncWatch"] = [ptr](const std::string& name, const sol::function& callback, int priority) {
        //    m_onSyncWatch[Utils::GetStableHashCode(name)].emplace_back(callback, priority);
        //};

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

					std::string name, entry;
                    RunModInfoFrom(dirname, name, entry);
                    auto mod = PrepareModEnvironment(name);
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

		ModManager::Event::OnEnable();
	}

	void UnInit() {
		Event::OnDisable();
		mods.clear();
	}

	//std::vector<Mod*>& GetMods() {
	//	return modsByPriority;
	//}

	// instead get a particular mod
	//lua_State* GetLuaState() {
	//	return lua.lua_state();
	//}

    ModCallbacks& getCallbacks() {
        return m_callbacks;
    }

	namespace Event {

#define EXECUTE(func, ...) \
for (auto&& ppair : func) { \
    auto&& pair = ppair.second; \
	if (!pair.first) \
		continue; \
	try { \
		pair.first(##__VA_ARGS__); \
	} \
	catch (sol::error& e) { \
		LOG(ERROR) << e.what(); \
	} \
}

#define EXECUTE_RES(func, res, ...) \
for (auto&& ppair : func) { \
    auto&& pair = ppair.second; \
	if (!pair.first) \
		continue; \
	try { \
		res = pair.first(##__VA_ARGS__); \
	} \
	catch (sol::error& e) { \
		LOG(ERROR) << e.what(); \
	} \
}

        void OnEnable() {
			OPTICK_EVENT();

			EXECUTE(m_callbacks.m_onEnable);
		}

		void OnDisable() {
			OPTICK_EVENT();

			EXECUTE(m_callbacks.m_onDisable);
		}

		void OnUpdate(float delta) {
			OPTICK_EVENT();

			EXECUTE(m_callbacks.m_onUpdate, delta);
		}

		/// Event forward calls
		bool OnPeerInfo(NetRpc *rpc, OWNER_t uuid,
                        const std::string& name, const std::string& version) {
			OPTICK_EVENT();

			bool allow = true;
            EXECUTE_RES(m_callbacks.m_onPeerInfo, allow, rpc, uuid, name, version);

            //for (auto &&ppair: m_onPeerInfo) {
            //    auto &&pair = ppair.second;
            //    if (!pair.first)continue;
            //    try { allow = pair.first(rpc, uuid, name, version); } catch (sol::error &e) {
            //        LOG(ERROR) << e.what();
            //    }
            //};

			return allow;
		}

		void OnChatMessage() {

		}




	}
}
