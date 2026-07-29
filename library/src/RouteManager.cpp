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


void RouteManager::OnNewPeer(Peer::Ptr peer)
{
    peer->Register("RoutedRPC", [this](Peer::Ptr peer, DataReader reader) {
        if (peer->IsGated())
            return;

        reader.read<std::int64_t>();// skip msgid
        /*DataWriter(avledet::util::ByteView(reader.data(), reader.size()), reader.get_pos()).Write(peer->m_uuid);*/
        reader.read<avledet::util::UserID>();// skip sender
        auto sender    = peer->GetUserID();
        auto target    = reader.read<avledet::util::UserID>();
        auto targetZDO = reader.read<ZDOID>();
        auto hash      = reader.read<avledet::util::Hash>();
        auto params    = reader.read<std::vector<char>>();

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
            if (!AVL_SCRIPT_EVENT(ScriptManager::Events::RouteInAll ^ hash, peer, targetZDO, params)) {
                return;
            }

            //dpp death trigger webhook
            //TODO
            //	can obviously be spammed by bad actors, but their name is shown, so... self inflicted
            // TODO test this
            //if (hash == avledet::util::get_stable_hash("OnDeath")) {
            //    AVL_DISPATCH_WEBHOOK(peer->m_name + " has died");
            //}

            // 'EVERYBODY' also targets the server
            if (!targetZDO) {
                auto reader0 = avledet::util::Reader(params);
                this->internal_invoke(peer, hash, reader0);
            }//else ... // netview currently not supported

            auto &&peers = NetManager::instance().GetPeers();
            if (!peers.empty()) {
                DataWriter writer;

                writer.write(avledet::util::hashes::Rpc::RoutedRPC);
                {
                    avledet::util::WriterScopedEncap scoped(writer);

                    RouteManager::instance().prepare_packet(writer, sender, target, targetZDO, hash);

                    writer.write(params);
                }

                for (auto &&other : peers) {
                    // Ignore the src peer
                    if (peer->GetUserID() != other->GetUserID()) {
                        // we do the lowest level send to avoid rpc lua
                        other->Send(writer.get_buf());
                    }
                }
            }
        } else {
            if (target != AVL_ID) {
                if (auto other = NetManager::instance().FindPeerByUserID(target)) {
                    // TODO test if working correctly
                    if (!AVL_SCRIPT_EVENT(ScriptManager::Events::Routed ^ hash, peer, other, targetZDO,
                                          params)) {
                        return;
                    }

                    DataWriter writer;

                    writer.write(avledet::util::hashes::Rpc::RoutedRPC);
                    {
                        avledet::util::WriterScopedEncap scoped(writer);

                        RouteManager::instance().prepare_packet(writer, sender, target, targetZDO, hash);

                        writer.write(params);
                    }

                    // we do the lowest level send to avoid rpc lua
                    other->Send(writer.get_buf());
                }
            } else {
                if (!targetZDO) {
                    //TODO we dont need to copy params here, but portability is better...
                    auto reader0 = avledet::util::Reader(params);
                    this->internal_invoke(peer, hash, reader0);
                }//else ... // netview is not currently supported
            }
        }
    });
}
