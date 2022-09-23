// main.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <assert.h>

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingcustomsignaling.h>
#include "trivial_signaling_client.h"

HSteamListenSocket listenSock;
HSteamNetConnection connection;

void Quit(int rc)
{
    if (rc == 0)
    {
        // OK, we cannot just exit the process, because we need to give
        // the connection time to actually send the last message and clean up.
        // If this were a TCP connection, we could just bail, because the OS
        // would handle it.  But this is an application protocol over UDP.
        // So give a little bit of time for good cleanup.  (Also note that
        // we really ought to continue pumping the signaling service, but
        // in this exampple we'll assume that no more signals need to be
        // exchanged, since we've gotten this far.)  If we just terminated
        // the program here, our peer could very likely timeout.  (Although
        // it's possible that the cleanup packets have already been placed
        // on the wire, and if they don't drop, things will get cleaned up
        // properly.)
        std::cout << "Waiting for any last cleanup packets.\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    GameNetworkingSockets_Kill();
    exit(rc);
}

void SendMessageToPeer(const char* pszMsg)
{
    printf("Sending msg '%s'\n", pszMsg);
    EResult r = SteamNetworkingSockets()->SendMessageToConnection(
        connection, pszMsg, (int) strlen(pszMsg) + 1, k_nSteamNetworkingSend_Reliable, nullptr);
    assert(r == k_EResultOK);
}

int main(int argc, char **argv) {

    SteamNetworkingIdentity localIdentity; localIdentity.Clear();
    assert(localIdentity.ParseString("str:peer_server"));

    SteamNetworkingErrMsg errMsg;
    if (!GameNetworkingSockets_Init(nullptr /*&localIdentity*/, errMsg)) {
        std::cout << "GameNetworkingSockets_Init failed: " << errMsg << "\n";
        return 0;
    }



    SteamNetworkingUtils()->SetGlobalConfigValueString(k_ESteamNetworkingConfig_P2P_STUN_ServerList, "stun.l.google.com:19302");
    SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_P2P_Transport_ICE_Enable, k_nSteamNetworkingConfig_P2P_Transport_ICE_Enable_All);


    // https://partner.steamgames.com/doc/api/ISteamNetworkingSockets#custom_signaling
    ITrivialSignalingClient* pSignaling = CreateTrivialSignalingClient("localhost:10000", SteamNetworkingSockets(), errMsg);
    if (pSignaling == nullptr)
        printf("Failed to initializing signaling client.  %s", errMsg);



    // set callbacks
    SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged([](SteamNetConnectionStatusChangedCallback_t* info) {
        printf("Callback received\n");
        
        switch (info->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
            std::cout << "[" << info->m_info.m_szConnectionDescription << "] "
                << (info->m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer ? "closed by peer" : "problem detected locally")
                << ", reason " << info->m_info.m_eEndReason
                << ": " << info->m_info.m_szEndDebug << "\n";

            SteamNetworkingSockets()->CloseConnection(info->m_hConn, 0, nullptr, false);

            int rc = 0;
            if (rc == k_ESteamNetworkingConnectionState_ProblemDetectedLocally || info->m_info.m_eEndReason != k_ESteamNetConnectionEnd_App_Generic)
                rc = 1; // failure

            Quit(rc);
            break;
        }

        case k_ESteamNetworkingConnectionState_None:
            // We closed the connection, so ignore
            break;

        case k_ESteamNetworkingConnectionState_Connecting:

            // Is this a connection we initiated, or one that we are receiving?
            if (listenSock != k_HSteamListenSocket_Invalid && info->m_info.m_hListenSocket == listenSock)
            {
                // Somebody's knocking
                // Note that we assume we will only ever receive a single connection
                //assert(g_hConnection == k_HSteamNetConnection_Invalid); // not really a bug in this code, but a bug in the test

                printf("[%s] Accepting\n", info->m_info.m_szConnectionDescription);
                connection = info->m_hConn;
                SteamNetworkingSockets()->AcceptConnection(info->m_hConn);
            }
            else
            {
                // Note that we will get notification when our own connection that
                // we initiate enters this state.
                //assert(g_hConnection == pInfo->m_hConn);
                printf("[%s] Entered connecting state\n", info->m_info.m_szConnectionDescription);
            }
            break;

        case k_ESteamNetworkingConnectionState_FindingRoute:
            // P2P connections will spend a brief time here where they swap addresses
            // and try to find a route.
            printf("[%s] finding route\n", info->m_info.m_szConnectionDescription);
            break;

        case k_ESteamNetworkingConnectionState_Connected:
            // We got fully connected
            //assert(pInfo->m_hConn == g_hConnection); // We don't initiate or accept any other connections, so this should be out own connection
            printf("[%s] connected\n", info->m_info.m_szConnectionDescription);
            break;

        default:
            throw std::runtime_error("Impossible state");
            break;
        }
    });


    int nVirtualPortLocal = 0;
    printf("Creating listen socket, local virtual port %d\n", nVirtualPortLocal);
    listenSock = SteamNetworkingSockets()->CreateListenSocketP2P(nVirtualPortLocal, 0, nullptr);
    assert(listenSock != k_HSteamListenSocket_Invalid);

    for (;;)
    {
        // Check for incoming signals, and dispatch them
        pSignaling->Poll();

        // Check callbacks
        //TEST_PumpCallbacks();
        SteamNetworkingSockets()->RunCallbacks();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));

        // If we have a connection, then poll it for messages
        if (connection != k_HSteamNetConnection_Invalid)
        {
            SteamNetworkingMessage_t* pMessage;
            int r = SteamNetworkingSockets()->ReceiveMessagesOnConnection(connection, &pMessage, 1);
            assert(r == 0 || r == 1); // <0 indicates an error
            if (r == 1)
            {
                // In this example code we will assume all messages are '\0'-terminated strings.
                // Obviously, this is not secure.
                //printf("Received message '%s'\n", pMessage->GetData());
                printf("Received message from a client");

                // Free message struct and buffer.
                pMessage->Release();

                // We're the server.  Send a reply.
                SendMessageToPeer("I got your message");
            }
        }
    }

    Quit(0);

	return 0;
}
