#include <random>
#include <limits>
#include <openssl/rand.h>
#include <openssl/md5.h>

#include "Utils.h"

namespace Utils {
    // https://stackoverflow.com/questions/12398377/is-it-possible-to-have-zlib-read-from-and-write-to-the-same-memory-buffer
    // https://zlib.net/zpipe.c



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

    BYTES_t Decompress(const BYTES_t& in) {
        return Decompress(in.data(), in.size());
    }








    void FormatAscii(std::string& in) {

        //OPTICK_EVE
        //__LINE__;
        //__TIMESTAMP__;

        //__FILE__

        auto period = 5s;

        auto now = steady_clock::now();
        static auto last_run = now; // Initialized to this once
        auto elapsed = std::chrono::duration_cast<milliseconds>(now - last_run); // .count();

        // now repeat every period
        if (elapsed > period) {
            last_run = now;
            // run func
        }


        BYTE_t *data = reinterpret_cast<BYTE_t*>(in.data());
        for (int i = 0; i < in.size(); i++) {
            if (data[i] > 127)
                data[i] = 63;
        }
    }


    //std::string BytesToAscii(const BYTE_t* bytes, int count) {
    //    std::string result;
    //    for (int i = 0; i < count; i++) {
    //        if (bytes[i] > 127)
    //            result[i] = '?';
    //        else 
    //            result[i] =
    //            //bytes[i] = 63;
    //    }
    //}

    //void GenerateBytes(std::vec)

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

    HASH_t GetStableHashCode(const std::string& str) {
        int num = 5381;
        int num2 = num;
        int num3 = 0;
        while (str[num3] != '\0')
        {
            num = ((num << 5) + num) ^ (int)str[num3];
            if (str[num3 + 1] == '\0')
            {
                break;
            }
            num2 = ((num2 << 5) + num2) ^ (int)str[num3 + 1];
            num3 += 2;
        }
        return num + num2 * 1566083941;
    }



    // Will break on incorrectly encoded strings
    //int32_t GetUTF8Count(const char* p) {
    //    int count = 0;
    //    for (p; *p != 0; ++p)
    //        count += ((*p & 0xc0) != 0x80);
    //
    //    return count;
    //}

    // https://en.wikipedia.org/wiki/UTF-8#Encoding
    int32_t GetUTF8Count(const BYTE_t*p) {
        // leading bits:
        //   0: total 1 byte
        //   110: total 2 bytes (trailing 10xxxxxx)
        //   1110: total 3 bytes (trailing 10xxxxxx)
        //   11110: total 4 bytes (trailing 10xxxxxx)
        int32_t count = 0;
        for (; *p != '\0'; ++p, count++) {
#define CHECK_TRAILING_BYTES(n) \
        { \
            for (p++; /*next byte*/ \
                *p != '\0', i < (n); /*min bounds check*/ \
                ++p, ++i) /*increment*/ \
            { \
                if (((*p) >> 6) != 0b10) { \
                    return -1; \
                } \
            } \
            /* if string ended prematurely, panic */ \
            if (i != (n)) \
                return -1; \
        }

            // 1-byte code point
            if (((*p) >> 7) == 0b0) {
                continue;
            }
            else {
                int i = 0;
                // 2-byte code point
                if (((*p) >> 5) == 0b110) {
                    CHECK_TRAILING_BYTES(1);
                }
                // 3-byte code point
                else if (((*p) >> 4) == 0b1110) {
                    CHECK_TRAILING_BYTES(2);
                }
                // 4-byte code point
                else if (((*p) >> 3) == 0b11110) {
                    CHECK_TRAILING_BYTES(3);
                }
                else
                    return -1;
            }
        }
        return count;
    }

    //OWNER_t StringToUID(const std::string& s) {
    //    std::stringstream ss(s);
    //    OWNER_t uid;
    //    ss >> uid;
    //    return uid;
    //}

    std::string Join(std::vector<std::string_view>& strings) {
        std::string result;
        for (auto& s : strings) {
            result += s;
        }
        return result;
    }

    std::vector<std::string_view> Split(std::string& s, const std::string &delim) {
        std::string_view remaining(s);
        std::vector<std::string_view> result;
        int pos = 0;
        //ABC DE FGHI JK
        while ((pos = remaining.find(delim)) != std::string::npos) {
            // If the delim was not at idx 0, then add everything from 0 to the pos
            if (pos) result.push_back(remaining.substr(0, pos));
            // Trim everything before pos
            remaining = remaining.substr(pos + 1);
        }
        // add final match to list after delim
        if (!remaining.empty())
            result.push_back(remaining);
        return result;
    }

}
