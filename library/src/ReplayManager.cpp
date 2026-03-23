#include "ReplayManager.h"
#include "Avledet.h"
#include "CompileSettings.h"
#include "DataStream.h"
#include "NetSocket.h"
#include "Peer.h"
#include "Replay.h"
#include "ServerSettings.h"
#include "Task.h"
#include "Types.h"
#include "VUtils.h"
#include "VUtilsRandom.h"
#include "WorldManager.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <emmintrin.h>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <quill/LogMacros.h>
#include <shared_mutex>
#include <stop_token>
#include <string>
#include <utility>
#include <vector>

namespace avledet::replay {

    auto REPLAY_MANAGER = std::make_unique<IReplayManager>();

    IReplayManager *ReplayManager() {
        return REPLAY_MANAGER.get();
    }

    XShare::XShare(std::filesystem::path path) 
        : m_zstream(path)
        //: m_time_begin(time_begin)
    {
        // TODO finish...
        //assert(false);

        //m_active.store(&m_buf1, std::memory_order_relaxed);
        m_active = &m_buf1;
    }

    XShare::~XShare() {
        if (true) {
            volatile int x = 0;
        }
    }

    //void XShare::on_packet(avledet::util::Bytes packet) {
    //    SwapBuffer* buf = m_active_buf.load(std::memory_order_relaxed);
//
    //    auto nanos = Avledet()->Nanos();
//
    //    this->m_bytes_size += packet.size();
    //    buf->push_back({ nanos, std::move(packet) });
    //}

    void IReplayManager::init() {
        this->m_server_nanos = std::chrono::steady_clock::now().time_since_epoch();

        this->m_this_world_session_path = std::filesystem::path(AVL_REPLAY_PATH) 
            / WorldManager()->GetWorld()->m_name 
            /// std::to_string(nanos.count()) // time stamp here, so what format?
            /// avledet::util::GenerateAlphaNum(6)
            / std::to_string(m_server_nanos.count()); //TODO place a server-session identifier here

        std::filesystem::create_directories(this->m_this_world_session_path);

        // TODO load paths, NOT sessions YET
        //  storing session paths is very cheap
        //auto hosts_dir_itr = std::vector(std::filesystem::directory_iterator{this->m_this_world_session_path / "hosts"}, std::filesystem::directory_iterator{});
        //for (auto host_itr : hosts_dir_itr) {
        //    //host_itr.path()
        //    auto hostname = host_itr.path().stem().string();
        //    
        //    m_peer_playback_sessions[hostname].push_back()
        //}

        m_thread = std::jthread([&](std::stop_token token) {
            if (AVL_SETTINGS.m_replay_mode == ReplayMode::CAPTURE) {
                this->thread_capture_job(token);
            } else if (AVL_SETTINGS.m_replay_mode == ReplayMode::PLAYER) {
                // player
                this->thread_player_job(token);
            } else {
                assert(false); // unknown mode
            }
        });

        LOG_NOTICE(AVL_LOGGER, "Experimental packet replay mode enabled");
    }

    void IReplayManager::uninit() {
        // TODO: stop the thread
        //  finish all peers
        m_thread.request_stop();
        m_thread.join();

        // write 
        //  well.. we used a directory iterator on later read
        //  
    }

    void IReplayManager::update() {
        // have queued sessions ready
    }

    void IReplayManager::emit_to_stream(XShare& share, XShare::SwapBuffer const& buf) {
        avledet::util::Writer writer;

        for (auto const& pair : buf) {
            writer.write(pair.first.count());
            writer.write(pair.second);
        }

        auto&& writer_buf = writer.get_buf();
        share.m_zstream.compressChunk(writer_buf.data(), writer_buf.size());
    }

    void IReplayManager::thread_capture_job(std::stop_token token) {
        bool run_one_more = false;
        //while (!token.stop_requested())
        for (;;)
        {
            if (run_one_more) {
                break;
            }

            if (token.stop_requested()) {
                run_one_more = true;
            }

            XShare::Ptr next =
                m_ready_head.exchange(nullptr, std::memory_order_acquire);

            auto *list = next.get();

            while (list)
            {
                auto* buf   = list->m_inactive;

                try {
                    if (!list->m_zstream.streaming_ready()) {
                        // open it
                        // TODO: this REALLY should be done once,
                        //  in a different sort of work queue
                        list->m_zstream.init();
                        list->m_zstream.open_file();
                    }

                    emit_to_stream(*list, *buf);

                    buf->clear();

                    // One-off, if peer is closing, write final data, then close stream/file
                    if (list->m_closing.load(std::memory_order_acquire))
                    {
                        auto* active = list->m_active;

                        emit_to_stream(*list, *active);
                        active->clear();

                        list->m_zstream.finish();
                    } else {
                        // mark buffer as free
                        list->m_in_flight.store(false, std::memory_order_release);
                    }
                } catch (std::exception const& ex) {
                    // possible errors off the top of my head:
                    //  - zstd failure (cant create, cant param, cant dict)
                    //  - file open failure
                    //  - file write failure
                    //  - zstd compress failure
                    
                    // we skipped the "mark buffer as free"
                    //  because: the writer is now indeterminate
                    //  and it is pointless/unsafe to release it back

                    // TODO
                    //  this must be fixed
                    LOG_ERROR(AVL_LOGGER, "unhandled error: {}", ex.what());

                    // TODO
                    //  what do do during what error?
                    //  should we disconnect peer?
                    //  should we create a new stream?
                    //      - require reset / realloc
                    //  likely, a server setting to determine the fail action
                    //  - options:
                    //      - lock the stream permanently by panicking the single -per-stream
                    //          - peer remains online
                    //      - disconnect the peer
                    //          - able to recover dead stream
                    //          - however, proper handling must occur for the subsequent flush
                    //              - this should be handled, because a "panic" keeps the
                    //                  write flag active, preventing subsequent stream finish
                    //      - attempt to retry a replay
                    //          - most dangerous part
                    //          - would be better to retry via forcing player disposal (kick)
                }

                next = std::move(list->next);
                list = next.get();
            }

            //_mm_pause();

            //bool expected = m_wakeup.load(std::memory_order_acquire);
            bool expected = false;
            m_wakeup.wait(expected, std::memory_order_relaxed); // blocks while false

            //if (m_ready_head.load(std::memory_order_acquire) == nullptr) {
            //    m_ready_head.wait(nullptr, std::memory_order_relaxed);
            //}
        }
    }

