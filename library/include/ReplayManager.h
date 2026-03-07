#pragma once

#include "Avledet.h"
#include "DataStream.h"
#include "Hashes.h"
#include "NetSocket.h"
#include "Peer.h"
#include "Quaternion.h"
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

    class ZstdStreamTo {
    public:
        ZstdStreamTo(int compressionLevel = ZSTD_defaultCLevel(),
                    const std::vector<char>* dictionary = nullptr,
                    size_t outBufferSize = ZSTD_CStreamOutSize())
            : outBuffer_(outBufferSize)
        {
            cctx_ = ZSTD_createCCtx();
            if (!cctx_) {
                throw std::runtime_error("ZSTD_createCCtx failed");
            }

            this->zassert(ZSTD_CCtx_setParameter(
                cctx_, ZSTD_c_compressionLevel, compressionLevel));
            
            if (dictionary) {
                if (!dictionary->empty()) {
                    this->zassert(ZSTD_CCtx_loadDictionary(
                        cctx_, dictionary->data(), dictionary->size()));
                } else {
                    throw std::runtime_error("empty stream dictionary");
                }
            }
        }

        ~ZstdStreamTo() {
            if (cctx_) {
                try {
                    this->finish();
                } catch(std::exception const& e) {
                    LOG_WARNING(AVL_LOGGER, "stream dctor {}", e.what());
                }
                ZSTD_freeCCtx(cctx_);
            }
        }

        void startStream(const std::string& outputFile) {
            this->filename = outputFile;

            // we dont throw, incase earlier finish() failed to reach closing the file
            if (out_.is_open()) {
                //throw std::runtime_error("tried to reopen stream");
                out_.close();
                LOG_WARNING(AVL_LOGGER, "tried reusing open stream-file {} as {}", this->filename, outputFile);
            }

            // reset incase earlier throw in subsequent finish() occurred
            ZSTD_CCtx_reset(cctx_, ZSTD_reset_session_only);

            out_.open(filename + ".tmp", std::ios::binary);
            out_.exceptions(std::ios::badbit | std::ios::failbit); //loss of integrity

            if (!out_) {
                throw std::runtime_error("Failed to open output file");
            }
        }

        void compressChunk(const void* data, std::size_t size) {
            if (!out_.is_open()) {
                throw std::runtime_error("tried streaming to unopen file");
            }

            ZSTD_inBuffer input{ data, size, 0 };

            while (input.pos < input.size) {
                ZSTD_outBuffer output{
                    outBuffer_.data(),
                    outBuffer_.size(),
                    0
                };

                std::size_t ret = ZSTD_compressStream2(
                    cctx_, &output, &input, ZSTD_e_continue);

                this->zassert(ret);

                out_.write(
                    reinterpret_cast<char*>(output.dst),
                    output.pos);
            }
        }

        void finish() {
            if (!out_.is_open()) {
                //LOG_DEBUG(AVL_LOGGER, "tried finish() on unopen stream");
                //return false;
                throw std::runtime_error("tried finish() on unopen stream");
            }

            ZSTD_inBuffer input{ nullptr, 0, 0 };

            std::size_t remaining;
            do {
                ZSTD_outBuffer output{
                    outBuffer_.data(),
                    outBuffer_.size(),
                    0
                };

                remaining = ZSTD_compressStream2(
                    cctx_, &output, &input, ZSTD_e_end);

                this->zassert(remaining);
                out_.write(
                    reinterpret_cast<char*>(output.dst),
                    output.pos);

            } while (remaining != 0);

            out_.close();

            // rename the .tmp to name
            std::filesystem::rename(filename + ".tmp", filename);

            //return true;
        }

    private:
        bool zgood(std::size_t code) {
            return !ZSTD_isError(code);
        }

        void zassert(std::size_t code) {
            if (!zgood(code)) {
                throw std::runtime_error(ZSTD_getErrorName(code));
            }
        }

    private:
        ZSTD_CCtx* cctx_ {};
        std::ofstream out_;
        std::vector<char> outBuffer_;
        std::string filename;
    };

    class ReplayManager
    {
      public:

        //struct Record {
        //    std::string m_hostname_originate;
        //    std::chrono::nanoseconds m_timestamp;
        //    avledet::util::Bytes m_packet;
        //};

        /*
        struct Session {
            //std::string m_hostname_originate;
            std::chrono::nanoseconds m_join_time;
            std::vector<std::pair<
                std::chrono::nanoseconds, // packet send time
                avledet::util::Bytes>> m_swap_packets; // packets waiting

                // Flush idea
                //  Writer thread *Should* keep a handle open on the files
                //  otherwise, corruption is possible if deleted halfway
                //  but then i'd have to track OPEN files ...

                // better:
                //  allow accumulation of packets (up to a size)
                //  then, thread will perform a flush,
                //  just effectively swapping the active with the inactive

            //std::chrono::steady_clock::time_point m_
            std::chrono::nanoseconds m_tracked_flush; // flush every 1min, etc...
            std::atomic_int32_t m_tracked_size; // flush every x MB, etc...
            
            ZstdStreamTo m_zstd;

            Session(std::chrono::nanoseconds join_time)
                : m_join_time(join_time) {
            }
        };*/

        // we dont want to store the same string hostname every packet
        //avledet::util::Map<std::string, std::uint32_t> m_hostname_hash_dict;
        // the above doesnt matter too much because compression will help

        //std::vector<
        //    std::pair<
        //        std::string, // hostname
        //        std::pair<
        //            std::chrono::nanoseconds, // start time
        //            std::chrono::nanoseconds> // end time
        //        >> m_sortedSessions;

        //avledet::util::Map<std::string, std::int32_t> m_sessionIndexes; // hostname, next session index

        //std::vector<avledet::util::Bytes> m_packets;

        //std::vector<Record> m_records_in; // sockets write to this
        //std::vector<Record> m_records_out; // thread reads from this
        //std::vector<Session> m_sessions_digest;

        // vars marked with an x
        //  are consumer thread only
        //  everything else is assumed shared
        struct XShare {
            // every joining player has a timestamp
            const std::chrono::nanoseconds m_time_begin;

            // a host name
            //std::string m_hostname;
            //const Peer::Ptr m_peer;

            // a stream of packets
            std::vector<std::pair<std::chrono::nanoseconds, avledet::util::Bytes>> m_swap_packets;

            // the compressor for those packets
            ZstdStreamTo m_x_zstd;

            // a quit time
            std::chrono::nanoseconds m_time_end;

            std::chrono::nanoseconds m_x_tracked_flush; // flush every 1min, etc...
            std::atomic_int32_t m_x_tracked_size; // flush every x MB, etc...

            XShare(std::filesystem::path path,
                std::chrono::nanoseconds time_begin) 
                : m_time_begin(time_begin)
            {
                //m_x_zstd.startStream(socket->get_host_name());
                m_x_zstd.startStream(path);
                
                assert(false);
                // TODO determine path
            }
        };

        avledet::util::Map<ISocket::Ptr, std::unique_ptr<XShare>> m_digest;

        // a per-player session container
        //  sessions are periods of JOIN <=> QUIT
        //avledet::util::Map<std::string, std::vector<Session>> m_digest;
        //std::mutex m_mux; // used when swapping vecs
        std::shared_mutex m_mux; // used when swapping vecs
        std::jthread m_thread;

        std::chrono::nanoseconds m_server_nanos;

        std::filesystem::path m_this_world_session_path;

        // when m_records_out has been fully processed / written to disk,
        //  we swap with m_records_in,
        //  then continue the process...

      public:
        void init();

        void prime_new_capture(ISocket::Ptr socket);

        void on_packet(ISocket::Ptr socket, avledet::util::Bytes packet);

      public:
        //static std::vector<RandomSpawn> parse_list(util::Reader &reader);
    };

}// namespace avledet::replay
