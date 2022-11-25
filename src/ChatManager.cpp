#include "ChatManager.h"
#include "Vector.h"
#include "VUtils.h"
#include "NetRouteManager.h"
#include "NetManager.h"
#include <ranges>

namespace ChatManager {

    void RPC_ChatMessage(OWNER_t sender, Vector3 position, Type type, std::string name, std::string text, std::string senderAccountId);

    void Init() {
        //NetRouteManager::_Register(Routed_Hash::ChatMessage, &RPC_ChatMessage, &WATCHER_RPC_ChatMessage);
        NetRouteManager::Register(NetHashes::Routed::ChatMessage, RPC_ChatMessage);
        std::unordered_map<int, int> map;

        //auto lam = [](OWNER_t, int) -> void {};
        //NetRouteManager::Register(NetHashes::Routed::ChatMessage, 
        //    std::function(lam)
        //);
    }



    void RPC_ChatMessage(OWNER_t sender, Vector3, Type type, std::string, std::string text, std::string) {
        //LOG(INFO) << "Received chat '" << text << "' from " << name << ", type: " << static_cast<int>(type) << ", " << senderAccountId;

        auto &&peer = NetManager::GetPeer(sender);
        if (peer) {
            LOG(INFO) << "The peer name is " << peer->m_name;
        }

        CALL_EVENT("ChatMessage", sender, type, text);
        //VModManager::Event::OnChatMessage(sender, type, text);

        // Intended usage
        //VModManager::CallEvent("ChatMessage")



        //this.OnNewChatMessage(null, sender, position, (Talker.Type)type, name, text, senderAccountId);
    }

    void WATCHER_RPC_ChatMessage(OWNER_t sender, Vector3 position, Type type, std::string name, std::string text, std::string senderAccountId) {
        // this asserts that the data the client is sending to other clients is correct
    }



}
