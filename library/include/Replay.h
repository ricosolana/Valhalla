#pragma once

#include "Avledet.h"
#include "VUtils.h"
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace avledet::replay {

    // https://facebook.github.io/zstd/doc/api_manual_latest.html
    // TODO move this class out of here
    //  placec into Utils...
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
                throw std::runtime_error("tried to init zstream twice");
            }

            outBuffer_.resize(outBufferSize);

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

        void open_file() {
            //this->filename = outputFile;

            if (!cctx_) {
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

        bool streaming_ready() {
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

    struct XShare : public std::enable_shared_from_this<XShare> {
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
        

        XShare(std::filesystem::path path);
        ~XShare();

        //void on_packet(avledet::util::Bytes packet);
    };

}
