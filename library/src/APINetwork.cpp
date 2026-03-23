#include "CompileSettings.h"

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    #include <quill/core/LogLevel.h>
    #include <sol/forward.hpp>
    #include <sol/property.hpp>

    #include "Avledet.h"
    #include "ModManager.h"
    #include "NetManager.h"
    #include "NetSocket.h"

using namespace avledet::network;

void IScriptManager::load_userdata_network()
{
    // clang-format off

    LOG_DEBUG(AVL_LOGGER, "Initializing API types - network");

    this->new_enum("NetStatus", 
        "CONNECTING", Status::Connecting, 
        "CONNECTED", Status::Connected,
        "LINGERING", Status::Lingering, 
        "CLOSED", Status::Closed, 
        "CONNECT_FAILED", Status::Connect_Failed
    );

    // TODO full socket impl
    this->new_usertype<ISocket>("Socket", 
        sol::no_constructor,
        "close", &ISocket::close,
        "address", sol::property(&ISocket::get_address), 
        "host", sol::property(&ISocket::get_host_name),
        "send_queue_size", sol::property(&ISocket::get_send_queue_size), 
        "status", sol::property(&ISocket::get_status), 
        "send", &ISocket::send, 
        "ping", sol::property(&ISocket::get_ping), 
        "quality", sol::property(&ISocket::get_connection_quality), //TODO this is a func, interpreted as a prop, which returns multiples values. TEST
        "outbound", sol::property(&ISocket::is_outbound)
    );

    this->new_usertype<INetManager>("INetManager", 
        sol::no_constructor,
        "find_peer", &INetManager::FindPeer,
        "peers", sol::property(&INetManager::GetPeers) //TODO require immutable container
    );

    // clang-format on
}

#endif
