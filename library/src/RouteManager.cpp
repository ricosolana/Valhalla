#include "RouteManager.h"
#include "Avledet.h"
#include "DataStream.h"
#include "DiscordManager.h"
#include "Hashes.h"
#include "Method.h"
#include "NetManager.h"
#include "Peer.h"
#include "Types.h"
#include "ZDOManager.h"
#include "ZoneManager.h"
#include <cstdint>
#include <limits>
#include <ratio>

auto ROUTE_MANAGER = std::make_unique<IRouteManager>();// TODO stop constructing in global

IRouteManager *RouteManager()
{
    return ROUTE_MANAGER.get();
}

// throttle different packets differently
// throttling inputs:
//	- method hash
//	- if targetZDO (netview aimed), then prefab hash extracted from getzdo(zdoid).prefab
//	- burst interval
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <vector>


#include <chrono>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <vector>

//	- rate throttling
//


void IRouteManager::OnNewPeer(Peer::Ptr peer)
{
    peer->Register(avledet::util::hashes::Rpc::RoutedRPC, [this](Peer::Ptr peer, DataReader reader) {
        if (peer->IsGated())
            return;

        reader.read<std::int64_t>();// skip msgid
        /*DataWriter(avledet::util::ByteView(reader.data(), reader.size()), reader.get_pos()).Write(peer->m_uuid);*/
        reader.read<avledet::util::UserID>();// skip sender
        auto target    = reader.read<avledet::util::UserID>();
        auto targetZDO = reader.read<ZDOID>();
        auto hash      = reader.read<avledet::util::Hash>();
        auto params    = DataReader(reader.read<std::vector<char>>());

        /*
		* Rpc and multi-execution dilemna
		*	Should I let multiple handlers be able to call RoutedRpc methods?
		*	What about packets coming in that do not refer to any currently register Rpc?
		*		Should they still dispatch the Lua handlers?
		*	Similarly, what about missing RoutedRpc handlers?
		*/

        if (target == EVERYBODY) {
            // Confirmed: targetZDO CAN have a value when globally routed
            //  TODO make params a COPY here for lua, or define copy-like behaviour / modifs...
            //if (!AVL_SCRIPT_EVENT(IScriptManager::Events::RouteInAll ^ hash, peer, targetZDO, params))
            //    return;

            //dpp death trigger webhook
            //TODO
            //	can obviously be spammed by bad actors, but their name is shown, so... self inflicted
            // TODO test this
            //if (hash == avledet::util::get_stable_hash("OnDeath")) {
            //    AVL_DISPATCH_WEBHOOK(peer->m_name + " has died");
            //}

            // 'EVERYBODY' also targets the server
            if (!targetZDO) {
                auto params_copy = params;
                this->internal_invoke(peer, hash, params_copy);
            }//else ... // netview currently not supported

            auto &&peers = NetManager()->GetPeers();
            for (auto &&other : peers) {
                // Ignore the src peer
                if (peer->GetUserID() != other->GetUserID()) {
                    other->Invoke(avledet::util::hashes::Rpc::RoutedRPC, (std::int64_t) 0, peer->GetUserID(),
                                  target, targetZDO, hash, params);// params (everything really...) is copied
                }
            }
        } else {
            if (target != AVL_ID) {
                if (auto other = NetManager()->FindPeerByUserID(target)) {
                    // TODO test if working correctly
                    //if (!AVL_SCRIPT_EVENT(IScriptManager::Events::Routed ^ hash, peer, reader))
                    //    return;

                    //other->Invoke(avledet::util::hashes::Rpc::RoutedRPC, reader);
                    other->Invoke(avledet::util::hashes::Rpc::RoutedRPC, (std::int64_t) 0, peer->GetUserID(),
                                  target, targetZDO, hash, params);
                }
            } else {
                if (!targetZDO) {
                    //TODO we dont need to copy params here, but portability is better...
                    auto params_copy = params;
                    this->internal_invoke(peer, hash, params_copy);
                }//else ... // netview is not currently supported
            }
        }
    });
}
