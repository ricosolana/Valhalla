#include "Peer.h"
#include "NetManager.h"
#include "ValhallaServer.h"
#include "ZoneManager.h"
#include "Hashes.h"
#include "RouteManager.h"
#include "ZDOManager.h"

void Peer::Update() {
    OPTICK_EVENT();

    auto now(steady_clock::now());

    // Send packet data
    m_socket->Update();

    // Read packets
    while (auto opt = m_socket->Recv()) {

        auto&& bytes = opt.value();
        DataReader reader(bytes);

        auto hash = reader.Read<HASH_t>();
        if (hash == 0) {
            if (reader.Read<bool>()) {
                // Reply to the server with a pong
                bytes.clear();
                DataWriter writer(bytes);
                writer.Write<HASH_t>(0);
                writer.Write<bool>(false);
                m_socket->Send(std::move(bytes));
            }
            else {
                m_lastPing = now;
            }
        }
        else {
            if (auto method = GetMethod(hash))
                method->Invoke(this, reader);
        }
    }

    if (now - m_lastPing > SERVER_SETTINGS.socketTimeout) {
        LOG(INFO) << "Client RPC timeout";
        m_socket->Close(false);
    }
}

IMethod<Peer*>* Peer::GetMethod(HASH_t hash) {
    auto&& find = m_methods.find(hash);
    if (find != m_methods.end()) {
        return find->second.get();
    }
    return nullptr;
}



void Peer::ConsoleMessage(const std::string& msg) {
    Invoke(Hashes::Rpc::S2C_ConsoleMessage, msg);
}

void Peer::Kick() {
    LOG(INFO) << "Kicking " << m_name;
        
    SendKicked();
    Disconnect();
}



void Peer::ChatMessage(const std::string& text, ChatMsgType type, const Vector3 &pos, const UserProfile& profile, const std::string& senderID) {
    RouteManager()->Invoke(m_uuid, Hashes::Routed::ChatMessage, 
        pos, //Vector3(10000, 10000, 10000),
        type,
        profile, //"<color=yellow><b>SERVER</b></color>",
        text,
        senderID //""
    );
}

void Peer::UIMessage(const std::string& text, UIMsgType type) {
    RouteManager()->Invoke(m_uuid, Hashes::Routed::S2C_UIMessage, type, text);
}

ZDO* Peer::GetZDO() {
    return ZDOManager()->GetZDO(m_characterID);
}

void Peer::Teleport(const Vector3& pos, const Quaternion& rot, bool animation) {
    RouteManager()->Invoke(m_uuid, Hashes::Routed::S2C_RequestTeleport,
        pos,
        rot,
        animation
    );
}

void Peer::MoveTo(const Vector3& pos, const Quaternion& rot) {
    // hackish method to possibly move a player instantaneously
    if (auto zdo = GetZDO()) {
        zdo->Disown();
        //zdo->SetPosition(pos);
        //zdo->SetRotation(rot);

        auto uuid = m_uuid;

        Valhalla()->RunTaskLater([pos, rot, uuid](Task&) {
            if (auto peer = NetManager()->GetPeer(uuid)) {
                if (auto zdo = peer->GetZDO()) {
                    zdo->Disown();
                    zdo->SetPosition(pos);
                    zdo->SetRotation(rot);
                    Valhalla()->RunTaskLater([pos, rot, uuid](Task&) {
                        if (auto peer = NetManager()->GetPeer(uuid)) {
                            if (auto zdo = peer->GetZDO()) {
                                zdo->SetOwner(peer->m_uuid);
                                //zdo->SetPosition(pos);
                                //zdo->SetRotation(rot);
                            }
                        }
                        // TODO these values must be tweaked to be as low as possible
                        //  This moveto method is so spontaneous it sometimes works
                        //  sometimes it doesnt
                        //  I have no clue why... something related to the client not
                        //      abandoning its zdo
                    }, milliseconds(500 + peer->m_socket->GetPing() * 2));
                }
            }
        }, milliseconds(500 + m_socket->GetPing() * 2));
        //, milliseconds(std::max(80U, m_socket->GetPing() * 2)));
    }
}



void Peer::ZDOSectorInvalidated(ZDO& zdo) {
    if (zdo.IsOwner(this->m_uuid))
        return;

    if (!ZoneManager()->ZonesOverlap(zdo.GetZone(), m_pos)) {
        if (m_zdos.erase(zdo.ID())) {
            m_invalidSector.insert(zdo.ID());
        }
    }
}

void Peer::ForceSendZDO(const ZDOID &id) {
    m_forceSend.insert(id);
}

bool Peer::IsOutdatedZDO(ZDO& zdo, decltype(m_zdos)::iterator& outItr) {
    auto&& find = m_zdos.find(zdo.ID());

    outItr = find;

    return find == m_zdos.end()
        || zdo.m_rev.m_ownerRev > find->second.m_ownerRev
        || zdo.m_rev.m_dataRev > find->second.m_dataRev;
}
