#pragma once

#include <sol/sol.hpp>

namespace ScriptManager {
	void Init();
	void Uninit();
	lua_State* GetLuaState();

	namespace Event {
		/// Event calls
		void OnPreLogin();
		void OnUpdate(float delta);
	}
};
