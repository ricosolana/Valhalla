#pragma once

#include <robin_hood.h>
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
#include "RouteManager.h"

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

    robin_hood::unordered_map<HASH_t, std::unique_ptr<Method>> m_methods;

public:
    ISocket::Ptr m_socket;

    // Immutable variables
    OWNER_t m_uuid;
    std::string m_name;

    // Mutable variables
    Vector3 m_pos;
    bool m_visibleOnMap = false;
    ZDOID m_characterID;
    bool m_admin = false;

    robin_hood::unordered_map<ZDOID, ZDO::Rev> m_zdos;
    robin_hood::unordered_set<ZDOID> m_forceSend;
    robin_hood::unordered_set<ZDOID> m_invalidSector;

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
        LOG(DEBUG) << "~Peer()";
    }

    /**
        * @brief Register a static method for remote invocation
        * @param name function name to register
        * @param lambda
    */
    template<typename F>
    void Register(HASH_t hash, F func) {
        m_methods[hash] = std::make_unique<MethodImpl<Peer*, F>>(func, IModManager::EVENT_RpcIn, hash); // TODO use make_unique
    }

    template<typename F>
    decltype(auto) Register(const std::string& name, F func) {
        return Register(VUtils::String::GetStableHashCode(name), func);
    }

    void RegisterLua(const IModManager::MethodSig& sig, const sol::function& func) {
        m_methods[sig.m_hash] = std::make_unique<MethodImplLua<Peer*>>(func, sig.m_types);
    }



    template <typename... Types>
    void Invoke(HASH_t hash, const Types&... params) {
        if (!m_socket->Connected())
            return;

        if (ModManager()->CallEvent(IModManager::EVENT_RpcOut ^ hash, this, params...))
            return;

        BYTES_t bytes;
        DataWriter writer(bytes);

        writer.Write(hash);
        DataWriter::_Serialize(writer, params...); // serialize

        m_socket->Send(std::move(bytes));
    }

    template <typename... Types>
    decltype(auto) Invoke(const std::string& name, const Types&... params) {
        return Invoke(VUtils::String::GetStableHashCode(name), params...);
    }



    void InvokeLua(const IModManager::MethodSig& repr, const sol::variadic_args& args) {
        if (args.size() != repr.m_types.size())
            throw std::runtime_error("mismatched number of args");

        BYTES_t bytes;
        DataWriter params(bytes);

        params.Write(repr.m_hash);
        DataWriter::_SerializeLua(params, repr.m_types, args);

        m_socket->Send(std::move(bytes));
    }



    Method* GetMethod(HASH_t hash) {
        auto&& find = m_methods.find(hash);
        if (find != m_methods.end()) {
            return find->second.get();
        }
        return nullptr;
    }

    decltype(auto) GetMethod(const std::string& name) {
        return GetMethod(VUtils::String::GetStableHashCode(name));
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

    void Close(ConnectionStatus status);



    // Higher utility functions once authenticated



    ZDO* GetZDO();

    void Teleport(const Vector3& pos, const Quaternion& rot, bool animation);

    void Teleport(const Vector3& pos) {
        Teleport(pos, Quaternion::IDENTITY, false);
    }

    // Show a specific chat message
    void ChatMessage(const std::string& text, ChatMsgType type, const Vector3& pos, const UserProfile& profile, const std::string& senderID);
    // Show a chat message
    void ChatMessage(const std::string& text) {
        //ChatMessage(text, ChatMsgType::Normal, Vector3(10000, 10000, 10000), "<color=yellow><b>SERVER</b></color>", "");

        auto profile = UserProfile("", "<color=yellow><b>SERVER</b></color>", "");

        ChatMessage(text, ChatMsgType::Normal, Vector3(10000, 10000, 10000), profile, "");
    }
    // Show a console message
    decltype(auto) ConsoleMessage(const std::string& msg) {
        return Invoke(Hashes::Rpc::S2C_ConsoleMessage, msg);
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



    template <typename... Types>
    void Route(const ZDOID& targetZDO, HASH_t hash, const Types&... params) {
        IRouteManager::Data data;
        data.m_sender = SERVER_ID;
        data.m_target = this->m_uuid;
        data.m_targetZDO = targetZDO;
        data.m_method = hash;
        data.m_params = DataWriter::Serialize(params...);

        BYTES_t bytes;
        data.Serialize(DataWriter(bytes));

        Invoke(Hashes::Rpc::RoutedRPC, bytes);
    }

    template <typename... Types>
    decltype(auto) Route(HASH_t hash, const Types&... params) {
        return Route(ZDOID::NONE, hash, params...);
    }

    template <typename... Types>
    decltype(auto) Route(const std::string& name, const Types&... params) {
        return Route(ZDOID::NONE, VUtils::String::GetStableHashCode(name), params...);
    }

    template <typename... Types>
    decltype(auto) Route(const ZDOID& targetZDO, const std::string& name, const Types&... params) {
        return Route(targetZDO, VUtils::String::GetStableHashCode(name), params...);
    }



    void RouteViewLua(const ZDOID& targetZDO, const IModManager::MethodSig& repr, const sol::variadic_args& args) {
        if (args.size() != repr.m_types.size())
            throw std::runtime_error("mismatched number of args");

        IRouteManager::Data data;
        data.m_sender = SERVER_ID;
        data.m_target = this->m_uuid;
        data.m_targetZDO = targetZDO;
        data.m_method = repr.m_hash;
        data.m_params = DataWriter::SerializeLua(repr.m_types, args);

        BYTES_t bytes;
        data.Serialize(DataWriter(bytes));

        Invoke(Hashes::Rpc::RoutedRPC, bytes);
    }

    decltype(auto) RouteLua(const IModManager::MethodSig& repr, const sol::variadic_args& args) {
        return RouteViewLua(ZDOID::NONE, repr, args);
    }
};
