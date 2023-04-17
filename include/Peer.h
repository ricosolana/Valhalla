#pragma once

// TODO use a wrapper method?
#include <openssl/md5.h>

#include "VUtils.h"
#include "VUtilsString.h"
#include "Method.h"
#include "NetSocket.h"
#include "Task.h"
#include "DataWriter.h"
#include "ValhallaServer.h"
#include "Hashes.h"
#include "ZDO.h"
#include "RouteData.h"

enum class ChatMsgType : int32_t {
    Whisper,
    Normal,
    Shout,
    Ping
};

enum class ConnectionStatus : int32_t {
    None,
    Connecting,
    Connected,
    ErrorVersion,
    ErrorDisconnected,
    ErrorConnectFailed,
    ErrorPassword,
    ErrorAlreadyConnected,
    ErrorBanned,
    ErrorFull,
    ErrorPlatformExcluded,
    ErrorCrossplayPrivilege,
    ErrorKicked,
    MAX // 13
};

class Peer {
    friend class IZDOManager;
    friend class INetManager;
    friend class IModManager;

public:
    using Method = IMethod<Peer*>;

private:
    static std::string SALT;
    static std::string PASSWORD;

private:
    std::chrono::steady_clock::time_point m_lastPing;

    UNORDERED_MAP_t<HASH_t, std::unique_ptr<Method>> m_methods;

public:
    ISocket::Ptr m_socket;

    // Immutable variables
    OWNER_t m_uuid;
    std::string m_name;

    // Mutable variables
    Vector3f m_pos;
    bool m_visibleOnMap = false;
    ZDOID m_characterID;
    bool m_admin = false;

    UNORDERED_MAP_t<ZDOID, ZDO::Rev> m_zdos;
    UNORDERED_SET_t<ZDOID> m_forceSend;
    UNORDERED_SET_t<ZDOID> m_invalidSector;

private:
    void Update();

    void ZDOSectorInvalidated(ZDO& zdo);

    void ForceSendZDO(const ZDOID& id) {
        m_forceSend.insert(id);
    }

    bool IsOutdatedZDO(ZDO& zdo, decltype(m_zdos)::iterator& outItr);
    bool IsOutdatedZDO(ZDO& zdo) {
        decltype(m_zdos)::iterator outItr;
        return IsOutdatedZDO(zdo, outItr);
    }

public:
    Peer(ISocket::Ptr socket);

    Peer(const Peer& other) = delete; // copy

    ~Peer() {
        VLOG(1) << "~Peer()";
    }

    /**
        * @brief Register a static method for remote invocation
        * @param name function name to register
        * @param lambda
    */
    template<typename F>
    void Register(HASH_t hash, F func) {
        VLOG(1) << "Register, hash: " << hash;
        m_methods[hash] = std::make_unique<MethodImpl<Peer*, F>>(func);
    }

    template<typename F>
    decltype(auto) Register(const std::string& name, F func) {
        return Register(VUtils::String::GetStableHashCode(name), func);
    }



    template <typename... Types>
    void Invoke(HASH_t hash, const Types&... params) {
        if (!m_socket->Connected())
            return;

        VLOG(2) << "Invoke, hash: " << hash << ", #params: " << sizeof...(params);

        m_socket->Send(DataWriter::Serialize(hash, params...));
    }

    template <typename... Types>
    decltype(auto) Invoke(const std::string& name, const Types&... params) {
        return Invoke(VUtils::String::GetStableHashCode(name), params...);
    }



    // Invoke method with pre-initialized packet data
    //  To be used when invoking a method that takes a BYTES_t or other large types
    template <typename Func>
    void PrepInvoke(HASH_t hash, Func func) {
        BYTES_t bytes;
        DataWriter writer(bytes);

        writer.Write(hash);

        func(writer);

        m_socket->Send(std::move(bytes));
    }



    bool InternalInvoke(HASH_t hash, DataReader &reader) {
        auto&& find = m_methods.find(hash);
        if (find != m_methods.end()) {
            VLOG(2) << "InternalInvoke, hash: " << hash;

            auto result = find->second->Invoke(this, reader);
            if (!result) {
                // this is UB in cases where a method is added by the Invoked func
                //  insertions of deletions invalidate iterators, causing the crash
                //m_methods.erase(find); 
                m_methods.erase(hash);
            }
            return result;
        }
        return true;
    }

    decltype(auto) InternalInvoke(const std::string& name, DataReader& reader) {
        return InternalInvoke(VUtils::String::GetStableHashCode(name), reader);
    }



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

    void Kick() {
        SendKicked();
        Disconnect();
    }

    bool Close(ConnectionStatus status);



    // Higher utility functions once authenticated



    ZDO* GetZDO();

    void Teleport(const Vector3f& pos, const Quaternion& rot, bool animation);

    void Teleport(const Vector3f& pos) {
        Teleport(pos, Quaternion::IDENTITY, false);
    }

    // Show a specific chat message
    void ChatMessage(const std::string& text, ChatMsgType type, const Vector3f& pos, const UserProfile& profile, const std::string& senderID);
    // Show a chat message
    void ChatMessage(const std::string& text) {
        //ChatMessage(text, ChatMsgType::Normal, Vector3f(10000, 10000, 10000), "<color=yellow><b>SERVER</b></color>", "");

        auto profile = UserProfile("", "<color=yellow><b>SERVER</b></color>", "");

        ChatMessage(text, ChatMsgType::Normal, Vector3f(10000, 10000, 10000), profile, "");
    }
    // Show a console message
    decltype(auto) ConsoleMessage(const std::string& msg) {
        return Invoke(Hashes::Rpc::S2C_ConsoleMessage, std::string_view(msg));
    }
    // Show a screen message
    void UIMessage(const std::string& text, UIMsgType type);
    // Show a corner screen message
    decltype(auto) CornerMessage(const std::string& text) {
        return UIMessage(text, UIMsgType::TopLeft);
    }
    // Show a center screen message
    decltype(auto) CenterMessage(const std::string& text) {
        return UIMessage(text, UIMsgType::Center);
    }



    void RouteParams(const ZDOID& targetZDO, HASH_t hash, BYTES_t params);



    template <typename... Types>
    void RouteView(const ZDOID& targetZDO, HASH_t hash, Types&&... params) {
        RouteParams(targetZDO, hash, DataWriter::Serialize(params...));
    }

    template <typename... Types>
    decltype(auto) RouteView(const ZDOID& targetZDO, const std::string& name, Types&&... params) {
        return RouteView(targetZDO, VUtils::String::GetStableHashCode(name), std::forward<Types>(params)...);
    }

    template <typename... Types>
    decltype(auto) Route(HASH_t hash, Types&&... params) {
        return RouteView(ZDOID::NONE, hash, std::forward<Types>(params)...);
    }

    template <typename... Types>
    decltype(auto) Route(const std::string& name, Types&&... params) {
        return RouteView(ZDOID::NONE, VUtils::String::GetStableHashCode(name), std::forward<Types>(params)...);
    }
};
