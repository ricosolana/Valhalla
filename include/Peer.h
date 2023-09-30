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

    constexpr static int VISIBLE_PACK_INDEX = 0;
    constexpr static int ADMIN_PACK_INDEX = 1;
    constexpr static int GATED_PACK_INDEX = 2;

public:
    using Method = IMethod<Peer*>;

private:
    std::chrono::steady_clock::time_point m_lastPing;

    // Elements rarely added/removed
    //  Queried frequently
    UNORDERED_MAP_t<HASH_t, std::unique_ptr<Method>> m_methods;

public:
    // Elements are never removed
    //  Queried frequently, and frequent adds
    ankerl::unordered_dense::segmented_map<ZDOID, std::pair<ZDO::Rev, float>> m_zdos;
    UNORDERED_SET_t<ZDOID> m_forceSend; // TODO this is rarely ever used (only for portal)
    UNORDERED_SET_t<ZDOID> m_invalidSector; // TODO this is also odd

    // Immutable

    std::string m_name;
    ISocket::Ptr m_socket;

    // Mutable

    Vector3f m_pos;
    ZDOID m_characterID;

public:
    // Visible: 0, Admin: 1, Gated: 2
    BitPack<uint8_t, 1, 1, 1, 5> m_pack;

    UNORDERED_MAP_t<std::string, std::string, ankerl::unordered_dense::string_hash, std::equal_to<>> m_syncData;

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
    Peer(ISocket::Ptr socket);

    Peer(const Peer& other) = delete; // copy

    ~Peer() {
        //VLOG(1) << "~Peer()";
    }

    bool IsMapVisible() const {
        return m_pack.Get<VISIBLE_PACK_INDEX>();
    }

    bool IsAdmin() const {
        return m_pack.Get<ADMIN_PACK_INDEX>();
    }

    bool IsGated() const {
        return m_pack.Get<GATED_PACK_INDEX>();
    }

    void SetMapVisible(bool enable) {
        m_pack.Set<VISIBLE_PACK_INDEX>(enable);
    }

    void SetAdmin(bool enable) {
        m_pack.Set<ADMIN_PACK_INDEX>(enable);
    }

    void SetGated(bool enable) {
        m_pack.Set<GATED_PACK_INDEX>(enable);
    }

    /**
        * @brief Register a static method for remote invocation
        * @param name function name to register
        * @param lambda
    */
    template<typename F>
    void Register(HASH_t hash, F func) {
        //VLOG(1) << hash;
#if VH_IS_ON(VH_USE_MODS)
        m_methods[hash] = std::make_unique<MethodImpl<Peer*, F>>(func, IModManager::Events::RpcIn, hash);
#else
        m_methods[hash] = std::make_unique<MethodImpl<Peer*, F>>(func);
#endif
    }

    template<typename F>
    decltype(auto) Register(std::string_view name, F func) {
        return Register(VUtils::String::GetStableHashCode(name), func);
    }

#if VH_IS_ON(VH_USE_MODS)
    void RegisterLua(const IModManager::MethodSig& sig, const sol::function& func) {
        //VLOG(1) << sol::state_view(func.lua_state())["tostring"](func).get<std::string>() << ", hash: " << sig.m_hash;
        
        m_methods[sig.m_hash] = std::make_unique<MethodImplLua<Peer*>>(func, sig.m_types);
    }
