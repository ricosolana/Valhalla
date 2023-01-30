#include "Peer.h"
#include "NetManager.h"
#include "ValhallaServer.h"
#include "ZoneManager.h"
#include "Hashes.h"
#include "RouteManager.h"

void Peer::Update() {
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
            }
        }
    }

    if (now - m_lastPing > SERVER_SETTINGS.socketTimeout) {
        LOG(INFO) << "Client RPC timeout";
        m_socket->Close(false);
    }
}



void Peer::RemotePrint(const std::string& msg) {
    Invoke(Hashes::Rpc::RemotePrint, msg);
}

void Peer::Kick(bool now) {
    LOG(INFO) << "Kicking " << m_name;

    SendDisconnect();
    m_socket->Close(now);
}

void Peer::Kick(const std::string& reason) {
    LOG(INFO) << "Kicking " << m_name << " for: " << reason;

    //RemotePrint(reason)
    SendDisconnect();
    Disconnect();    
}

void Peer::SendDisconnect() {
    Invoke("Disconnect");
}

void Peer::Disconnect() {
    m_socket->Close(true);
}

//void Peer::Message() {
//    RouteManager()->Invoke(m_uuid, Hashes::Routed::ChatMessage, 
//            localPlayer.GetHeadPoint(),
//            2,
//            localPlayer.GetPlayerName(),
//            text,
//            PrivilegeManager.GetNetworkUserId()
//        );
//}



void Peer::ZDOSectorInvalidated(ZDO* zdo) {
    if (zdo->m_owner == m_uuid)
        return;

    if (!ZoneManager()->ZonesOverlap(zdo->Sector(), m_pos)) {
        if (m_zdos.erase(zdo->ID())) {
            m_invalidSector.insert(zdo->ID());
        }
    }
}

void Peer::ForceSendZDO(const NetID &id) {
    m_forceSend.insert(id);
}

bool Peer::IsOutdatedZDO(ZDO* zdo) {
    auto find = m_zdos.find(zdo->ID());

    return find == m_zdos.end()
        || zdo->m_rev.m_ownerRev > find->second.m_ownerRev
        || zdo->m_rev.m_dataRev > find->second.m_dataRev;
}
