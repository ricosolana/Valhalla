#include "ReplayManager.h"
#include "Avledet.h"
#include "CompileSettings.h"
#include "DataStream.h"
#include "NetSocket.h"
#include "Peer.h"
#include "Types.h"
#include "WorldManager.h"
#include <chrono>
#include <memory>
#include <mutex>
#include <quill/LogMacros.h>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

void avledet::replay::ReplayManager::init() {
    this->m_server_nanos = std::chrono::steady_clock::now().time_since_epoch();

    this->m_this_world_session_path = std::filesystem::path(AVL_REPLAY_PATH) 
        / WorldManager()->GetWorld()->m_name 
        /// std::to_string(nanos.count()) // time stamp here, so what format?
        / std::to_string(m_server_nanos.count()); //TODO place a server-session identifier here

    m_thread = std::jthread([&](std::stop_token token) {
        size_t chunkIndex = 0;

        //std::filesystem::create_directories(root);

        const auto fnSavePacketDigest = [&](XShare* xshare, decltype(XShare::m_swap_packets) const& swapped) {
            avledet::util::Writer writer;

            //writer.write((std::int32_t)swapped.size());
            for (auto const& pair : swapped) {
                auto const& ns = pair.first;
                auto const& packet = pair.second;
                
                writer.write(ns.count());
                writer.write(packet);
            }

            auto&& buf = writer.get_buf();
            xshare->m_x_zstd.compressChunk(buf.data(), buf.size());
        };

        // we notate which need dumping
        //  soft reference
        std::vector<std::pair<XShare*, decltype(XShare::m_swap_packets)>> m_swappies;

        //make it simple now
        //  every x KB, digest, and write to file
        while (!token.stop_requested()) {

            {// non-exclusive lock enter
                std::scoped_lock scoped(m_mux);

                for (auto&& kv : m_digest) {
                    const auto& host = kv.first;
                    auto&& xptr = kv.second;

                    // TODO make variable cfg-size
                    auto&& session = xptr.get();

                    auto nanos = Avledet()->Nanos();

                    if (session->m_x_tracked_size >= 256000
                        || session->m_x_tracked_flush + 60s < nanos) {
                        m_swappies.push_back({ session, std::move(session->m_swap_packets) });

                        session->m_x_tracked_flush = nanos;
                        session->m_x_tracked_size = 0;
                    }
                }
            }// non-exclusive lock exit

            for (auto&& pair : m_swappies) {
                fnSavePacketDigest(pair.first, pair.second);
            }
        }
    });
}

void avledet::replay::ReplayManager::prime_new_capture(ISocket::Ptr socket) {
    if (AVL_SETTINGS.TEST_replayEnabled) {
        // record peer joindata
        const auto host = socket->get_host_name();

        auto nanos = Avledet()->Nanos();

        const auto path = m_this_world_session_path
            / "hosts"
            / host
            / std::to_string(nanos.count());

        auto xshare = std::make_unique<XShare>(path, nanos);

        {
            std::scoped_lock scoped(m_mux);
            m_digest[socket] = std::move(xshare);
        }

        //LOG(WARNING) << "Starting capture for " << peer->m_socket->GetHostName();
    }
}

void avledet::replay::ReplayManager::on_packet(ISocket::Ptr socket, avledet::util::Bytes packet) {
    // TODO lock
    
    //  there is an issue with the locking

    //  one supplier thread, and one consumer thread
    //  each are sharing resources
    //  the supplier accesses A LOT
    //  the consumer only needs to consume every other moment

    // so how to prevent so many shares
    
    // ??? does mutex acquire cause an expensive pausing?
    //  what is the cost when done frequently?

    // ultimately,
    //  !!! HOW, to perform cheap swapping, across atomic contexes?
    
    //m_digest[socket]->m_swap_packets

}
