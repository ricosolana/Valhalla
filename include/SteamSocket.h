#pragma once

#include <NetSocket.h>
#include <list>
#include <steamnetworkingtypes.h>
#include <isteamnetworkingsockets.h>

namespace avledet::network {

    class SteamSocket : public Socket {
        friend class SteamContext;

    private:
        void send_queued();

        void init_identifiers();

    protected:
        static bool is_game_server() {
            auto game_server = SteamGameServerNetworkingSockets();
            return game_server != nullptr;
        }

        static ISteamNetworkingSockets* get_steam_sockets() {
            auto game_server = SteamGameServerNetworkingSockets();
            return game_server ? game_server : SteamNetworkingSockets();
        }

    public:
        using Ptr = std::shared_ptr<SteamSocket>;

        explicit SteamSocket(HSteamNetConnection hConn, bool is_outbound);
        ~SteamSocket() override;

        void flush();

        void close(bool linger) override;

        void send(std::vector<char> bytes) override;
        std::vector<char> recv() override;

        std::string get_hostname() override;
        std::string get_address() override;
        bool is_outbound() override;

        Status get_status() override;
        int get_ping() override;
        int get_send_queue_size() override;
        //std::tuple<float, float, int, float, float> get_connection_stats() override;
        std::tuple<float, float> get_connection_quality() override;

    private:
        SteamNetworkingIdentity m_steam_id{};
        std::list<std::vector<char>> m_send_queue;
        std::string m_address;
        HSteamNetConnection m_conn{};
        Status m_status{};
        const bool m_is_outbound;
    };

}// namespace avledet::network