#endif


    // TODO disable this method if full mod capture is enabled
    //  allowing this method is better for performance but limits mod catcheability
    template <typename Func>
    void SubInvoke(HASH_t hash, Func func) {
        if (!m_socket->Connected())
            return;

        BYTES_t bytes;
        DataWriter writer(bytes);

        writer.Write(hash);
        writer.SubWrite(func);

        // Prefix
        //if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::RpcOut ^ hash, this, bytes))
            //return;
        
        this->Send(std::move(bytes));

        // Postfix
        //VH_DISPATCH_MOD_EVENT(IModManager::Events::RpcOut ^ hash ^ IModManager::Events::POSTFIX, this, writer);
    }

    template <typename Func>
    void SubRoute(HASH_t hash, ZDOID targetZDO, Func func) {
        if (!m_socket->Connected())
            return;

        BYTES_t bytes;
        DataWriter writer(bytes);

        writer.Write(Hashes::Rpc::RoutedRPC);

        writer.SubWrite([&](DataWriter& writer) {
            // routed rpc spec
            writer.Write<int64_t>(0); // msg id
            writer.Write(VH_ID); // sender
            writer.Write(m_characterID.GetOwner()); // target
            writer.Write(targetZDO); // target ZDO
            writer.Write(hash); // routed method hash
            // FIrst subwrite the routedrpc parameter package then nest the params within it
            writer.SubWrite([func](DataWriter& writer) {
                writer.SubWrite(func); // explicit parameter as a package (length + array)
            });
        });

        // Prefix
        //if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::RouteOut ^ hash, this, targetZDO, bytes))
            //return;

        this->Send(std::move(bytes));

        // Postfix
        //VH_DISPATCH_MOD_EVENT(IModManager::Events::RpcOut ^ hash ^ IModManager::Events::POSTFIX, this, writer);
    }

    template <typename Func>
    void SubRoute(HASH_t hash, Func func) {
        SubRoute(hash, ZDOID::NONE, func);
    }



    template <typename... Types>
    void Invoke(HASH_t hash, const Types&... params) {
        if (!m_socket->Connected())
            return;

        // Prefix
        if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::RpcOut ^ hash, this, params...))
            return;

        //VLOG(2) << "Invoke, hash: " << hash << ", #params: " << sizeof...(params);

        this->Send(DataWriter::Serialize(hash, params...));

        // Postfix
        //VH_DISPATCH_MOD_EVENT(IModManager::Events::RpcOut ^ hash ^ IModManager::Events::POSTFIX, this, params...);
    }

    template <typename... Types>
    decltype(auto) Invoke(std::string_view name, const Types&... params) {
        return Invoke(VUtils::String::GetStableHashCode(name), params...);
    }



    //void InvokeLua(sol::state_view state, const IModManager::MethodSig& repr, const sol::variadic_args& args) {
#if VH_IS_ON(VH_USE_MODS)
    void InvokeLua(const IModManager::MethodSig& repr, const sol::variadic_args& args) {
        if (!m_socket->Connected())
            return;

        if (args.size() != repr.m_types.size())
            throw std::runtime_error("mismatched number of args");

        // Prefix
        //if (!VH_DISPATCH_MOD_EVENT(IModManager::EVENT_RpcOut ^ repr.m_hash, this, sol::as_args(args)))
        //    return;

        //VLOG(2) << "InvokeLua, hash: " << repr.m_hash << ", #params : " << args.size();

        BYTES_t bytes;
        DataWriter params(bytes);
        params.Write(repr.m_hash);
        params.SerializeLua(repr.m_types, sol::variadic_results(args.begin(), args.end()));
        this->Send(std::move(bytes));

        // Postfix
        //VH_DISPATCH_MOD_EVENT(IModManager::EVENT_RpcOut ^ repr.m_hash ^ IModManager::EVENT_POST, this, sol::as_args(args));
    }
#endif

    /*
    Method* GetMethod(HASH_t hash) {
        auto&& find = m_methods.find(hash);
        if (find != m_methods.end()) {
            return find->second.get();
        }
        return nullptr;
    }

    decltype(auto) GetMethod(const std::string& name) {
        return GetMethod(VUtils::String::GetStableHashCode(name));
    }*/



    bool InternalInvoke(HASH_t hash, DataReader &reader) {
        auto&& find = m_methods.find(hash);
        if (find != m_methods.end()) {
            ZoneScoped;
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

        if (VH_DISPATCH_MOD_EVENT(IModManager::Events::Send, this, std::ref(bytes)))
            this->m_socket->Send(std::move(bytes));
    }

    std::optional<BYTES_t> Recv() {
        if (auto&& opt = this->m_socket->Recv()) {
            auto&& bytes = *opt;
            if (VH_DISPATCH_MOD_EVENT(IModManager::Events::Recv, this, std::ref(bytes))) {
                return opt;
            }
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
        return Invoke(Hashes::Rpc::ConsoleMessage, msg);
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
        if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::RouteOut ^ hash, this, targetZDO, params...))
            return;

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


#if VH_IS_ON(VH_USE_MODS)
    void RouteViewLua(ZDOID targetZDO, const IModManager::MethodSig& repr, const sol::variadic_args& args) {
        if (args.size() != repr.m_types.size())
            throw std::runtime_error("mismatched number of args");

        auto results = sol::variadic_results(args.begin(), args.end());

#ifdef MOD_EVENT_RESPONSE
        if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::RouteOut ^ repr.m_hash, this, targetZDO, sol::as_args(results)))
            return;
#endif

        RouteParams(targetZDO, repr.m_hash, DataWriter::SerializeExtLua(repr.m_types, results));
    }

    decltype(auto) RouteLua(const IModManager::MethodSig& repr, const sol::variadic_args& args) {
        return RouteViewLua(ZDOID::NONE, repr, args);
    }
#endif

    /*
    friend std::ostream& operator<<(std::ostream& ost, const Peer& peer) {
        return ost << peer.m_name << " (" << peer.m_socket << ")";
    }*/
};
