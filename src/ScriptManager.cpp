#include "ScriptManager.h"
#include <sol/sol.hpp>
#include <filesystem>
#include "ZNet.h"
#include "ValhallaServer.h"
#include "ResourceManager.h"
#include <easylogging++.h>

struct Script {
	const std::function<void()> onEnable;
	const std::function<void()> onDisable;
	const std::function<void(float)> onUpdate;
	const std::function<void()> onPreLogin;
};

std::vector<Script> scripts;
sol::state lua;

namespace ScriptManager {
	namespace Api {
		void RegisterScript(sol::table scriptTable) {
			Script script = {
				scriptTable["onEnable"].get_or(std::function<void()>()),
				scriptTable["onDisable"].get_or(std::function<void()>()),
				scriptTable["onUpdate"].get_or(std::function<void(float)>()),
				scriptTable["onPreLogin"].get_or(std::function<void()>()),
			};
			scripts.push_back(script);
			script.onEnable();
		}

		void Disconnect() {
			LOG(INFO) << "Lua disconnect invoked";
			//Valhalla()->m_znet->Disconnect();
		}

		void SendPeerInfo(std::string password) {
			LOG(INFO) << "Lua peer info invoked";
			//Valhalla()->m_znet->SendPeerInfo(password);
		}
	}

	int OverrideLuaPrint(lua_State* L)
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
		LOG(INFO) << "[LUA] " << output;
		return 0;
	}

	// how should scripting be best handled?
	// scripts should be added like mods
	// multiple entry points, where each primary entry script is loaded
	void Init() {
		std::string scriptCode;
		if (ResourceManager::ReadFileBytes("scripts/entry.lua", scriptCode)) {

			// State
			lua = sol::state();
			lua.open_libraries();

			auto L(lua.lua_state());

			//Rml::Lua::Initialise(L);

			auto apiTable = lua["Valhalla"].get_or_create<sol::table>();

			apiTable["RegisterScript"] = Api::RegisterScript;

			// most of these are useless when on a server
			// however catching player login events is desired, anddisconnecting a player, ...
			//apiTable["Connect"] = Api::Connect;
			apiTable["Disconnect"] = Api::Disconnect;
			apiTable["SendPeerInfo"] = Api::SendPeerInfo;

			//sol::set_default_exception_handler(L, )
			lua.set_exception_handler([](lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
				// L is the lua state, which you can wrap in a state_view if necessary
				// maybe_exception will contain exception, if it exists
				// description will either be the what() of the exception or a description saying that we hit the general-case catch(...)
				if (maybe_exception) {
					const std::exception& ex = *maybe_exception;
					LOG(ERROR) << "[LUA exception] " << ex.what();
				}
				else {
					LOG(ERROR) << "[LUA description] " << description;
				}

				// you must push 1 element onto the stack to be
				// transported through as the error object in Lua
				// note that Lua -- and 99.5% of all Lua users and libraries -- expects a string
				// so we push a single string (in our case, the description of the error)
				return sol::stack::push(L, description);
			});

			// Override Lua print
			lua_getglobal(L, "_G");

			lua_pushcfunction(L, OverrideLuaPrint);
			lua_setfield(L, -2, "print");

			lua_pop(L, 1); //pop _G



			// Test global user states to load stuff from script
			// ...

			lua.safe_script(scriptCode);
		}
	}

	void Uninit() {
		for (auto& script : scripts) {
			if (script.onDisable) // check is mandatory to avoid std::bad_function_call
									// if function is empty

				script.onDisable();
		}
		scripts.clear();
	}

	lua_State* GetLuaState() {
		return lua.lua_state();
	}

	namespace Event {
		/// Event forward calls
		void OnPreLogin() {
			for (auto& script : scripts) {
				if (script.onPreLogin) // check is mandatory to avoid std::bad_function_call
										// if function is empty

					script.onPreLogin();
			}
		}

		void OnUpdate(float delta) {
			for (auto& script : scripts) {
				if (script.onUpdate)
					script.onUpdate(delta);
			}
		}
	}
}
