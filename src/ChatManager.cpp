#include "ChatManager.h"
#include "Vector.h"
#include "Utils.h"
#include "NetRouteManager.h"
#include "NetManager.h"

namespace ChatManager {

    enum class Type : int32_t {
        Whisper,
        Normal,
        Shout,
        Ping
    };

    void RPC_ChatMessage(OWNER_t sender, Vector3 position, Type type, std::string name, std::string text, std::string senderAccountId);
    void WATCHER_RPC_ChatMessage(OWNER_t sender, Vector3 position, Type type, std::string name, std::string text, std::string senderAccountId);

    void Init() {
        //NetRouteManager::_Register(Routed_Hash::ChatMessage, &RPC_ChatMessage, &WATCHER_RPC_ChatMessage);
        NetRouteManager::Register(Routed_Hash::ChatMessage, &RPC_ChatMessage);
    }



    void RPC_ChatMessage(OWNER_t sender, Vector3 position, Type type, std::string name, std::string text, std::string senderAccountId) {
        LOG(INFO) << "Received chat '" << text << "' from " << name << ", type: " << static_cast<int>(type) << ", " << senderAccountId;

        auto &&peer = NetManager::GetPeer(sender);
        if (peer) {
            LOG(INFO) << "The peer name is " << peer->m_name;
        }

        //this.OnNewChatMessage(null, sender, position, (Talker.Type)type, name, text, senderAccountId);
    }

    void WATCHER_RPC_ChatMessage(OWNER_t sender, Vector3 position, Type type, std::string name, std::string text, std::string senderAccountId) {
        // this asserts that the data the client is sending to other clients is correct
    }



}
