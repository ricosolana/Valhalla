#include "ModManager.h"
#include <sol/sol.hpp>
#include "ZNet.h"
#include "ValhallaServer.h"
#include "ResourceManager.h"
#include <easylogging++.h>

class Mod {
public:
	// required attributes
	const std::string m_name;
	const std::string m_version; // std::array<int, 3> version;
	const int m_apiVersion;
	const std::string m_description;

	// optional attributes
	const std::vector<std::string> m_authors;
	const std::string m_website;

	// lua
	sol::state m_state;
	//std::unique_ptr<
	//sol::state_view view;
	
	const std::function<void()> m_onEnable;
	const std::function<void()> m_onDisable;
	const std::function<void(float)> m_onUpdate;
	const std::function<void(ISocket::Ptr, uuid_t, std::string, std::string)> m_onPeerInfo; // onHandshake; //onNewConnection;
	//const std::function<void()>

	Mod(const std::string &name,
		const std::string &version,
		const int api_version,
		const std::string &description,
		const std::vector<std::string> &authors,
		const std::string &website,
		sol::state state,
		const std::function<void()> &onEnable,
		const std::function<void()> &onDisable,
		const std::function<void(float)> &onUpdate,
		const std::function<void(ISocket::Ptr, uuid_t, std::string, std::string)> &onPeerInfo)
		:	m_name(name), m_version(version), m_apiVersion(api_version), m_description(description),
			m_authors(m_authors), m_state(std::move(state)), 
			m_onEnable(onEnable), m_onDisable(onDisable), m_onUpdate(onUpdate), m_onPeerInfo(onPeerInfo) {}

};

//std::vector<Script> scripts;
//sol::state lua;
//std::vector<Mod> mods;
robin_hood::unordered_map<std::string, std::unique_ptr<Mod>> mods;
//robin_hood::unordered_map<lua_State*, Mod*> modsByState; // 

namespace ModManager {
	namespace Api {
		sol::state LoadModFrom(const std::string& name) {
			//return LoadModFrom(name)
			std::string modCode;
			if (!ResourceManager::ReadFileBytes(fs::path("scripts") / fs::path(name) / "mod.lua", modCode)) {
				throw std::runtime_error(std::string("Failed to open mod file ") + name);
			}

			auto state = (sol::state());

			state.open_libraries();
			state.script(modCode);

			return state;
		}

		void RegisterMod(sol::state state) {//, robin_hood::unordered_set<std::string> circular) {
			auto modInfo = state["modInfo"];

			// required attributes
			sol::optional<std::string> name = modInfo["name"]; // str
			sol::optional<std::string> version = modInfo["version"]; // str
			sol::optional<int> api_version = modInfo["api_version"];
			sol::optional<std::string> description = modInfo["description"]; // str

			// optional attributes
			auto authors = modInfo["authors"].get_or(std::vector<std::string>());
			auto website = modInfo["website"].get_or(std::string());

			auto onEnable = modInfo["onEnable"].get_or(std::function<void()>());
			auto onDisable = modInfo["onDisable"].get_or(std::function<void()>());
			auto onUpdate = modInfo["onUpdate"].get_or(std::function<void(float)>());
			auto onPeerInfo = modInfo["onPeerInfo"].get_or(std::function<void(ISocket::Ptr, uuid_t, std::string, std::string)>());

			if (!(name && version && api_version && description)) {
				throw std::runtime_error("modInfo missing required specifiers");
			}

			if (mods.contains(name.value())) {
				throw std::runtime_error("tried registering mod with same name as an existing mod");
			}



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

					auto view = sol::state_view(L);

					auto find = mods.find(view["modInfo"]["name"]);
					if (find == mods.end()) {
						LOG(ERROR) << "A loaded mod has deleted its modInfo";
						return 0;
					}

					auto&& mod = find->second;

					LOG(INFO) << "[" << mod->m_name << "] " << output;
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
			
			//Valhalla()->RunTask([](Task* self) {});

			//apiTable["RunTask"] = [](sol::function f) {Valhalla()->RunTask([&f](Task* self) {f(self);});};



			//apiTable["Disconnect"] = Api::Disconnect;
			//apiTable["SendPeerInfo"] = Api::SendPeerInfo;

			// dependency management
			/*
			sol::optional<std::vector<std::string>> depend = modInfo["depend"];

			if (depend) {
				for (auto&& d : depend.value()) {
					if (!mods.contains(d)) {
						if (!circular.contains(d)) {
							// circular dependencies i will not care about
							// circular can be determined by going around the daisy chain while holding a certain
							// reference, and if that same one is found, then thats circular dependency, then throw or 
							// something, but this is lua so it doesnt matter too much
							try {
								RegisterMod(LoadModFrom(d), circular);
								//LOG(INFO) << "Loaded " << d << " as a dependency of " << name.value();
							}
							catch (std::exception& e) {
								LOG(ERROR) << "failed to load dependency: " << e.what();
								//mods.erase(name.value());
								throw std::runtime_error(e.what());
							}
						}
						else {
							throw std::runtime_error("A mod references itself through")
						}
					}
				}
			}*/


			auto ptr(new Mod(
				// required
				name.value(),
				version.value(),
				api_version.value(),
				description.value(),

				// optional
				authors,
				website,

				// lua
				std::move(state),

				// optional callbacks
				onEnable,
				onDisable,
				onUpdate,
				onPeerInfo
			));

			mods.insert({ name.value(), std::unique_ptr<Mod>(ptr) });
			//modsByState.insert({ name.value(), ptr});

			if (onEnable)
				onEnable();

			LOG(INFO) << "Loaded mod '" << name.value() << "' " << version.value();
		}

		//void Disconnect() {
		//	LOG(INFO) << "Lua disconnect invoked";
		//	//Valhalla()->m_znet->Disconnect();
		//}

	}

	void Init() {
		for (const auto& dir 
			: fs::directory_iterator(ResourceManager::GetPath("scripts"))) {
			
			try {
				if (dir.exists() && dir.is_directory()) {
					Api::RegisterMod(
						Api::LoadModFrom(dir.path().filename().string()));
				}
			}
			catch (std::exception& e) {
				LOG(ERROR) << "Failed to load mod: " << e.what();
			}
		}

		LOG(INFO) << "Loaded " << mods.size() << " mods";
	}

	void Uninit() {
		for (auto&& mod : mods) {
			LOG(INFO) << "Disabling " << mod.first;
			if (mod.second->m_onDisable)
				mod.second->m_onDisable();
		}
		//modsByState.clear();
		mods.clear();
	}

	// instead get a particular mod
	//lua_State* GetLuaState() {
	//	return lua.lua_state();
	//}

	namespace Event {
		/// Event forward calls
		void OnPeerInfo(ISocket::Ptr socket, uuid_t uuid, const std::string& name, const std::string& version) {
			for (auto&& mod : mods) {
				if (mod.second->m_onPeerInfo) // check is mandatory to avoid std::bad_function_call
										// if function is empty
		
					mod.second->m_onPeerInfo(socket, uuid, name, version);
			}
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
