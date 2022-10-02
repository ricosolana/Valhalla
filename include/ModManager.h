#pragma once

#include <sol/sol.hpp>
#include "NetSocket.h"

namespace ModManager {
	void Init();
	void Uninit();

	namespace Event {
		// Do not call externally
		void OnEnable();
		// Do not call externally
		void OnDisable();

		/// Event calls
		bool OnPeerInfo(ISocket::Ptr socket, 
			uuid_t uuid, 
			const std::string& name, 
			const std::string& version);
		void OnUpdate(float delta);
	}
};
