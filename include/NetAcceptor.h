#pragma once

#include <memory>
#include <thread>

#include "NetSocket.h"

class IAcceptor {
public:
	virtual ~IAcceptor() = default;

	// Start listening and queueing any accepted connections
	virtual void Listen() = 0;

	// Poll for a ready and newly accepted connection
	virtual ISocket::Ptr Accept() = 0;
};

class AcceptorSteam : public IAcceptor {
private:
	uint16_t m_port;
	HSteamListenSocket m_listenSocket;

	robin_hood::unordered_map<HSteamNetConnection, std::shared_ptr<SteamSocket>> m_sockets;			// holds all sockets
	robin_hood::unordered_map<HSteamNetConnection, std::shared_ptr<SteamSocket>> m_pendingConnect;	// holds all newly connecting sockets
	robin_hood::unordered_map<HSteamNetConnection, std::shared_ptr<SteamSocket>> m_readyAccepted;	// holds sockets ready for Accept() retrieval

	//std::deque<std::shared_ptr<SteamSocket>> m_readyAccepted;

public:
	AcceptorSteam(const std::string& name, 
		bool hasPassword, uint16_t port, bool isPublic, float timeout);
	~AcceptorSteam();

	void Listen() override;

	ISocket::Ptr Accept() override;

private:
	// https://partner.steamgames.com/doc/sdk/api#callbacks
	STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamStatusChanged, SteamNetConnectionStatusChangedCallback_t);

	// dummy prints
	STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServersConnected, SteamServersConnected_t);
	STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServersDisconnected, SteamServersDisconnected_t);
	STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServerConnectFailure, SteamServerConnectFailure_t);
};
