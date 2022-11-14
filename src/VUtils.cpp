#include <random>
#include <limits>
#include <openssl/rand.h>
#include <openssl/md5.h>
#include <zlib.h>

#include "VUtils.h"

namespace VUtils {

    int CompressGz(const BYTE_t* in, unsigned int bufSize, int level, BYTE_t* out, unsigned int outCapacity) {
        z_stream zs;
        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        zs.opaque = Z_NULL;
        zs.avail_in = (uInt) bufSize;
        zs.next_in = (Bytef*) in;
        zs.avail_out = (uInt) outCapacity; // HERE
        zs.next_out = (Bytef*) out;

        // possible init errors are:
        //  - invalid param (can be fixed at compile time)
        //  - out of memory (unlikely)
        //  - incompatible version (should be fine if using the init macro)
        // https://stackoverflow.com/a/72499721
        if (int res = deflateInit2(&zs, level, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
            return res;

        deflate(&zs, Z_FINISH);
        if (int res = deflateEnd(&zs) != Z_OK)
            return res;

        return zs.total_out;
    }

    int CompressGz(const BYTE_t* in, unsigned int inSize, BYTE_t* out, unsigned int outCapacity) {
        return CompressGz(in, inSize, Z_DEFAULT_COMPRESSION, out, outCapacity);
    }

    bool CompressGz(const BYTE_t* in, unsigned int inSize, int level, BYTES_t& out) {
        out.resize(inSize);
        int res = CompressGz(in, inSize, level, out.data(), out.size());
        if (res < 0)
            return false;

        out.resize(res);
        return true;
    }

    bool CompressGz(const BYTE_t* in, unsigned int inSize, BYTES_t& out) {
        return CompressGz(in, inSize, Z_DEFAULT_COMPRESSION, out);
    }



    int CompressGz(const BYTES_t& in, int level, BYTE_t* out, unsigned int outCapacity) {
        return CompressGz(in.data(), in.size(), out, outCapacity);
    }

    bool CompressGz(const BYTES_t& in, int level, BYTES_t& out) {
        return CompressGz(in.data(), in.size(), level, out);
    }

    bool CompressGz(const BYTES_t& in, BYTES_t& out) {
        return CompressGz(in, Z_DEFAULT_COMPRESSION, out);
    }



    BYTES_t CompressGz(const BYTE_t* in, unsigned int inSize, int level) {
        BYTES_t buf;
        if (!CompressGz(in, inSize, level, buf))
            throw compress_error("Unable to compress data");
        return buf;
    }

    BYTES_t CompressGz(const BYTE_t* in, unsigned int inSize) {
        return CompressGz(in, inSize, Z_DEFAULT_COMPRESSION);
    }

    BYTES_t CompressGz(const BYTES_t& in, int level) {
        return CompressGz(in.data(), in.size(), level);
    }

    BYTES_t CompressGz(const BYTES_t& in) {
        return CompressGz(in, in.size());
    }



    bool Decompress(const BYTE_t* in, unsigned int inSize, BYTES_t &out) {
        if (inSize == 0)
            return false;

        //BYTES_t result;
        out.resize(inSize);

        z_stream stream;
        stream.next_in = (Bytef*) in;
        stream.avail_in = inSize;
        stream.total_out = 0;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;

        if (inflateInit2(&stream, 15 | 16) != Z_OK)
            return false;

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
                return false;
            }
        }

        if (inflateEnd(&stream) != Z_OK)
            return false;

        // Trim off the extra-capacity inflated bytes
        out.resize(stream.total_out);
        return true;
    }

    BYTES_t Decompress(const BYTE_t* in, unsigned int inSize) {
        BYTES_t buf;
        if (!Decompress(in, inSize, buf))
            throw compress_error("unable to decompress");
        return buf;
    }

    bool Decompress(const BYTES_t& in, BYTES_t& out) {
        return Decompress(in.data(), in.size(), out);
    }

    BYTES_t Decompress(const BYTES_t& in) {
        return Decompress(in.data(), in.size());
    }



    void GenerateBytes(BYTE_t* out, unsigned int count) {
        RAND_bytes(out, count);
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
