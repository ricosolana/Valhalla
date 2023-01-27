#include "NetRpc.h"
#include "Hashes.h"

void NetRpc::PollOne() {
    m_socket->Update();

    auto opt = m_socket->Recv();
    if (!opt)
        return;

    auto now(steady_clock::now());

    auto&& pkg = opt.value();

    auto hash = pkg.Read<HASH_t>();
    if (hash == 0) {
        if (pkg.Read<bool>()) {
            // Reply to the server with a pong
            pkg.m_stream.Clear();
            pkg.Write<HASH_t>(0);
            pkg.Write<bool>(false);
            m_socket->Send(std::move(pkg));
        }
        else {
            m_lastPing = now;
        }
    }
    else {
        auto&& find = m_methods.find(hash);
        if (find != m_methods.end()) {
            find->second->Invoke(this, pkg);
            m_methods.erase(find);
        }
    }
}

static const char* STATUS_STRINGS[] = { "None", "Connecting", "Connected",
    "ErrorVersion", "ErrorDisconnected", "ErrorConnectFailed", "ErrorPassword",
    "ErrorAlreadyConnected", "ErrorBanned", "ErrorFull" };

void NetRpc::Close(ConnectionStatus status) {
    LOG(INFO) << "Client error: " << STATUS_STRINGS[(int)status];
    Invoke(Hashes::Rpc::Error, status);
    m_socket->Close(true);
}
