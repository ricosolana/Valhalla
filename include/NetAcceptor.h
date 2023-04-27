#pragma once

#include <memory>
#include <thread>
#include <steam_gameserver.h>

#include "NetSocket.h"


class IAcceptor {
public:
    virtual ~IAcceptor() = default;

    // Init listening and queueing any accepted connections
    //  Should be non-blocking
    virtual void Listen() = 0;

    // Poll for a ready and newly accepted connection
    //  Should be non-blocking
    //  Nullable
    virtual ISocket::Ptr Accept() = 0;

    virtual void OnConfigLoad(bool reloading) {}

    // Do not use
    //virtual void Cleanup(ISocket* socket) = 0;
};



class AcceptorSteamDedicated : public IAcceptor {
private:
    const uint16_t m_port;
    HSteamListenSocket m_listenSocket;

    UNORDERED_MAP_t<HSteamNetConnection, std::shared_ptr<SteamSocket>> m_sockets;    // holds all sockets and manages lifetime
    UNORDERED_MAP_t<HSteamNetConnection, std::shared_ptr<SteamSocket>> m_connected;

public:
    AcceptorSteamDedicated();
    ~AcceptorSteamDedicated() override;

    void Listen() override;

    ISocket::Ptr Accept() override;

    //void Cleanup(ISocket* socket) override;

private:
    // https://partner.steamgames.com/doc/sdk/api#callbacks
    STEAM_GAMESERVER_CALLBACK(AcceptorSteamDedicated, OnSteamStatusChanged, SteamNetConnectionStatusChangedCallback_t);

    // status logs
    STEAM_GAMESERVER_CALLBACK(AcceptorSteamDedicated, OnSteamServersConnected, SteamServersConnected_t);
    STEAM_GAMESERVER_CALLBACK(AcceptorSteamDedicated, OnSteamServersDisconnected, SteamServersDisconnected_t);
    STEAM_GAMESERVER_CALLBACK(AcceptorSteamDedicated, OnSteamServerConnectFailure, SteamServerConnectFailure_t);
};

class AcceptorSteamP2P : public IAcceptor {
private:
    HSteamListenSocket m_listenSocket;

    UNORDERED_MAP_t<HSteamNetConnection, std::shared_ptr<SteamSocket>> m_sockets;    // holds all sockets and manages lifetime
    UNORDERED_MAP_t<HSteamNetConnection, std::shared_ptr<SteamSocket>> m_connected;

    CSteamID m_lobbyID;

    void OnLobbyCreated(LobbyCreated_t* pCallback, bool failure);
    CCallResult<AcceptorSteamP2P, LobbyCreated_t> m_lobbyCreatedCallResult;

public:
    AcceptorSteamP2P();
    ~AcceptorSteamP2P() override;

    void Listen() override;

    ISocket::Ptr Accept() override;

    void OnConfigLoad(bool reloading) override;

    //void Cleanup(ISocket* socket) override;

private:
    // https://partner.steamgames.com/doc/sdk/api#callbacks
    STEAM_CALLBACK(AcceptorSteamP2P, OnSteamStatusChanged, SteamNetConnectionStatusChangedCallback_t);

    // status logs
    STEAM_CALLBACK(AcceptorSteamP2P, OnSteamServersConnected, SteamServersConnected_t);
    STEAM_CALLBACK(AcceptorSteamP2P, OnSteamServersDisconnected, SteamServersDisconnected_t);
    STEAM_CALLBACK(AcceptorSteamP2P, OnSteamServerConnectFailure, SteamServerConnectFailure_t);
};
