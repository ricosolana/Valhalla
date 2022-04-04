#pragma once

#include <sol/sol.hpp>

namespace Alchyme {
	namespace ScriptManager {
		void Init();
		lua_State* GetLuaState();

		namespace Event {
			/// Event calls
			void OnLogin();
			void OnUpdate(float delta);
		}
	};
}
