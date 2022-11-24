#include <random>
#include <limits>
#include <openssl/rand.h>
#include <openssl/md5.h>
#include <zlib.h>

#include "VUtils.h"

namespace VUtils {

    std::optional<BYTES_t> CompressGz(const BYTE_t* in, unsigned int inSize, int level) {
        if (inSize == 0)
            return std::nullopt;

        BYTES_t out;

        z_stream zs;
        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        zs.opaque = Z_NULL;
        zs.avail_in = (uInt) inSize;
        zs.next_in = (Bytef*) in;
        zs.next_out = (Bytef*) out.data();

        // possible init errors are:
        //  - invalid param (can be fixed at compile time)
        //  - out of memory (unlikely)
        //  - incompatible version (should be fine if using the init macro)
        // https://stackoverflow.com/a/72499721
        if (deflateInit2(&zs, level, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
            return std::nullopt;

        // Set output buffer size to an upper bound compressed size
        out.resize(deflateBound(&zs, inSize));

        zs.avail_out = out.size();

        assert(deflate(&zs, Z_FINISH) == Z_STREAM_END);

        out.resize(zs.total_out);

        return out;
    }

    std::optional<BYTES_t> CompressGz(const BYTES_t& in, int level) {
        return CompressGz(in.data(), in.size(), level);
    }

    std::optional<BYTES_t> CompressGz(const BYTES_t& in) {
        return CompressGz(in, Z_BEST_SPEED);
    }



    std::optional<BYTES_t> Decompress(const BYTE_t* in, unsigned int inSize) {
        if (inSize == 0)
            return std::nullopt;

        BYTES_t out;
        out.resize(inSize + inSize/2);

        z_stream stream;
        stream.next_in = (Bytef*) in;
        stream.avail_in = inSize;
        stream.total_out = 0;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;

        if (inflateInit2(&stream, 15 | 16) != Z_OK)
            return std::nullopt;

        while (true) {
            // If our output buffer is too small
            if (stream.total_out >= out.size()) {
                out.resize(stream.total_out + inSize/2);
            }

            // Advance to the next chunk to decode
            stream.next_out = (Bytef*)(out.data() + stream.total_out);

            // Set the available output capacity
            stream.avail_out = out.size() - stream.total_out;

            // Inflate another chunk.
            int err = inflate(&stream, Z_SYNC_FLUSH);
            if (err == Z_STREAM_END)
                break;
            else if (err != Z_OK) {
                return std::nullopt;
            }
        }

        if (inflateEnd(&stream) != Z_OK)
            return std::nullopt;

        // Trim off the extra-capacity inflated bytes
        out.resize(stream.total_out);
        return out;
    }

    std::optional<BYTES_t> Decompress(const BYTES_t& in) {
        return Decompress(in.data(), in.size());
    }



    void GenerateBytes(BYTE_t* out, unsigned int count) {
        RAND_bytes((unsigned char*) out, count);
    }

    BYTES_t GenerateBytes(unsigned int count) {
        BYTES_t result(count);

        GenerateBytes(result.data(), count);

        return result;
    }



    OWNER_t GenerateUID() {
        OWNER_t result;
        GenerateBytes(reinterpret_cast<BYTE_t*>(&result), sizeof(result));
        return result;
    }
}
