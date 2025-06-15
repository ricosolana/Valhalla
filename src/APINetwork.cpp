#include "ModManager.h"
#include "NetManager.h"

#if VH_IS_ON(VH_USE_MODS)

#include <sol/property.hpp>

using namespace avledet::network;

void avledet::api::init_network(sol::table table) {
    table.new_enum("NetStatus",
        "CONNECTING", Status::Connecting,
        "CONNECTED", Status::Connected,
        "LINGERING", Status::Lingering,
        "CLOSED", Status::Closed,
        "CONNECT_FAILED", Status::Connect_Failed
    );

    // TODO full socket impl
    table.new_usertype<ISocket>("Socket",
        "close", &ISocket::Close,
        //"connected", sol::property(&ISocket::Connected),
        "address", sol::property(&ISocket::get_address),
        "host", sol::property(&ISocket::get_host_name),
        "send_queue_size", sol::property(&ISocket::get_send_queue_size),
        "status", sol::property(&ISocket::get_status),
        "send", sol::property(&ISocket::send),
        "ping", sol::property(&ISocket::get_ping),
        "quality", sol::property(&ISocket::get_connection_quality),
        "outbound", sol::property(&ISocket::is_outbound)
    );

    table["NetManager"] = NetManager();
    table.new_usertype<INetManager>("INetManager",
        "get_peer", sol::overload(
            [](INetManager& self, Int64Wrapper owner) { return self.FindPeerByUserID((std::int64_t)owner); },
            //sol::resolve<Peer*(UserID)>(&INetManager::GetPeer),
            sol::resolve<Peer* (std::string_view)>(&INetManager::FindPeerByName)
        ),
        "peers", sol::property(&INetManager::GetPeers)
    );
}

#endif
