#pragma once

#include "Avledet.h"
#include "DataStream.h"
#include "Hashes.h"
#include "NetSocket.h"
#include "Peer.h"
#include "Quaternion.h"
#include "Replay.h"
#include "Types.h"
#include "VUtils.h"
#include "Vector.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
#include <quill/LogMacros.h>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace avledet::replay {

    class IReplayManager
    {
      public:
        std::jthread m_thread;
        std::chrono::nanoseconds m_server_nanos;
        std::filesystem::path m_this_world_session_path;
        std::atomic<XShare::Ptr> m_ready_head = nullptr;

      private:
        void emit_to_stream(XShare& share, XShare::SwapBuffer const& buf);
        void thread_job(std::stop_token);

      public:
        void init();
        void on_new_peer(Peer::Ptr peer);
        void on_peer_quit(Peer::Ptr peer);
        void on_packet(Peer::Ptr peer, avledet::util::Bytes packet);
        void flush(XShare& share);
    };

    IReplayManager *ReplayManager();

}// namespace avledet::replay
