#pragma once

#include "Avledet.h"
#include "DataStream.h"
#include "Hashes.h"
#include "Method.h"
#include "ModManager.h"
#include "NetManager.h"
#include "Peer.h"
#include <tuple>

class IRouteManager : public avledet::rpc::RpcBase<Peer::Ptr>
{
    friend class INetManager;

  public:
    static constexpr std::int64_t EVERYBODY = 0;

  private:
    // Called from NetManager
    void OnNewPeer(Peer::Ptr peer);

  public:
    /**
		* @brief Register a static method for routed remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
    template<typename F>
    void Register(avledet::util::Hash hash, F func)
    {
#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
        register_method(
                std::make_unique<MethodImpl<Peer::Ptr, F>>(hash, IScriptManager::Events::RouteIn, func));

        //m_methods[hash]
        //        = std::make_unique<MethodImpl<Peer::Ptr, F>>(func, IScriptManager::Events::RouteIn, hash);
#else
        register_method(std::make_unique<MethodImpl<Peer::Ptr, F>>(hash, func));

        //m_methods[hash] = std::make_unique<MethodImpl<Peer::Ptr, F>>(hash, func);
#endif
    }

    template<typename F>
    decltype(auto) Register(std::string_view name, F func)
    {
        return Register(avledet::util::get_stable_hash(name), func);
    }

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    void RegisterLua(IScriptManager::MethodSig const &sig, sol::function const &func)
    {
        //VLOG(1) << "RegisterLua, func: " << sol::state_view(func.lua_state())["tostring"](func).get<std::string>() << ", hash: " << sig.m_hash;

        //m_methods[sig.m_hash] = std::make_unique<MethodImplLua<Peer::Ptr>>(func, sig.m_types);
        register_method(std::make_unique<MethodImplLua<Peer::Ptr>>(sig.m_hash, func, sig.m_types));
    }
#endif

    // Forwards raw data to peer(s) with no Lua handlers
    //void InvokeParams(avledet::util::UserID target, const ZDOID& targetZDO, avledet::util::Hash hash, avledet::util::Bytes params);


    // Invoke a routed function bound to a peer with sub zdo
    template<typename... Args>
    void InvokeView(avledet::util::UserID target, ZDOID const &targetZDO, avledet::util::Hash hash,
                    Args const &...params)
    {
        // Prefix
        if (target == EVERYBODY) {
            // If the script wants to modify the arguments, it will be allowed, ...
            //  BUT, it will have to subsequently make a new call to Invoke(Lua)
            //  Its just safer this way..
            if (!AVL_SCRIPT_EVENT(IScriptManager::Events::RouteOutAll ^ hash, targetZDO, params...))
                return;

            avledet::util::Writer writer;
            writer.write(avledet::util::hashes::Rpc::RoutedRPC);
            {
                avledet::util::WriterScopedEncap scoped(writer);

                /*writer =*/prepare_packet(writer, AVL_ID, (std::int64_t) target, targetZDO, hash);
                {
                    avledet::util::WriterScopedEncap scoped2(writer);

                    writer.write_all(params...);
                }
            }

            for (auto &&peer : NetManager()->GetPeers()) {
                peer->Send(writer.get_buf());
            }
        } else {
            if (auto peer = NetManager()->FindPeerByUserID(target)) {
                peer->RouteView(targetZDO, hash, params...);
            }
        }
    }

    // Invoke a routed function bound to a peer with sub zdo
    template<typename... Args>
    void InvokeView(avledet::util::UserID target, ZDOID const &targetZDO, std::string_view name,
                    Args const &...params)
    {
        InvokeView(target, targetZDO, avledet::util::get_stable_hash(name), params...);
    }

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    void InvokeViewLua(Int64Wrapper target, ZDOID const &targetZDO, IScriptManager::MethodSig const &repr,
                       sol::variadic_args const &args)
    {
        if ((std::int64_t) target == EVERYBODY) {
            if (args.size() != repr.m_types.size())
                throw std::runtime_error("mismatched number of args");

            // RoutedRPC packet Shape
            //  Valheim:
            //      int32: <hash>
            //  Rpc (args)
            //  RoutedRpc (package)
            //      uint32: length
            //          int64: msg_id
            //          int64: sender
            //          int64: target
            //          int64+int32: zdo
            //          int32: hash
            //              uint32: length
            //              <args>

            avledet::util::Writer writer;
            writer.write(avledet::util::hashes::Rpc::RoutedRPC);
            {
                avledet::util::WriterScopedEncap scoped(writer);

                /*writer =*/prepare_packet(writer, AVL_ID, (std::int64_t) target, targetZDO, repr.m_hash);
                {
                    avledet::util::WriterScopedEncap scoped2(writer);

                    auto results = sol::variadic_results(args.begin(), args.end());
                    writer.write(repr.m_types, results);
                }
            }

            for (auto &&peer : NetManager()->GetPeers()) {
                peer->Send(writer.get_buf());
            }
        } else {
            if (auto peer = NetManager()->FindPeerByUserID((std::int64_t) target)) {
                peer->RouteViewLua(targetZDO, repr, args);
            }
        }

        //Serialize(AVL_ID, target, targetZDO, repr.m_hash,
        //DataWriter::serializeLua(repr.m_types, sol::variadic_results(args.begin(), args.end())));

        //Invoke(target, targetZDO, repr.m_hash, DataWriter::serializeLua(repr.m_types, sol::variadic_results(args.begin(), args.end())));
    }
