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
//robin_
std::vector<Mod*> modsByPriority; // priority sorted

static bool prioritySort(Mod* a, Mod* b) {
	return a->m_priority < b->m_priority;
}

namespace ModManager {
	namespace Api {

		// can get the lua state by passing sol::this_state type at end
		// https://sol2.readthedocs.io/en/latest/tutorial/functions.html#any-return-to-and-from-lua

		// Load a Lua script starting in relative root 'data/mods/'
		sol::state LoadLuaFrom(const fs::path& luaPath) {
			std::string modCode;
			if (!ResourceManager::ReadFileBytes(fs::path("mods") / luaPath, modCode)) {
				throw std::runtime_error(std::string("Failed to open Lua file ") + luaPath.string());
			}

			auto state = sol::state();

			state.open_libraries();
			state.script(modCode);

			return state;
		}

		//sol::state LoadModFrom(const std::string& name) {
		//	//return LoadModFrom(name)
		//	std::string modCode;
		//	if (!ResourceManager::ReadFileBytes(fs::path("scripts") / fs::path(name) / "mod.lua", modCode)) {
		//		throw std::runtime_error(std::string("Failed to open mod file ") + name);
		//	}
		//
		//	auto state = (sol::state());
		//
		//	state.open_libraries();
		//	state.script(modCode);
		//
		//	return state;
		//}

		void LoadModInfoFrom(const fs::path& dirname, std::string& outName, std::string& outEntry, int &outPriority) {
			auto&& state = LoadLuaFrom(dirname / "modInfo.lua");

			auto modInfo = state["modInfo"];

			// required
			sol::optional<std::string> name = modInfo["name"]; // str
			sol::optional<int> api_version = modInfo["api_version"];
			sol::optional<std::string> entry = modInfo["entry"];
			sol::optional<int> priority = modInfo["priority"];
			sol::optional<std::string> version = modInfo["version"]; // str

			if (!(name && api_version && entry && priority && version)) {
				throw std::runtime_error("modInfo missing required specifiers");
			}

			if (mods.contains(name.value())) {
				throw std::runtime_error("tried registering mod with same name as an existing mod");
			}

			outName = name.value();
			outEntry = entry.value();
			outPriority = priority.value();

			//LOG(INFO) << "Loading mod '" << name.value() << "' " << version.value();
		}



		std::unique_ptr<Mod> LoadModFrom(const fs::path& dirname, const std::string& name, int priority) {//, robin_hood::unordered_set<std::string> circular) {
			
			auto mod = std::make_unique<Mod>(name, priority, LoadLuaFrom(dirname));
			auto ptr = mod.get();

			auto&& state = mod->m_state;
			
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
				"socket", &NetRpc::m_socket
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
				//"Start", &ISocket::Start
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

			//state.new_usertype<

			auto apiTable = state["Valhalla"].get_or_create<sol::table>();

			apiTable["Version"] = VALHEIM_VERSION;
			
			apiTable["RpcCallback"] = [ptr](std::string name, sol::function callback) {
				ptr->m_rpcCallbacks.insert({ Utils::GetStableHashCode(name), callback});
			};
			apiTable["RouteCallback"] = [ptr](std::string name, sol::function callback) {
				ptr->m_routeCallbacks.insert({ Utils::GetStableHashCode(name), callback });
			};
			apiTable["SyncCallback"] = [ptr](std::string name, sol::function callback) {
				ptr->m_syncCallbacks.insert({ Utils::GetStableHashCode(name), callback });
			};

			return mod;
		}
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
					int priority;
					Api::LoadModInfoFrom(dirname, name, entry, priority);

					auto &&mod = Api::LoadModFrom(dirname / (entry + ".lua"), name, priority);

					modsByPriority.push_back(mod.get());
					mods.insert({ name, std::move(mod) });

					LOG(INFO) << "Loaded mod '" << name << "'";
				}
			}
			catch (const std::exception& e) {
				LOG(ERROR) << "Failed to load mod: " << e.what();
			}
		}

		LOG(INFO) << "Loaded " << mods.size() << " mods";

		std::sort(modsByPriority.begin(), modsByPriority.end(), prioritySort);

		ModManager::Event::OnEnable();
	}

	void Uninit() {
		Event::OnDisable();
		modsByPriority.clear();
		mods.clear();
	}

	std::vector<Mod*>& GetMods() {
		return modsByPriority;
	}

	// instead get a particular mod
	//lua_State* GetLuaState() {
	//	return lua.lua_state();
	//}

	namespace Event {

#define EXECUTE(func, ...) \
for (auto&& mod : modsByPriority) { \
	if (!mod->func) \
		continue; \
	try { \
		mod->func(##__VA_ARGS__); \
	} \
	catch (sol::error& e) { \
		LOG(ERROR) << e.what(); \
	} \
}

#define EXECUTE_RES(func, ret, ...) \
for (auto&& mod : modsByPriority) { \
	if (!mod->func) \
		continue; \
	try { \
		ret = mod->func(##__VA_ARGS__); \
	} \
	catch (sol::error& e) { \
		LOG(ERROR) << e.what(); \
	} \
}

		void OnEnable() {
			OPTICK_EVENT();

			EXECUTE(m_onEnable);
		}

		void OnDisable() {
			OPTICK_EVENT();

			EXECUTE(m_onDisable);
		}

		void OnUpdate(float delta) {
			OPTICK_EVENT();

			EXECUTE(m_onUpdate, delta);
		}

		/// Event forward calls
		bool OnPeerInfo(NetRpc *rpc, UUID_t uuid, 
			const std::string& name, const std::string& version) {
			OPTICK_EVENT();

			bool allow = true;
			EXECUTE_RES(m_onPeerInfo, allow, rpc, uuid, name, version);

			return allow;
		}

		void OnChatMessage() {

		}




	}
}
