#include "ChatManager.h"
#include "Vector.h"
#include "Utils.h"
#include "NetRouteManager.h"
#include "NetManager.h"

namespace ChatManager {

    void RPC_ChatMessage(OWNER_t sender, Vector3 position, Type type, std::string name, std::string text, std::string senderAccountId);
    void WATCHER_RPC_ChatMessage(OWNER_t sender, Vector3 position, Type type, std::string name, std::string text, std::string senderAccountId);

    void Init() {
        //NetRouteManager::_Register(Routed_Hash::ChatMessage, &RPC_ChatMessage, &WATCHER_RPC_ChatMessage);
        NetRouteManager::Register(Routed_Hash::ChatMessage, &RPC_ChatMessage);
    }



    void RPC_ChatMessage(OWNER_t sender, Vector3, Type type, std::string, std::string text, std::string) {
        //LOG(INFO) << "Received chat '" << text << "' from " << name << ", type: " << static_cast<int>(type) << ", " << senderAccountId;

        auto &&peer = NetManager::GetPeer(sender);
        if (peer) {
            LOG(INFO) << "The peer name is " << peer->m_name;
        }

        ModManager::CallEvent("ChatMessage", sender, type, text);
        //ModManager::Event::OnChatMessage(sender, type, text);

        // Intended usage
        //ModManager::CallEvent("ChatMessage")



        //this.OnNewChatMessage(null, sender, position, (Talker.Type)type, name, text, senderAccountId);
    }

    void WATCHER_RPC_ChatMessage(OWNER_t sender, Vector3 position, Type type, std::string name, std::string text, std::string senderAccountId) {
        // this asserts that the data the client is sending to other clients is correct
    }



}