#endif


    // Invoke a routed function bound to a peer
    template<typename... Args>
    void Invoke(avledet::util::UserID target, avledet::util::Hash hash, Args const &...params)
    {
        InvokeView(target, ZDOID::NONE, hash, params...);
    }

    // Invoke a routed function bound to a peer
    template<typename... Args>
    void Invoke(avledet::util::UserID target, std::string_view name, Args const &...params)
    {
        InvokeView(target, ZDOID::NONE, avledet::util::get_stable_hash(name), params...);
    }

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    void InvokeLua(Int64Wrapper target, IScriptManager::MethodSig const &repr, sol::variadic_args const &args)
    {
        InvokeViewLua(target, ZDOID::NONE, repr, args);
    }
#endif


    // Invoke a routed function targeted to all peers
    template<typename... Args>
    void InvokeAll(avledet::util::Hash hash, Args const &...params)
    {
        Invoke(EVERYBODY, hash, params...);
    }

    // Invoke a routed function targeted to all peers
    template<typename... Args>
    void InvokeAll(std::string_view name, Args const &...params)
    {
        Invoke(EVERYBODY, avledet::util::get_stable_hash(name), params...);
    }

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    void InvokeAllLua(IScriptManager::MethodSig const &repr, sol::variadic_args const &args)
    {
        InvokeLua(EVERYBODY, repr, args);
    }
#endif

    // TODO [deprecated]
    //  Instead, create a preparer(), and write to that directly (faster)
    ////[[deprecated("use prepare_params instead")]]
    ////avledet::util::Bytes Serialize(avledet::util::UserID sender, avledet::util::UserID target,
    ////                               ZDOID const &targetZDO, avledet::util::Hash hash,
    ////                               avledet::util::Bytes const &params)
    ////{
    ////    DataWriter writer;

    ////    writer.write((std::int64_t) 0);// msg id
    ////    writer.write(sender);
    ////    writer.write(target);
    ////    writer.write(targetZDO);
    ////    writer.write(hash);
    ////    writer.write(params);

    ////    return writer.release();
    ////}

    void /*avledet::util::Writer*/ prepare_packet(avledet::util::Writer &writer, avledet::util::UserID sender,
                                                  avledet::util::UserID target, ZDOID const &targetZDO,
                                                  avledet::util::Hash hash)
    {
        //avledet::util::Writer writer;
        //writer.write(avledet::util::hashes::Rpc::RoutedRPC);

        {
            // Wrong. Must encap entire packet, not just RoutedRpc header
            //avledet::util::WriterScopedEncap scoped(writer);

            writer.write((std::int64_t) 0);// msg id (dummy)
            writer.write(sender);
            writer.write(target);
            writer.write(targetZDO);
            writer.write(hash);
        }

        // value *should* be returned without copy
        //return writer;
    }
};

// Manager class for everything related to high-level networking for simulated p2p communication
IRouteManager *RouteManager();
