#include "CompileSettings.h"

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    #include <quill/core/LogLevel.h>
    #include <sol/forward.hpp>
    #include <sol/property.hpp>

    #include "ModManager.h"
    #include "NetManager.h"
    #include "NetSocket.h"
    #include "ValhallaServer.h"

using namespace avledet::network;

void IScriptManager::load_userdata_network()
{
    //AVL_LOGGER->set_log_level(quill::LogLevel::Info);

    LOG_DEBUG(AVL_LOGGER, "Initializing API types - network");

    m_state.new_enum("NetStatus", "CONNECTING", Status::Connecting, "CONNECTED", Status::Connected,
                     "LINGERING", Status::Lingering, "CLOSED", Status::Closed, "CONNECT_FAILED",
                     Status::Connect_Failed);

    // TODO full socket impl
    this->new_usertype<ISocket>(
            "Socket", "close", &ISocket::Close,
            //"connected", sol::property(&ISocket::Connected),
            "address", sol::property(&ISocket::get_address), "host", sol::property(&ISocket::get_host_name),
            "send_queue_size", sol::property(&ISocket::get_send_queue_size), "status",
            sol::property(&ISocket::get_status), "send", sol::property(&ISocket::send), "ping",
            sol::property(&ISocket::get_ping), "quality", sol::property(&ISocket::get_connection_quality),
            "outbound", sol::property(&ISocket::is_outbound));


    this->new_usertype<INetManager>(
            "INetManager", "get_peer",
            sol::overload([](INetManager &self,
                             Int64Wrapper owner) { return self.FindPeerByUserID((std::int64_t) owner); },
                          //sol::resolve<Peer*(UserID)>(&INetManager::GetPeer),
                          sol::resolve<Peer::Ptr(std::string_view)>(&INetManager::FindPeerByName)),
            "peers", sol::property(&INetManager::GetPeers));
}

#endif
