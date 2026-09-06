#pragma once

#include "Avledet.h"
#include "DataStream.h"
#include "MonotonicMap.h"
#include "Quaternion.h"
#include "Types.h"
#include "VUtils.h"
#include "ZDOID.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <gtl/btree.hpp>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace avledet::replay {

    // https://facebook.github.io/zstd/doc/api_manual_latest.html
    // TODO move this class out of here
    //  placec into Utils...

    // 576
    class ZstdStreamTo {
    public:
        ZstdStreamTo(std::string const& outfile_path) 
            : filename(outfile_path) {}

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

        void init(int compressionLevel = ZSTD_defaultCLevel(),
                    const std::vector<char>* dictionary = nullptr,
                    std::size_t outBufferSize = ZSTD_CStreamOutSize())
        {
            if (this->cctx_) {
                assert(false);
                throw std::runtime_error("tried to init zstream twice");
            }

            outBuffer_.resize(outBufferSize);

            cctx_ = ZSTD_createCCtx();
            if (!cctx_) {
                assert(false);
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

        void open_file() {
            //this->filename = outputFile;

            if (!cctx_) {
                assert(false);
                throw std::runtime_error("must init zstream first");
            }

            // we dont throw, incase earlier finish() failed to reach closing the file
            if (out_.is_open()) {
                //throw std::runtime_error("tried to reopen stream");
                out_.close();
                LOG_WARNING(AVL_LOGGER, "tried reopening stream-file {}", this->filename);
            }

            // reset incase earlier throw in subsequent finish() occurred
            ZSTD_CCtx_reset(cctx_, ZSTD_reset_session_only);

            // make dirs
            std::filesystem::create_directories(std::filesystem::path(filename).parent_path());

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

        bool streaming_ready() noexcept {
            return this->cctx_ != nullptr;
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

    // 672 (base)
    // 688 (w/ enable_shared)
    class XShare : public std::enable_shared_from_this<XShare> {
    public:
        using PacketTS = std::pair<std::chrono::nanoseconds, avledet::util::Bytes>;
        using SwapBuffer = std::vector<PacketTS>;

        using Ptr = std::shared_ptr<XShare>;


        //std::chrono::nanoseconds m_time_begin;
        //std::chrono::nanoseconds m_time_end;
        
        SwapBuffer m_buf1;
        SwapBuffer m_buf2;

        SwapBuffer* m_active {};

        std::int32_t m_size_bytes {}; // flush every x MB, etc...        
        ZstdStreamTo m_zstream;

        SwapBuffer* m_inactive {}; //buffer
        //Job* next;
        XShare::Ptr next;

        //struct Job
        //{
        //    XShare::Ptr owner;
        //    SwapBuffer* buffer;
        //    //Job* next;
        //    XShare::Ptr next;
        //};

        //Job m_job {};

        std::atomic<bool> m_in_flight{false}; // <--- prevents unsafe swaps
        std::atomic<bool> m_closing{false}; // mark for cannibalize        

        XShare(std::filesystem::path path);
        ~XShare();

        //void on_packet(avledet::util::Bytes packet);
    };

    class DataTracker {
        public:

        // mutation tracker
        //  just tracks old -> present -> new -> newer -> newest
        //  presumably starts from oldest point in the zdo lifespan. time is either relative or absolute
        //  ... figure this out ...

        // milliseconds gives a nice blend between precision and efficiency
        //  (packets are sent out at 20ms intervals, zdo at 50ms, ...)

        // Contains changes of a given type vec over time. 
        //  For same-time changes, there is no specific ordering of hash members, except that they are relatively sequential neighbors
        //  TODO: if memory space is of concern:
        //      - store abs ms instead as uint delta ms since last packet
        //      - bake known long-form hashes into minified versions for given zdo
        //      - (I would say to store time as rel encoded int, but this introduces dynamic allocations...)
        template<class T>
        struct mut_node {
            //std::chrono::milliseconds m_time; // TODO could turn to uint32 rel delta ms
            avledet::util::Hash m_hash; // TODO could bake into an index
            T m_data;
        };

/*         template<class T>
        struct time_slice_union {
            // An array of time-segmented indices.
            //  Refers into m_data_nodes:
            //  - [0, 10, 24, 39, 67, 81, 129, 134, ...]
            //  - Performing an indexed query into time_slice_union acts as a time query into data_nodes
            //  - So, m_sliced_indices[4], is to retrieve the scaled time unit * 4.
            //      If scaled unit is in 'seconds', querying the 4th element is to get the zdo data member at that point in time
            //      This time is segmented to allow for slightly quicker value indexing within an equal interval space

            // However, there is no speciic need for this to require a time sliced helper...
            //  - data_nodes is ordered by time, so we could perform a binary search instead. log(n)
            //std::vector<std::uint32_t> m_sliced_indices; // contains indices referring to mut_node vector, 

            //std::vector<mut_node<T>> m_data_nodes;
            gtl::btree_map<std::chrono::milliseconds, mut_node<T>> m_data_nodes;
        }; */

        //util::mono::weak_btree_map<class Value, class Keys>

        template<class T>
        using Tree = gtl::btree_map<std::chrono::milliseconds, mut_node<T>>;

        template<class T>
        using TrackerMap = avledet::util::Map<avledet::util::ZDOID, Tree<T>, ZDO::hash,
                                                            std::equal_to<>>;

        std::tuple<        
            avledet::util::Map<ZDOID, time_slice_union<float>>,
            avledet::util::Map<ZDOID, time_slice_union<avledet::util::CSU::Vector3f>>,
            avledet::util::Map<ZDOID, time_slice_union<avledet::util::CSU::Quaternion>>,
            avledet::util::Map<ZDOID, time_slice_union<std::int32_t>>,
            avledet::util::Map<ZDOID, time_slice_union<std::int64_t>>,
            avledet::util::Map<ZDOID, time_slice_union<std::string>>,
            avledet::util::Map<ZDOID, time_slice_union<std::vector<char>>>
        > trackers;

        // should be extracted into outer class
        //std::vector<std::uint32_t> m_time_slices
        //template<typename T>
        //using is_member
        //        = VUtils::Traits::tuple_has_type<std::remove_cvref_t<T>,
        //                                        std::tuple<float, Vector3f, Quaternion, std::int32_t,
        //                                                    std::int64_t, std::string, std::vector<char>>>;
        //static inline VarMap<float> m_floats;
        //static inline VarMap<avledet::util::CSU::Vector3f> m_vec3;
        //static inline VarMap<avledet::util::CSU::Quaternion> m_quats;
        //static inline VarMap<std::int32_t> m_ints;
        //static inline VarMap<std::int64_t> m_longs;
        //static inline VarMap<std::string> m_strings;
        //static inline VarMap<std::vector<char>> m_byteArrays;
        // IDEA:
        //  think similarly to global zdo data map
        //  some zdos have only specific data members
        //  unused data types end up wasting space by have an empty vec
        //  so, how to contain a time ordered data-type slice vec for ALL zdos?
        //  simple:
        //avledet::util::Map<ZDOID, time_slice_union<float>> float_trackers;
        //avledet::util::Map<ZDOID, time_slice_union<avledet::util::CSU::Vector3f>> vec3_trackers;
        //avledet::util::Map<ZDOID, time_slice_union<avledet::util::CSU::Quaternion>> quat_trackers;
        //avledet::util::Map<ZDOID, time_slice_union<std::int32_t>> int_trackers;
        //avledet::util::Map<ZDOID, time_slice_union<std::int64_t>> long_trackers;
        //avledet::util::Map<ZDOID, time_slice_union<std::string>> string_trackers;
        //avledet::util::Map<ZDOID, time_slice_union<std::vector<char>>> bytes_trackers;

        // expects zdo packet/data
        // TODO will create the slices/node array
        //  will perform segmentation, calculations, and rel storage...
        //  remember, the entire point of this data structure is for efficient time querying
        void compile(avledet::util::Reader &reader);

        
    };

}
