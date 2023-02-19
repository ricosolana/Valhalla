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
            InvokeSelf(hash, reader);
            //if (auto method = GetMethod(hash))
            //    method->Invoke(this, reader);

            //auto&& find = m_methods.find(hash);
            //if (find != m_methods.end()) {
            //    find->second->Invoke(this, pkg);
            //}
        }
    }

    if (now - m_lastPing > SERVER_SETTINGS.socketTimeout) {
        LOG(INFO) << "Client RPC timeout";
        m_socket->Close(false);
    }
}

IMethod<Peer*>* Peer::GetMethod(const std::string& name) {
    return GetMethod(VUtils::String::GetStableHashCode(name));
}

IMethod<Peer*>* Peer::GetMethod(HASH_t hash) {
    auto&& find = m_methods.find(hash);
    if (find != m_methods.end()) {
        return find->second.get();
        //find->second->Invoke(this, pkg);
    }
    return nullptr;
}



void Peer::RemotePrint(const std::string& msg) {
    Invoke(Hashes::Rpc::RemotePrint, msg);
}

void Peer::Kick(bool now) {
    LOG(INFO) << "Kicking " << m_name;

    SendDisconnect();
    m_socket->Close(now);
}

void Peer::Kick(std::string reason) {
    if (reason.empty()) reason = "being not fun";

    LOG(INFO) << "Kicking " << m_name << " for: " << reason;

    RemotePrint("kick: " + reason);
    RouteManager()->Invoke(m_uuid, Hashes::Routed::ShowMessage, MessageType::Center, "kick: " + reason);
    
    SendDisconnect();
    Disconnect();
}

void Peer::SendDisconnect() {
    Invoke("Disconnect");
}

void Peer::Disconnect() {
    m_socket->Close(true);
}



void Peer::SendChatMessage(const std::string& text, TalkerType type, Vector3 pos, const std::string& senderName, const std::string& senderID) {
    RouteManager()->Invoke(m_uuid, Hashes::Routed::ChatMessage, 
        pos, //Vector3(10000, 10000, 10000),
        type,
        senderName, //"<color=yellow><b>SERVER</b></color>",
        text,
        senderID //""
    );
}

void Peer::ShowMessage(const std::string& text, MessageType type) {
    RouteManager()->Invoke(m_uuid, Hashes::Routed::ShowMessage, type, text);
}

void Peer::Message(const std::string& text, MsgType type) {
    switch (type) {
    case MsgType::WHISPER:
        SendChatMessage(text, TalkerType::Whisper, Vector3(10000, 10000, 10000), "<color=yellow><b>SERVER</b></color>", "");
        break;
    case MsgType::NORMAL:
        SendChatMessage(text, TalkerType::Normal, Vector3(10000, 10000, 10000), "<color=yellow><b>SERVER</b></color>", "");
        break;
    case MsgType::CONSOLE:
        RemotePrint(text);
        break;
    case MsgType::CORNER:
        ShowMessage(text, MessageType::TopLeft);
        break;
    case MsgType::CENTER:
        ShowMessage(text, MessageType::Center);
        break;
    default:
        throw std::runtime_error("bad enum type");
    }
}

ZDO* Peer::GetZDO() {
    return ZDOManager()->GetZDO(m_characterID);
}



void Peer::ZDOSectorInvalidated(ZDO& zdo) {
    if (zdo.m_owner == m_uuid)
        return;

    if (!ZoneManager()->ZonesOverlap(zdo.Sector(), m_pos)) {
        if (m_zdos.erase(zdo.ID())) {
            m_invalidSector.insert(zdo.ID());
        }
    }
}

void Peer::ForceSendZDO(const ZDOID &id) {
    m_forceSend.insert(id);
}

bool Peer::IsOutdatedZDO(ZDO& zdo) {
    auto find = m_zdos.find(zdo.ID());

    return find == m_zdos.end()
        || zdo.m_rev.m_ownerRev > find->second.m_ownerRev
        || zdo.m_rev.m_dataRev > find->second.m_dataRev;
}
