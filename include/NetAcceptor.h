#pragma once

#include <memory>
#include <thread>
#include <steam_gameserver.h>

#include "NetSocket.h"
#include "ValhallaServer.h"

class IAcceptor {
public:
    virtual ~IAcceptor() = default;

    // Init listening and queueing any accepted connections
    //  Should be non-blocking
    virtual void Listen() = 0;

    virtual void Close() = 0;

    // Poll for a ready and newly accepted connection
    //  Should be non-blocking
    //  Nullable
    virtual ISocket::Ptr Accept() = 0;

    virtual void OnConfigLoad(bool reloading) {}

    // Do not use
    //virtual void Cleanup(ISocket* socket) = 0;
};



class AcceptorSteam : public IAcceptor {
private:
    //const uint16_t m_port;
    HSteamListenSocket m_listenSocket;

    UNORDERED_MAP_t<HSteamNetConnection, std::shared_ptr<SteamSocket>> m_sockets;    // holds all sockets and manages lifetime
    UNORDERED_MAP_t<HSteamNetConnection, std::shared_ptr<SteamSocket>> m_connected;

    CSteamID m_lobbyID;

    void OnLobbyCreated(LobbyCreated_t* pCallback, bool failure);
    CCallResult<AcceptorSteam, LobbyCreated_t> m_lobbyCreatedCallResult;

    //ISteamNetworkingSockets* m_steamNetworkingSockets;

public:
    static ISteamNetworkingSockets* STEAM_NETWORKING_SOCKETS;

public:
    AcceptorSteam();
    ~AcceptorSteam() override;

    void Listen() override;
    void Close() override;

    ISocket::Ptr Accept() override;

    void OnConfigLoad(bool reloading) override;

    //void Cleanup(ISocket* socket) override;

private:
    // Expanded from the STEAM_CALLBACK macro
    // https://partner.steamgames.com/doc/sdk/api#callbacks
    //STEAM_CALLBACK(AcceptorSteam, OnSteamStatusChanged, SteamNetConnectionStatusChangedCallback_t);
    //STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamStatusChanged, SteamNetConnectionStatusChangedCallback_t);
    struct CCallbackInternal_OnSteamStatusChanged : private CCallbackImpl< sizeof(SteamNetConnectionStatusChangedCallback_t) > {
        CCallbackInternal_OnSteamStatusChanged() {
            if (VH_SETTINGS.serverDedicated)
                this->SetGameserverFlag();
            SteamAPI_RegisterCallback(this, SteamNetConnectionStatusChangedCallback_t::k_iCallback);
        } CCallbackInternal_OnSteamStatusChanged(const CCallbackInternal_OnSteamStatusChanged&) {
            if (VH_SETTINGS.serverDedicated)
                this->SetGameserverFlag();
            SteamAPI_RegisterCallback(this, SteamNetConnectionStatusChangedCallback_t::k_iCallback);
        } CCallbackInternal_OnSteamStatusChanged& operator=(const CCallbackInternal_OnSteamStatusChanged&) {
            return *this;
        } private: virtual void Run(void* pvParam) {
            AcceptorSteam* pOuter = reinterpret_cast<AcceptorSteam*>(reinterpret_cast<char*>(this) - ((::size_t) & reinterpret_cast<char const volatile&>((((AcceptorSteam*)0)->m_steamcallback_OnSteamStatusChanged)))); pOuter->OnSteamStatusChanged(reinterpret_cast<SteamNetConnectionStatusChangedCallback_t*>(pvParam));
        }
    } m_steamcallback_OnSteamStatusChanged; void OnSteamStatusChanged(SteamNetConnectionStatusChangedCallback_t* pParam);
    
    // status logs
    //STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServersConnected, SteamServersConnected_t);
    //STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServersDisconnected, SteamServersDisconnected_t);
    //STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServerConnectFailure, SteamServerConnectFailure_t);
};
