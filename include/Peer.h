#pragma once

#include "VUtils.h"
#include "VUtilsString.h"
#include "Method.h"
#include "NetSocket.h"
#include "Task.h"
#include "DataWriter.h"
#include "ValhallaServer.h"
#include "Hashes.h"
#include "ZDO.h"

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
    std::chrono::steady_clock::time_point m_lastPing;

    UNORDERED_MAP_t<HASH_t, std::unique_ptr<Method>> m_methods;

public:
    NetSocket::Ptr m_socket;

    // Immutable variables
    OWNER_t m_uuid;
    std::string m_name;

    // Mutable variables
    Vector3f m_pos;
    bool m_visibleOnMap = false;
    ZDOID m_characterID;
    bool m_admin = false;

public:
    UNORDERED_MAP_t<ZDOID, std::pair<ZDO::Rev, float>> m_zdos;
    UNORDERED_SET_t<ZDOID> m_forceSend;
    UNORDERED_SET_t<ZDOID> m_invalidSector;

private:
    void Update();

    void ZDOSectorInvalidated(ZDO& zdo);

    void ForceSendZDO(ZDOID id) {
        m_forceSend.insert(id);
    }

    bool IsOutdatedZDO(ZDO& zdo, decltype(m_zdos)::iterator& outItr);
    bool IsOutdatedZDO(ZDO& zdo) {
        decltype(m_zdos)::iterator outItr;
        return IsOutdatedZDO(zdo, outItr);
    }

public:
    Peer(NetSocket::Ptr socket);

    Peer(const Peer& other) = delete; // copy

    ~Peer() {
        //VLOG(1) << "~Peer()";
    }

    /**
        * @brief Register a static method for remote invocation
        * @param name function name to register
        * @param lambda
    */
    template<typename F>
    void Register(HASH_t hash, F func) {
        //VLOG(1) << hash;
        m_methods[hash] = std::make_unique<MethodImpl<Peer*, F>>(func, 0, hash);
    }

    template<typename F>
    decltype(auto) Register(std::string_view name, F func) {
        return Register(VUtils::String::GetStableHashCode(name), func);
    }


    // More efficient way to send a nested array packet to a peer
    //  especially on embedded esp32
    template <typename Func, typename... Types>
    void SubInvoke(HASH_t hash, Func func) {
        if (!m_socket->Connected())
            return;

        BYTES_t bytes;
        DataWriter writer(bytes);

        writer.Write(hash);
        writer.SubWrite(func);
        
        this->Send(std::move(bytes));
    }

    template <typename... Types>
    void Invoke(HASH_t hash, const Types&... params) {
        if (!m_socket->Connected())
            return;

        this->Send(DataWriter::Serialize(hash, params...));
    }

    template <typename... Types>
    decltype(auto) Invoke(std::string_view name, const Types&... params) {
        return Invoke(VUtils::String::GetStableHashCode(name), params...);
    }



    bool InternalInvoke(HASH_t hash, DataReader &reader) {
        auto&& find = m_methods.find(hash);
        if (find != m_methods.end()) {
            //VLOG(2) << "InternalInvoke, hash: " << hash;

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

    decltype(auto) InternalInvoke(std::string_view name, DataReader& reader) {
        return InternalInvoke(VUtils::String::GetStableHashCode(name), reader);
    }



    void Send(BYTES_t bytes) {
        assert(!bytes.empty());
        this->m_socket->Send(std::move(bytes));
    }

    std::optional<BYTES_t> Recv() {
        if (auto&& opt = this->m_socket->Recv()) {
            return *opt;
        }
        return std::nullopt;
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

    void Kick() {
        SendKicked();
        Disconnect();
    }

    bool Close(ConnectionStatus status);



    // Higher utility functions once authenticated



    ZDO* GetZDO();

    void Teleport(Vector3f pos, Quaternion rot, bool animation);

    void Teleport(Vector3f pos) {
        Teleport(pos, Quaternion::IDENTITY, false);
    }

    // Show a specific chat message
    void ChatMessage(std::string_view msg, ChatMsgType type, Vector3f pos, const UserProfile& profile, std::string_view senderID) {
        this->Route(Hashes::Routed::ChatMessage,
            pos,
            type,
            profile,
            msg,
            senderID
        );
    }
    // Show a chat message (string, string_view, tuple<Strings...>
    void ChatMessage(std::string_view msg) {
        this->Route(Hashes::Routed::ChatMessage,
            Vector3f(10000, 10000, 10000),
            ChatMsgType::Normal,
            std::string_view(""), std::string_view("<color=yellow><b>SERVER</b></color>"), std::string_view(""),
            msg,
            ""
        );
    }

    // Show a console message
    void ConsoleMessage(std::string_view msg) {
        return Invoke(Hashes::Rpc::S2C_ConsoleMessage, msg);
    }

private:
    // Show a screen message
    void UIMessage(std::string_view msg, UIMsgType type) {
        this->Route(Hashes::Routed::S2C_UIMessage, type, msg);
    }

public:
    // Show a corner screen message
    void CornerMessage(std::string_view msg) {
        return UIMessage(msg, UIMsgType::TopLeft);
    }

    // Show a center screen message
    void CenterMessage(std::string_view msg) {
        return UIMessage(msg, UIMsgType::Center);
    }



    void RouteParams(ZDOID targetZDO, HASH_t hash, BYTES_t params);



    template <typename... Types>
    void RouteView(ZDOID targetZDO, HASH_t hash, Types&&... params) {
        RouteParams(targetZDO, hash, DataWriter::Serialize(params...));
    }

    template <typename... Types>
    decltype(auto) RouteView(ZDOID targetZDO, std::string_view name, Types&&... params) {
        return RouteView(targetZDO, VUtils::String::GetStableHashCode(name), std::forward<Types>(params)...);
    }

    template <typename... Types>
    decltype(auto) Route(HASH_t hash, Types&&... params) {
        return RouteView(ZDOID::NONE, hash, std::forward<Types>(params)...);
    }

    template <typename... Types>
    decltype(auto) Route(std::string_view name, Types&&... params) {
        return RouteView(ZDOID::NONE, VUtils::String::GetStableHashCode(name), std::forward<Types>(params)...);
    }



    /*
    friend std::ostream& operator<<(std::ostream& ost, const Peer& peer) {
        return ost << peer.m_name << " (" << peer.m_socket << ")";
    }*/
};
