#include "ScriptManager.hpp"
#include <RmlUi/Core.h>
#include <RmlUi/Lua.h>
#include <sol/sol.hpp>
#include <filesystem>
#include "Client.hpp"

struct Script {
	const std::function<void()> onEnable;
	const std::function<void()> onDisable;
	const std::function<void(float)> onUpdate;
	const std::function<void()> onLogin;
};

std::vector<Script> scripts;
sol::state lua;

namespace Alchyme {
	namespace ScriptManager {
		namespace Api {
			void RegisterScript(sol::table scriptTable) {
				Script script = {
					scriptTable["onEnable"].get_or(std::function<void()>()),
					scriptTable["onDisable"].get_or(std::function<void()>()),
					scriptTable["onUpdate"].get_or(std::function<void(float)>()),
					scriptTable["onLogin"].get_or(std::function<void()>()),
				};
				scripts.push_back(script);
				script.onEnable();
			}

			void ConnectToServer(std::string address) {
				//LOG(INFO) << "Attempt to connect to server: " << address << "\n";

				auto at = address.find(':');
				std::string port = "8001";
				if (at != std::string::npos) {
					port = address.substr(at + 1);
					address = address.substr(0, at);
				}

				Client::Get()->Connect(address, port);
			}

			void ServerStatus(std::string address) {
				Client::Get()->m_mode = ConnectMode::STATUS;
				ConnectToServer(address);
			}

			void ServerJoin(std::string address) {
				Client::Get()->m_mode = ConnectMode::LOGIN;
				ConnectToServer(address);
			}

			void DisconnectFromServer() {
				LOG(INFO) << "Lua disconnect invoked";
				Client::Get()->Disconnect();
			}

			void SendLogin(std::string username, std::string password) {
				Client::Get()->SendLogin(username, password);
			}

		}

		int OverrideLuaPrint(lua_State* L)
		{
			int n = lua_gettop(L);  /* number of arguments */
			int i;
			lua_getglobal(L, "tostring");
			Rml::String output = "";
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
				output += Rml::String(s);
				lua_pop(L, 1);  /* pop result */
			}
			output += "\n";
			Rml::Log::Message(Rml::Log::LT_INFO, "[LUA] %s", output.c_str());
			return 0;
		}

		void Init() {
			std::string scriptCode;
			if (Rml::GetFileInterface()->LoadFile("scripts/entry.lua", scriptCode)) {

				// State
				lua = sol::state();
				lua.open_libraries();

				auto L(lua.lua_state());

				Rml::Lua::Initialise(L);

				auto apiTable = lua["Alchyme"].get_or_create<sol::table>();

				apiTable["RegisterScript"] = Api::RegisterScript;
				//apiTable["ConnectToServer"] = ScriptManager::Api::ConnectToServer;
				apiTable["ServerJoin"] = Api::ServerJoin;
				apiTable["ServerStatus"] = Api::ServerStatus;
				apiTable["DisconnectFromServer"] = Api::DisconnectFromServer;
				apiTable["SendLogin"] = Api::SendLogin;



				// Basically RML print, but add [LUA] to LOG differenciate better
				lua_getglobal(L, "_G");

				lua_pushcfunction(L, OverrideLuaPrint);
				lua_setfield(L, -2, "print");

				lua_pop(L, 1); //pop _G



				// Test global user states to load stuff from script
				// ...

				lua.safe_script(scriptCode);
			}
		}

		lua_State* GetLuaState() {
			return lua.lua_state();
		}

		namespace Event {
			/// Event forward calls
			void OnLogin() {
				for (auto& script : scripts) {
					if (script.onLogin) // check is mandatory to avoid std::bad_function_call
											// if function is empty

						script.onLogin();
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
}
