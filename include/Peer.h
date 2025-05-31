#pragma once

#include "VUtils.h"
#include "VUtilsString.h"
#include "Method.h"
#include "NetSocket.h"
#include "Task.h"
#include "Stream.h"
#include "ValhallaServer.h"
#include "Hashes.h"
#include "ZDO.h"

namespace avledet::game {

    enum class ConnectionStatus : int32_t {
        None,
        Connecting,
        Connected,
        ErrorVersion,
        ErrorDisconnected,
        ErrorConnectFailed,
        ErrorPassword,
        ErrorAlreadyConnected,
        ErrorBanned,
        ErrorFull,
        ErrorPlatformExcluded,
        ErrorCrossplayPrivilege,
        ErrorKicked,
        MAX // 13
    };



    class Peer : public std::enable_shared_from_this<Peer>, public RpcBase<std::shared_ptr<Peer>> /* dont change */ {
    public:
        using Ptr = std::shared_ptr<Peer>;

        // Internal use only!
        // TODO when and/or if smart ptrs migrate to friend-interfaces to wrap/hide unsafe non-smart ptr creation
        Peer(avledet::network::Socket::Ptr socket);

    public:
        // hide construction to avoid creating a non-shared ptr
        static Peer::Ptr create(avledet::network::Socket::Ptr socket) {
            return std::make_shared<Peer>(std::move(socket));
        }

        void update();

        void disconnect(ConnectionStatus status);

        void disconnect();

        template <typename... Ts>
        void invoke(avledet::util::Hash hash, const Ts&... params) {
            m_socket->send(avledet::util::Writer::serialize(hash, params...));
        }

        //template<typename F>
        //void register_method_admin(avledet::util::Hash hash, F func) {
        //    m_methods.emplace(hash, std::make_unique<avledet::util::Method<T>>([]() {func}));
        //}

    public:
        ankerl::unordered_dense::map<std::string, std::string> m_sync_data;
        std::string m_name;
        avledet::network::Socket::Ptr m_socket;
        avledet::util::CSU::Vector3f m_pos;
        avledet::sync::ZDOID m_zdoid;
        bool m_public{};

        bool m_accepted{}; // Whether PeerInfo successful; accepted into server
    };

    class RoutedRpc : public RpcBase<avledet::util::UserID> {
    public:
        class Data {
        public:
            //public long m_msgID;
            avledet::util::UserID m_sender;
            avledet::util::UserID m_dest;
            avledet::util::ZDOID m_dest_zdo;
            avledet::util::Hash m_method_hash;

            std::vector<char> m_parameters;
        };

    private:
        Peer::Ptr get_peer(avledet::util::UserID id);

        void route(Data data);
        void handle_invoke(Data data);

    public:
        void on_new_peer(Peer::Ptr peer);

        template <typename... Ts>
        void invoke(avledet::util::UserID dest, avledet::sync::ZDOID dest_zdo, avledet::util::Hash hash, const Ts&... params) {
            //m_socket->send(avledet::util::Writer::serialize(hash, params...));

            // 
            assert(false);
        }

        template <typename... Ts>
        void invoke(avledet::util::UserID dest, avledet::util::Hash hash, const Ts&... params) {
            //m_socket->send(avledet::util::Writer::serialize(hash, params...));

            // 
            assert(false);
        }

    private:
        std::vector<Peer::Ptr> m_peers;
    };

}// namespace avledet::game

template <>
struct avledet::util::Streamer<avledet::stream::RoutedRpc::Data> {
    void operator()(Writer& writer, avledet::stream::RoutedRpc::Data const& value) const {
        writer.write(
            (std::int64_t)0,
            value.m_sender,
            value.m_dest,
            value.m_dest_zdo,
            value.m_method_hash,
            value.m_parameters
        );
    }

    decltype(auto) operator()(Reader& reader) const {
        avledet::stream::RoutedRpc::Data data;

        reader.read<std::int64_t>(); // unused

        data.m_sender = reader.read<avledet::util::UserID>();
        data.m_dest = reader.read<avledet::util::UserID>();
        data.m_dest_zdo = reader.read<avledet::util::ZDOID>();
        data.m_method_hash = reader.read<avledet::util::Hash>();
        data.m_parameters = reader.read<std::vector<char>>();

        return data;
    }
};
