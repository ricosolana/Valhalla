#include <optick.h>

#include "NetRpc.h"
#include "ValhallaServer.h"
#include "NetManager.h"
#include "Hashes.h"

using namespace std::chrono;

static const char* STATUS_STRINGS[] = { "None", "Connecting", "Connected",
    "ErrorVersion", "ErrorDisconnected", "ErrorConnectFailed", "ErrorPassword",
    "ErrorAlreadyConnected", "ErrorBanned", "ErrorFull" };



NetRpc::NetRpc(ISocket::Ptr socket)
    : m_socket(std::move(socket)), m_lastPing(steady_clock::now()) {
}

NetRpc::~NetRpc() {
    LOG(DEBUG) << "~NetRpc()";
}

void NetRpc::Unregister(HASH_t hash) {
    assert(m_methods.contains(hash));

    m_methods.erase(hash);
}

void NetRpc::Unregister(const std::string& name) {
    Unregister(VUtils::String::GetStableHashCode(name));
}

void NetRpc::Update() {
    OPTICK_EVENT();

    auto now(steady_clock::now());

    // Send packet data
    m_socket->Update();

    // Read packets
    while (auto opt = m_socket->Recv()) {
        auto&& pkg = opt.value();

        auto hash = pkg.Read<HASH_t>();
        if (hash == 0) {
            if (pkg.Read<bool>()) {
                // Reply to the server with a pong
                pkg.m_stream.Clear();
                pkg.Write<HASH_t>(0);
                pkg.Write<bool>(false);
                SendPackage(std::move(pkg));
            }
            else {
                m_lastPing = now;
            }
        }
        else {
            auto&& find = m_methods.find(hash);
            if (find != m_methods.end()) {
                find->second->Invoke(this, pkg);
            }
        }
    }
    
    if (now - m_lastPing > SERVER_SETTINGS.socketTimeout) {
        LOG(INFO) << "Client RPC timeout";
        m_socket->Close(false);
    }
}

void NetRpc::SendError(ConnectionStatus status) {
    LOG(INFO) << "Client error: " << STATUS_STRINGS[(int)status];
    Invoke(Hashes::Rpc::Error, status);
    m_socket->Close(true);
}
