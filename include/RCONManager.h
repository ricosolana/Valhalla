#pragma once

#include "NetSocket.h"
#include "NetAcceptor.h"

class RCONManager {
	friend class IValhalla;

private:
	std::unique_ptr<RCONAcceptor> m_rcon;
	std::list<std::shared_ptr<RCONSocket>> m_rconSockets;

	std::string m_password;

private:
	void Init(std::string password, uint16_t port);
	void UnInit();
	void Update();


};
