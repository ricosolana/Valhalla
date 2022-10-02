#include "ModManager.h"
#include <sol/sol.hpp>
#include "NetManager.h"
#include "ValhallaServer.h"
#include "ResourceManager.h"
#include <easylogging++.h>

class Mod {
public:
	// required attributes
	const std::string m_name;
	const int m_priority;

	// lua
	sol::state m_state;
	
	const std::function<void()> m_onEnable;
	const std::function<void()> m_onDisable;
	const std::function<void(float)> m_onUpdate;
	const std::function<bool(ISocket::Ptr, uuid_t, std::string, std::string)> m_onPeerInfo; // onHandshake; //onNewConnection;

	Mod(const std::string &name,
		int priority,
		sol::state state,
		const std::function<void()> &onEnable,
		const std::function<void()> &onDisable,
		const std::function<void(float)> &onUpdate,
		const std::function<bool(ISocket::Ptr, uuid_t, std::string, std::string)> &onPeerInfo)
		:	m_name(name), m_priority(priority), m_state(std::move(state)), 
			m_onEnable(onEnable), m_onDisable(onDisable), m_onUpdate(onUpdate), m_onPeerInfo(onPeerInfo) {}

};

robin_hood::unordered_map<std::string, std::unique_ptr<Mod>> mods;
std::vector<Mod*> modsByPriority; // priority sorted

static bool prioritySort(Mod* a, Mod* b) {
	return a->m_priority < b->m_priority;
}

namespace ModManager {
	namespace Api {

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
			auto&& state = LoadLuaFrom(dirname);

			auto onEnable = state["onEnable"].get_or(std::function<void()>());
			auto onDisable = state["onDisable"].get_or(std::function<void()>());
			auto onUpdate = state["onUpdate"].get_or(std::function<void(float)>());
			auto onPeerInfo = state["onPeerInfo"].get_or(std::function<bool(ISocket::Ptr, uuid_t, std::string, std::string)>());

			// https://stackoverflow.com/a/61071485
			// Override Lua print
			auto L(state.lua_state());
			lua_getglobal(L, "_G");
			lua_pushcclosure(L, [](lua_State* L) -> int
				{
					int n = lua_gettop(L);  /* number of arguments */
					int i;
					lua_getglobal(L, "tostring");
					//Rml::String output = "";
					std::string output = "";
					for (i = 1; i <= n; i++)
					{
						const char* s;
						lua_pushvalue(L, -1);  /* function to be called */
						lua_pushvalue(L, i);   /* value to print */
						lua_call(L, 1, 1);
						s = lua_tostring(L, -1);  /* get result */
						if (s == nullptr)
							return luaL_error(L, "'tostring' must return a string to 'print'");
						if (i > 1)
							output += "\t";
						output += (s);
						lua_pop(L, 1);  /* pop result */
					}

//					auto view = sol::state_view(L);
					
					//
					//auto find = mods.find(view["modInfo"]["name"]);
					//if (find == mods.end()) {
					//	LOG(ERROR) << "A loaded mod has deleted its modInfo";
					//	return 0;
					//}
					//
					//auto&& mod = find->second;
					//
					//LOG(INFO) << "[" << mod->m_name << "] " << output;
					LOG(INFO) << "[LUA] " << output;
					return 0;
				}, 0);
			lua_setfield(L, -2, "print");
			lua_pop(L, 1); //pop _G

			//state.new_usertype<Task>("Task",
			//	"at", &Task::at,
			//	"Cancel", &Task::Cancel,
			//	"period", &Task::period,
			//	"Repeats", &Task::Repeats,
			//	"function", &Task::function); // dummy
			//
			//state.new_usertype<ZNetPeer>("ZNetPeer",
			//	"Kick", &ZNetPeer::Kick,
			//	"characterId", &ZNetPeer::m_characterID,
			//	"playerName", &ZNetPeer::m_playerName,
			//	"publicRefPos", &ZNetPeer::m_publicRefPos,
			//	"refPos", &ZNetPeer::m_refPos,
			//	"rpc", &ZNetPeer::m_rpc,
			//	"uid", &ZNetPeer::m_uid);
			//
			//state.new_usertype<ZRpc>("ZRpc",
			//	"", ZRpc::Invoke,
			//	"", ZRpc::Register,
			//	"", ZRpc::m_socket)

			

			auto apiTable = state["Valhalla"].get_or_create<sol::table>();

			//apiTable["RegisterMod"] = Api::RegisterMod;
			apiTable["Version"] = ValhallaServer::VERSION;

			auto ptr(new Mod(
				// required
				name,
				priority,

				// lua
				std::move(state),

				// optional callbacks
				onEnable,
				onDisable,
				onUpdate,
				onPeerInfo
			));

			return std::unique_ptr<Mod>(ptr);
		}
	}



	void Init() {
		for (const auto& dir 
			: fs::directory_iterator(ResourceManager::GetPath("mods"))) {
			
			try {
				if (dir.exists() && dir.is_directory()) {
					auto&& dirname = dir.path().filename();

					std::string name, entry;
					int priority;
					Api::LoadModInfoFrom(dirname, name, entry, priority);

					auto &&mod = Api::LoadModFrom(dirname / (entry + ".lua"), name, priority);

					modsByPriority.push_back(mod.get());
					mods.insert({ name, std::move(mod) });

					LOG(INFO) << "Loaded mod '" << name << "'";
				}
			}
			catch (std::exception& e) {
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

	// instead get a particular mod
	//lua_State* GetLuaState() {
	//	return lua.lua_state();
	//}

	namespace Event {

		void OnEnable() {
			for (auto&& mod : modsByPriority) {
				if (mod->m_onEnable) mod->m_onEnable();
			}
		}

		void OnDisable() {
			for (auto&& mod : modsByPriority) {
				if (mod->m_onDisable) mod->m_onDisable();
			}
		}

		/// Event forward calls
		bool OnPeerInfo(ISocket::Ptr socket, uuid_t uuid, 
			const std::string& name, const std::string& version) {
			//bool allow = true;
			int allowWeight = 1;
			for (auto&& mod : mods) {
				if (mod.second->m_onPeerInfo) // check is mandatory to avoid std::bad_function_call
										// if function is empty
		
					mod.second->m_onPeerInfo(socket, uuid, name, version);
			}
			return allowWeight;
		}

		void OnNewConnection() {
			
		}

		void OnUpdate(float delta) {
			for (auto& mod : mods) {
				if (mod.second->m_onUpdate)
					mod.second->m_onUpdate(delta);
			}
		}
	}
}
