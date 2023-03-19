#pragma once

#include "VUtils.h"
#include "NetSocket.h"
#include "Method.h"
#include "ZDO.h"
#include "ValhallaServer.h"
#include "ModManager.h"
#include "Hashes.h"
#include "UserData.h"

class IZDOManager;
class INetManager;

enum class ChatMsgType : int32_t {
    Whisper,
    Normal,
    Shout,
    Ping
};



class Peer {
    friend class IZDOManager;
    friend class INetManager;
    friend class IModManager;

private:
    std::chrono::steady_clock::time_point m_lastPing;
    robin_hood::unordered_map<HASH_t, std::unique_ptr<IMethod<Peer*>>> m_methods;

public:
    robin_hood::unordered_map<ZDOID, ZDO::Rev> m_zdos;
    robin_hood::unordered_set<ZDOID> m_forceSend;
    robin_hood::unordered_set<ZDOID> m_invalidSector;

public:
    ISocket::Ptr m_socket;

    const OWNER_t m_uuid;
    const std::string m_name;
    bool m_admin = false;
    //bool m_magicLogin = false;

    // Constantly changing vars
    Vector3 m_pos;
    bool m_visibleOnMap = false;
    ZDOID m_characterID;

    // if many bools are eventually required
    //  then use a bitmask

private:
    void Update();

    void ZDOSectorInvalidated(ZDO& zdo);
    void ForceSendZDO(const ZDOID& id);
    bool IsOutdatedZDO(ZDO& zdo, decltype(m_zdos)::iterator& outItr);
    bool IsOutdatedZDO(ZDO& zdo) {
        decltype(m_zdos)::iterator outItr;
        return IsOutdatedZDO(zdo, outItr);
    }

public:
    Peer(ISocket::Ptr socket, OWNER_t uuid, const std::string &name, const Vector3 &pos)
        : m_socket(std::move(socket)), m_lastPing(steady_clock::now()), 
        m_name(name), m_uuid(uuid), m_pos(pos)
    {}

    Peer(const Peer& peer) = delete;

    ~Peer() {
        LOG(DEBUG) << "~Peer()";
    }

    /**
        * @brief Register a static method for remote invocation
        * @param name function name to register
        * @param lambda
    */
    template<typename F>
    void Register(HASH_t hash, F func) {
        //m_methods[hash] = std::unique_ptr<IMethod<Peer*>>(new MethodImpl(func, EVENT_HASH_RpcIn, hash)); // TODO use make_unique
        //m_methods[hash] = std::make_unique<MethodImpl<Peer*>>(func, EVENT_HASH_RpcIn, hash);
        m_methods[hash] = std::make_unique<MethodImpl<Peer*, F>>(func, EVENT_HASH_RpcIn, hash); // TODO use make_unique
    }

    template<typename F>
    void Register(const std::string& name, F func) {
        Register(VUtils::String::GetStableHashCode(name), func);
    }

    void Register(MethodSig sig, sol::function func) {
        m_methods[sig.m_hash] = std::make_unique<MethodImplLua<Peer*>>(func, sig.m_types);
    }

    // Invoke a function on the remote client
    //  *NOT* thread safe, do not call this function on
    //  any Peer* instance from more than 1 thread!
    template <typename... Types>
    void Invoke(HASH_t hash, const Types&... params) {
        if (!m_socket->Connected())
            return;

        if (ModManager()->CallEvent(EVENT_HASH_RpcOut ^ hash, this, params...) == EventStatus::CANCEL)
            return;

        static BYTES_t bytes; bytes.clear();
        DataWriter writer(bytes);

        writer.Write(hash);
        DataWriter::_Serialize(writer, params...); // serialize

        m_socket->Send(std::move(bytes));
    }

    template <typename... Types>
    void Invoke(const std::string& name, const Types&... params) {
        Invoke(VUtils::String::GetStableHashCode(name), params...);
    }



    //bool InvokeSelf(HASH_t hash, DataReader reader) {
    //    if (auto method = GetMethod(hash)) {
    //        method->Invoke(this, reader);
    //        return true;
    //    }
    //    return false;
    //}
    //
    //bool InvokeSelf(const std::string& name, DataReader reader) {
    //    if (auto method = GetMethod(name)) {
    //        method->Invoke(this, reader);
    //        return true;
    //    }
    //    return false;
    //}

    IMethod<Peer*>* GetMethod(HASH_t hash);

    IMethod<Peer*>* GetMethod(const std::string& name) {
        return GetMethod(VUtils::String::GetStableHashCode(name));
    }

    void Kick();

    void Disconnect() {
        m_socket->Close(true);
    }

    void SendDisconnect() {
        Invoke(Hashes::Rpc::Disconnect);
    }

    void SendKicked() {
        Invoke(Hashes::Rpc::S2C_ResponseKicked);
    }

    void RequestSave() {
        Invoke(Hashes::Rpc::C2S_RequestSave);
    }



    // Show a specific chat message
    void ChatMessage(const std::string& text, ChatMsgType type, const Vector3 &pos, const UserProfile& profile, const std::string& senderID);
    // Show a chat message
    void ChatMessage(const std::string& text) {
        //ChatMessage(text, ChatMsgType::Normal, Vector3(10000, 10000, 10000), "<color=yellow><b>SERVER</b></color>", "");
        
        auto profile = UserProfile("", "<color=yellow><b>SERVER</b></color>", "");

        ChatMessage(text, ChatMsgType::Normal, Vector3(10000, 10000, 10000), profile, "");
    }
    // Show a console message
    void ConsoleMessage(const std::string& msg);
    // Show a screen message
    void UIMessage(const std::string& text, UIMsgType type);
    // Show a corner screen message
    void CornerMessage(const std::string& text) {
        UIMessage(text, UIMsgType::TopLeft);
    }
    // Show a center screen message
    void CenterMessage(const std::string& text) {
        UIMessage(text, UIMsgType::Center);
    }



    // Get the Player ZDO
    //  Rarely Nullable (during join/death/quit)
    ZDO* GetZDO();

    void Teleport(const Vector3& pos, const Quaternion& rot, bool animation);

    void Teleport(const Vector3& pos) {
        Teleport(pos, Quaternion::IDENTITY, false);
    }

    // Experimental
    void MoveTo(const Vector3& pos, const Quaternion& rot);

    // Experimental
    void MoveTo(const Vector3& pos) {
        MoveTo(pos, Quaternion::IDENTITY);
    }
};
