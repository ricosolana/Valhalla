#pragma once

#include <sol/sol.hpp>

namespace ScriptManager {
	void Init();
	void Uninit();
	lua_State* GetLuaState();

	namespace Event {
		/// Event calls
		void OnLogin();
		void OnUpdate(float delta);
	}
};
