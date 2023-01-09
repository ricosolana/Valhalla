#pragma once

#include <memory>
#include <thread>
#include <steam_gameserver.h>

#include "NetSocket.h"

class IAcceptor {
public:
    virtual ~IAcceptor() = default;

    // Init listening and queueing any accepted connections
    virtual void Listen() = 0;

    // Poll for a ready and newly accepted connection
    virtual std::optional<ISocket::Ptr> Accept() = 0;

    // Do not use
    //virtual void Cleanup(ISocket* socket) = 0;
};

class AcceptorSteam : public IAcceptor {
private:
    const uint16_t m_port;
    HSteamListenSocket m_listenSocket;

    robin_hood::unordered_map<HSteamNetConnection, std::shared_ptr<SteamSocket>> m_sockets;    // holds all sockets and manages lifetime
    robin_hood::unordered_map<HSteamNetConnection, std::shared_ptr<SteamSocket>> m_connected;

public:
    AcceptorSteam();
    ~AcceptorSteam() override;

    void Listen() override;

    std::optional<ISocket::Ptr> Accept() override;

    //void Cleanup(ISocket* socket) override;

private:
    // https://partner.steamgames.com/doc/sdk/api#callbacks
    STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamStatusChanged, SteamNetConnectionStatusChangedCallback_t);

    // status logs
    STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServersConnected, SteamServersConnected_t);
    STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServersDisconnected, SteamServersDisconnected_t);
    STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServerConnectFailure, SteamServerConnectFailure_t);
};