    void IReplayManager::thread_player_job(std::stop_token token) {
        //while (!token.stop_requested()) {
        //    // this loads the saved replays async
//
        //    // fd iterator
        //    //  how to store references?
        //    //  as paths
        //    
//
//
//
        //    if (!m_sortedSessions.empty()) {
        //        auto&& front = m_sortedSessions.front();
        //        if (Valhalla()->Nanos() >= front.second.first) {
        //            auto&& peer = std::make_unique<Peer>(
        //                std::make_shared<ReplaySocket>(front.first, m_sessionIndexes[front.first]++, front.second.second));
//
        //            m_connectedPeers.push_back(std::move(peer));
        //            m_sortedSessions.pop_front();
        //        }
        //        else {
        //            PERIODIC_NOW(30s, {
        //                LOG(INFO) << "Replay peer joining in " << duration_cast<seconds>(front.second.first - Valhalla()->Nanos());
        //            });
        //        }
        //    }
        //}
    }

    void IReplayManager::on_new_peer(Peer::Ptr peer) {
        // record peer joindata
        const auto host = peer->m_socket->get_host_name();

        auto nanos = Avledet()->Nanos();

        const auto path = m_this_world_session_path
            / "hosts"
            / host
            / (std::to_string(nanos.count()) + ".replay");
            
        peer->m_replay_share = std::make_shared<XShare>(path);
    }

    void IReplayManager::on_peer_quit(Peer::Ptr peer) {
        assert(peer->m_replay_share);
        // TODO: must dump ALL, while not interfering
        auto &share = *peer->m_replay_share;

        if (share.m_closing.exchange(true, std::memory_order_acq_rel)) {
            return;
        }

        flush(share);
    }

    void IReplayManager::on_packet(Peer::Ptr peer, avledet::util::Bytes packet) {
        assert(peer->m_replay_share);

        auto& share = *peer->m_replay_share;
        if (share.m_closing.load(std::memory_order_acquire)) {
            // TODO: on_packet shouldnt be called while already closed
            assert(false);
            //LOG_WARNING(AVL_LOGGER, "Packet received when share closing");
            // IMPORTANT
            //  if closing, active buffer has been marked for async write
            //  therefore, unsfe to write to while in MAIN
            return;
        }

        auto* buf = share.m_active; // CONTENDED

        share.m_size_bytes += packet.size();
        buf->push_back({ Avledet()->Nanos(), std::move(packet) });

        if (avledet::util::run_periodic<struct gbhusdr>(1s)) {
            LOG_INFO(AVL_LOGGER, "share: {} B", share.m_size_bytes);
        }

        // 1'000'000 = 8 MB
        if (share.m_size_bytes < 2000000)
            return;

        this->flush(share);
    }

    void IReplayManager::flush(XShare& share) {
        bool expected = false;
        if (!share.m_in_flight.compare_exchange_strong(
                expected, true,
                std::memory_order_acq_rel))
        {
            return; // previous buffer still in-flight
        }

        share.m_size_bytes = 0;

        auto* full = share.m_active;

        // swap buffers
        share.m_active =
            (full == &share.m_buf1)
            ? &share.m_buf2
            : &share.m_buf1;

        //share.m_job.owner = share.shared_from_this();
        //share.m_job.buffer = full; // CONTENDED
        share.m_inactive = full;

        // HMM,
        //  ALL SEEMS WELL, HOWEVER,
        //  what happens in the unlikely, but very well possible, situation,
        //  where:
        //  - MAIN: a flush(), swaping active to buf2
        //  - BUILDER: starts that Job (buf1)
        //  - MAIN: another flush, so swapping active to buf1
        //  - BUILDER: explodes, because MAIN wrote to buf1 while BUILDER working on buf1
        //      well, fuck
        //  The above issue occurs because I cannot identify any sort of managed contention area 
        //      during the BUILDER working on an XShare, and that XShare being allowed to swap.
        //  ie, the XShare should ONLY be allowed to swap when its NOT in the to-be-processed list

        // nevermind, were all good now

        // Job* old = m_ready_head; // move stored link to mine
        // share.job.next = old
        // ready_head = share.job // I become the new N[0]

        auto pshare = share.shared_from_this();

        auto old = m_ready_head.load(std::memory_order_relaxed);

        do
        {
            //share.m_job.next = &old_share->m_job;
            share.next = old;
        }
        while (!m_ready_head.compare_exchange_weak(
            old,
            pshare,
            std::memory_order_release,
            std::memory_order_relaxed));

        //m_ready_head.notify_one();

        m_wakeup.store(true, std::memory_order_release);
        m_wakeup.notify_one();   // or notify_all()
    }

}