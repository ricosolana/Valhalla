#include <random>
#include <limits>
#include <zlib.h>
#include <zstd.h>

#include "VUtils.h"
#include "VUtilsMath.h"
#include "VUtilsMathf.h"

Color Color::Lerp(const Color& other, float t) {
    //t = VUtils::Math::Clamp01(t);
    return Color(
        VUtils::Mathf::Lerp(r, other.r, t),
        VUtils::Mathf::Lerp(g, other.g, t),
        VUtils::Mathf::Lerp(b, other.b, t),
        VUtils::Mathf::Lerp(a, other.a, t));
}

Color32 Color32::Lerp(const Color32 &other, float t) {
    return Color32(
        VUtils::Mathf::Lerp(r, other.r, t),
        VUtils::Mathf::Lerp(g, other.g, t),
        VUtils::Mathf::Lerp(b, other.b, t),
        VUtils::Mathf::Lerp(a, other.a, t));
}

//const Color Color::BLACK = Color();
//const Color Color::RED = Color(1, 0, 0);
//const Color Color::GREEN = Color(0, 1, 0);
//const Color Color::BLUE = Color(0, 0, 1);

std::ostream& operator<<(std::ostream& st, const UInt64Wrapper& val) {
    return st << (uint64_t)val;
}

std::ostream& operator<<(std::ostream& st, const Int64Wrapper& val) {
    return st << (int64_t)val;
}

namespace VUtils {

    std::optional<BYTES_t> CompressGz(const BYTE_t* in, unsigned int inSize, int level) {
        if (inSize == 0)
            return std::nullopt;

        BYTES_t out;

        z_stream zs{};

        // possible init errors are:
        //  - invalid param (can be fixed at compile time)
        //  - out of memory (unlikely)
        //  - incompatible version (should be fine if using the init macro)
        // https://stackoverflow.com/a/72499721
        if (deflateInit2(&zs, level, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
            return std::nullopt;

        // Set output buffer size to an upper bound compressed size
        // Might throw if out of memory (unlikely)
        out.resize(deflateBound(&zs, inSize));

        zs.avail_in = (uInt) inSize;
        zs.next_in = (Bytef*) in;
        zs.next_out = (Bytef*) out.data();
        zs.avail_out = out.size();

        auto r = deflate(&zs, Z_FINISH);
        if (r != Z_STREAM_END) {
            return std::nullopt;
        }

        out.resize(zs.total_out);

        return out;
    }

    std::optional<BYTES_t> CompressGz(const BYTES_t& in, int level) {
        return CompressGz(in.data(), in.size(), level);
    }

    std::optional<BYTES_t> CompressGz(const BYTES_t& in) {
        return CompressGz(in, Z_BEST_SPEED);
    }



    std::optional<BYTES_t> CompressZStd(const BYTES_t& in) {

        //ZSTD_compress2

        //ZSTD_CCtx_loadDictionary()

        //ZSTD_CCtx_setParameter


        BYTES_t out;
        out.resize(ZSTD_compressBound(in.size()));
        
        size_t status = ZSTD_compress(out.data(), out.size(), in.data(), in.size(), ZSTD_fast);

        if (ZSTD_isError(status))
            return std::nullopt;

        out.resize(status);

        return out;
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



}
