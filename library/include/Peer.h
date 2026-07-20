#pragma once

#include <memory>
#include <sol/as_args.hpp>
#include <sol/forward.hpp>
#include <sol/variadic_args.hpp>
#include <string_view>
#include <tracy/Tracy.hpp>
#include <tuple>

#include "DataStream.h"
#include "Hashes.h"
#include "NetSocket.h"
#include "Replay.h"
#include "Rpc.h"
#include "Types.h"
#include "UserData.h"
#include "Vector.h"
#include "VUtils.h"
#include "VUtilsTraits.h"
#include "ZDO.h"//TODO might not need this class...

namespace avledet::replay {

}

enum class ChatMsgType : std::int32_t
{
    Whisper,
    Normal,
    Shout,
    Ping
};

enum class ConnectionStatus : std::int32_t
{
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
    MAX// 13
};

class Peer : public std::enable_shared_from_this<Peer>,
             public avledet::rpc::RpcBase<std::shared_ptr<Peer>>
// TODO replace with shared_ptr later...
{
    friend class IZDOManager;
    friend class INetManager;
    friend class IScriptManager;

    constexpr static int ADMIN_PACK_INDEX   = 0;
    constexpr static int VISIBLE_PACK_INDEX = 1;
    constexpr static int GATED_PACK_INDEX   = 2;

  private:
    std::chrono::steady_clock::time_point m_lastPing;

  public:
    using Ptr = std::shared_ptr<Peer>;

    // Elements are never removed
    //  Queried frequently, and frequent adds
    // TODO use zdo as key itself
    avledet::util::Map<ZDOID, std::pair<ZDO::Rev, float>> m_zdos;
    avledet::util::Set<ZDOID> m_forceSend;    // TODO this is rarely ever used (only for portal)
    avledet::util::Set<ZDOID> m_invalidSector;// TODO this is also odd

    // Immutable

    std::string m_name;
    ISocket::Ptr m_socket;

    // Mutable

    Vector3f m_pos;
    ZDOID m_characterID;

    // Admin: 0, Visible: 1, Gated: 2
    BitPack<std::uint8_t, 1, 1, 1, 5> m_pack;

    avledet::util::Map<std::string, std::string, ankerl::unordered_dense::string_hash, std::equal_to<>>
            m_syncData;

    avledet::replay::XShare::Ptr m_replay_share;

  private:
    void update();

    void ZDOSectorInvalidated(ZDO::reference zdo);

    void ForceSendZDO(ZDOID const &id)
    {
        m_forceSend.insert(id);
    }

    bool IsOutdatedZDO(ZDO::reference zdo, decltype(m_zdos)::iterator &outItr);

    bool IsOutdatedZDO(ZDO::reference zdo)
    {
        decltype(m_zdos)::iterator outItr;
        return IsOutdatedZDO(zdo, outItr);
    }

  public:
    Peer(ISocket::Ptr socket);

    Peer(Peer const &other) = delete;// copy

    //~Peer()
    //{
    //    //VLOG(1) << "~Peer()";
    //}

    avledet::util::UserID GetUserID()
    {
        return m_characterID.get_user_id();
    }

    bool IsAdmin() const;
    bool IsMapVisible() const;
    bool IsGated() const;

    void SetAdmin(bool enable);
    void SetMapVisible(bool enable);
    void SetGated(bool enable);

    /**
        * @brief Register a static method for remote invocation
        * @param name function name to register
        * @param lambda
    */
    template<typename F>
    void Register(avledet::util::Hash hash, std::string_view dbg_desc, F func)
    {
        static_assert(
                std::is_same_v<std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>,
                               Ptr>,
                "Rpc must accept a shared_ptr<Peer> as first argument");
#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
        register_method(
                std::make_unique<MethodImpl<Ptr, F>>(hash, dbg_desc, IScriptManager::Events::RpcIn, std::move(func)));
#else
        register_method(hash, dbg_desc, std::move(func));
        //register_method(std::make_unique<MethodImpl<Peer::Ptr, F>>(hash, std::move(func)));
#endif
    }

    template<typename F>
    void Register(std::string_view name, F func)
    {
        Register(avledet::util::get_stable_hash(name), name, func);
    }

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    void RegisterLua(IScriptManager::MethodSig const &sig, sol::protected_function const &func)
    {
        //VLOG(1) << sol::state_view(func.lua_state())["tostring"](func).get<std::string>() << ", hash: " << sig.m_hash;
        register_method(std::make_unique<MethodImplLua<Ptr>>(sig.m_hash, sig.m_dbg_desc, func, sig.m_env, sig.m_types));

        //m_methods[sig.m_hash] = std::make_unique<MethodImplLua<Peer::Ptr>>(func, sig.m_types);
    }
#endif


    // TODO disable this method if full mod capture is enabled
    //  allowing this method is better for performance but limits mod catcheability
    /*
    template<typename Func>
    void SubInvoke(avledet::util::Hash hash, Func func)
    {
        if (m_socket->get_status() == Status::Closed)
            return;

        DataWriter writer;

        writer.write(hash);
        //assert(false); // ADDRESS THE BELOW
        writer.write(func);

        // TODO Prefix
        //if (!AVL_SCRIPT_EVENT(IScriptManager::Events::RpcOut ^ hash, this->shared_from_this(), bytes))
        //return;

        this->Send(writer.release());

        // TODO Postfix
        //AVL_SCRIPT_EVENT(IScriptManager::Events::RpcOut ^ hash ^ IScriptManager::Events::POSTFIX, this->shared_from_this(), writer);
    }*/

    // TODO
    //  make a call to routed rpc direct- instead
    //  or visa-versa funnel
    ////template<typename Func>
    ////void SubRoute(avledet::util::Hash hash, avledet::util::UserID const &sender, ZDOID const &targetZDO,
    ////              Func func)
    ////{
    ////    if (m_socket->get_status() == Status::Closed)
    ////        return;

    ////    DataWriter writer;
    ////    writer.write(avledet::util::hashes::Rpc::RoutedRPC);

    ////    {
    ////        avledet::util::WriterScopedEncap scoped(writer);

    ////        // routed rpc spec
    ////        writer.write<std::int64_t>(0);// msg id
    ////        //writer.write(AVL_ID);                     // sender
    ////        writer.write(sender);
    ////        writer.write(m_characterID.get_user_id());// target
    ////        writer.write(targetZDO);                  // target ZDO
    ////        writer.write(hash);                       // routed method hash
    ////        // First subwrite the routedrpc parameter package then nest the params within it
    ////        //assert(false); //ADDRESS THE BELOW
    ////        {
    ////            avledet::util::WriterScopedEncap scoped1(writer);


    ////        }
    ////        writer.write([func](DataWriter &writer) {
    ////            writer.write(func);// explicit parameter as a package (length + array)
    ////        });
    ////    }
    ////    writer.write([this, hash, sender, targetZDO, func](DataWriter &writer) {
    ////        // routed rpc spec
    ////        writer.write<std::int64_t>(0);// msg id
    ////        //writer.write(AVL_ID);                     // sender
    ////        writer.write(sender);
    ////        writer.write(m_characterID.get_user_id());// target
    ////        writer.write(targetZDO);                  // target ZDO
    ////        writer.write(hash);                       // routed method hash
    ////        // FIrst subwrite the routedrpc parameter package then nest the params within it
    ////        //assert(false); //ADDRESS THE BELOW
    ////        writer.write([func](DataWriter &writer) {
    ////            writer.write(func);// explicit parameter as a package (length + array)
    ////        });
    ////    });

    ////    // Prefix
    ////    //if (!AVL_SCRIPT_EVENT(IScriptManager::Events::RouteOut ^ hash, this->shared_from_this(), targetZDO, bytes))
    ////    //return;

    ////    this->Send(writer.release());

    ////    // Postfix
    ////    //AVL_SCRIPT_EVENT(IScriptManager::Events::RpcOut ^ hash ^ IScriptManager::Events::POSTFIX, this->shared_from_this(), writer);
    ////}

    //template<typename Func>
    //void SubRoute(avledet::util::Hash hash, avledet::util::UserID const &sender, Func func)
    //{
    //    SubRoute(hash, sender, ZDOID::NONE, func);
    //}

    //template<typename Func>
    //void SubRoute(avledet::util::Hash hash, Func func)
    //{
    //    SubRoute(hash, ZDOID::NONE, func);
    //}

    template<typename... Types>
    void Invoke(avledet::util::Hash hash, Types const &...params)
    {
        if (m_socket->get_status() == Status::Closed)
            return;

        // Prefix
        if (!AVL_SCRIPT_EVENT(IScriptManager::Events::RpcOut ^ hash, this->shared_from_this(), params...))
            return;

        //VLOG(2) << "Invoke, hash: " << hash << ", #params: " << sizeof...(params);

        this->Send(DataWriter::serialize(hash, params...));

        // Postfix
        //AVL_SCRIPT_EVENT(IScriptManager::Events::RpcOut ^ hash ^ IScriptManager::Events::POSTFIX, this->shared_from_this(), params...);
    }

    template<typename... Types>
    decltype(auto) Invoke(std::string_view name, Types const &...params)
    {
        return Invoke(avledet::util::get_stable_hash(name), params...);
    }

    //void InvokeLua(sol::state_view state, const IScriptManager::MethodSig& repr, const sol::variadic_args& args) {
#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    void InvokeLua(IScriptManager::MethodSig const &repr, sol::variadic_args const &args)
    {
        if (m_socket->get_status() == Status::Closed)
            return;

        if (args.size() != repr.m_types.size())
            throw std::runtime_error("mismatched number of args");

        DataWriter params;
        params.write(repr.m_hash);
        params.write(repr.m_types, args);
        this->Send(params.release());
    }
#endif

    /*
    Method* GetMethod(avledet::util::Hash hash) {
        auto&& find = m_methods.find(hash);
        if (find != m_methods.end()) {
            return find->second.get();
        }
        return nullptr;
    }

    decltype(auto) GetMethod(const std::string& name) {
        return GetMethod(avledet::util::get_stable_hash(name));
    }*/


    void InternalInvoke(avledet::util::Hash hash, DataReader &reader);

    void InternalInvoke(std::string_view name, DataReader &reader)
    {
        return InternalInvoke(avledet::util::get_stable_hash(name), reader);
    }

    void Send(avledet::util::Bytes bytes)
    {
        assert(!bytes.empty());

        if (AVL_SCRIPT_EVENT(IScriptManager::Events::Send, this->shared_from_this(), bytes))
            this->m_socket->send(std::move(bytes));
    }

    std::optional<avledet::util::Bytes> Recv()
    {
        auto bytes = m_socket->Recv();
        if (!bytes.empty()) {
            if (AVL_SCRIPT_EVENT(IScriptManager::Events::Recv, this->shared_from_this(), bytes)) {
                return bytes;
            }
        }
        return std::nullopt;
    }

    void Disconnect()
    {
        m_socket->close(true);
    }

    void SendDisconnect()
    {
        Invoke(avledet::util::hashes::Rpc::Disconnect);
    }

    void SendKicked()
    {
        Invoke(avledet::util::hashes::Rpc::S2C_ResponseKicked);
    }

    void Kick()
    {
        SendKicked();
        Disconnect();
    }

    bool close(ConnectionStatus status);


    // Higher utility functions once authenticated


    ZDO::optional find_zdo();

    void Teleport(Vector3f pos, Quaternion rot, bool animation);

    void Teleport(Vector3f pos)
    {
        Teleport(pos, Quaternion::IDENTITY, false);
    }

    // Show a specific chat message
    void ChatMessage(std::string_view msg, ChatMsgType type, Vector3f pos, UserProfile const &profile,
                     std::string_view senderID)
    {
        this->Route(avledet::util::hashes::Routed::ChatMessage, pos, type, profile, msg, senderID);
    }

    // Show a chat message (string, string_view, tuple<Strings...>
    void ChatMessage(std::string_view msg)
    {
        this->Route(avledet::util::hashes::Routed::ChatMessage, Vector3f(10000, 10000, 10000),
                    ChatMsgType::Normal, std::string_view(""),
                    std::string_view("<color=yellow><b>SERVER</b></color>"), std::string_view(""), msg,
                    std::string_view(""));
    }

    // Show a console message
    //  AKA RemotePrint
    void ConsoleMessage(std::string_view msg)
    {
        return Invoke(avledet::util::hashes::Rpc::ConsoleMessage, msg);
    }

  private:
    // Show a screen message
    void UIMessage(std::string_view msg, UIMsgType type)
    {
        this->Route(avledet::util::hashes::Routed::S2C_UIMessage, type, msg);
    }

  public:
    // Show a corner screen message
    void CornerMessage(std::string_view msg)
    {
        return UIMessage(msg, UIMsgType::TopLeft);
    }

    // Show a center screen message
    void CenterMessage(std::string_view msg)
    {
        return UIMessage(msg, UIMsgType::Center);
    }

    void RouteParams(avledet::util::UserID const &sender, ZDOID targetZDO, avledet::util::Hash hash,
                     avledet::util::Bytes params);

    void RouteParams(ZDOID targetZDO, avledet::util::Hash hash, avledet::util::Bytes params);

    template<typename... Types>
    void RouteView(ZDOID targetZDO, avledet::util::Hash hash, Types const &...params)
    {
        if (!AVL_SCRIPT_EVENT(IScriptManager::Events::RouteOut ^ hash, this->shared_from_this(), targetZDO,
                              params...))
            return;

        RouteParams(targetZDO, hash, DataWriter::serialize(params...));
    }

    template<typename... Types>
    void RouteView(ZDOID targetZDO, std::string_view name, Types const &...params)
    {
        RouteView(targetZDO, avledet::util::get_stable_hash(name), params...);
    }

    template<typename... Types>
    void Route(avledet::util::Hash hash, Types const &...params)
    {
        RouteView(ZDOID::NONE, hash, params...);
    }

    template<typename... Types>
    void Route(std::string_view name, Types const &...params)
    {
        RouteView(ZDOID::NONE, avledet::util::get_stable_hash(name), params...);
    }


#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    void RouteViewLua(ZDOID targetZDO, IScriptManager::MethodSig const &repr, sol::variadic_args const &args)
    {
        if (args.size() != repr.m_types.size())
            throw std::runtime_error("mismatched number of args");

        DataWriter writer;
        writer.write(repr.m_types, args);
        RouteParams(targetZDO, repr.m_hash, writer.release());
    }

    decltype(auto) RouteLua(IScriptManager::MethodSig const &repr, sol::variadic_args const &args)
    {
        return RouteViewLua(ZDOID::NONE, repr, args);
    }
#endif

    /*
    friend std::ostream& operator<<(std::ostream& ost, const Peer& peer) {
        return ost << peer->m_name << " (" << peer->m_socket << ")";
    }*/

    friend std::ostream &operator<<(std::ostream &ost, Peer const &value)
    {
        return ost << "{ " << value.m_name << " | " << value.m_socket->get_host_name() << " }";
    }
};
