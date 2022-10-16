#include <random>
#include <limits>
#include "Utils.h"

namespace Utils {
    // https://stackoverflow.com/questions/12398377/is-it-possible-to-have-zlib-read-from-and-write-to-the-same-memory-buffer
    // https://zlib.net/zpipe.c

    bool Compress(const byte_t* buf, unsigned int bufSize, int level, byte_t* out, unsigned int &outSize) {
        z_stream zs;
        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        zs.opaque = Z_NULL;
        zs.avail_in = (uInt) bufSize;
        zs.next_in = (Bytef*) buf;
        zs.avail_out = (uInt) outSize; // HERE
        zs.next_out = (Bytef*) out;

        // possible init errors are:
        //  - invalid param
        //  - out of memory
        //  - incompatible version
        if (deflateInit2(&zs, level, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
            return false;

        deflate(&zs, Z_FINISH);
        if (deflateEnd(&zs) != Z_OK)
            return false;

        outSize = zs.total_out;
        return true;
    }

    void Compress(const bytes_t& buf, int level, bytes_t& out) {
        out.resize(buf.size());

        unsigned int compressedSize = out.size();
        if (!Compress(buf.data(), buf.size(), level, out.data(), compressedSize))
            throw std::runtime_error("compression error");

        out.resize(compressedSize);
    }

    std::vector<byte_t> Compress(const byte_t* buf, unsigned int bufSize, int level) {
        std::vector<byte_t> out(bufSize);

        if (!Compress(buf, bufSize, level, out.data(), bufSize))
            throw std::runtime_error("compression error");

        out.resize(bufSize);

        return out;
    }





    // This method is unfinished and untested
    // it requires tinkering and validation
    bool Decompress(const byte_t* buf, unsigned int bufSize, byte_t **out, unsigned int &outSize) {
        //std::vector<byte_t> ret;

        if (bufSize == 0)
            return true;

        const unsigned full_length = bufSize;
        const unsigned half_length = bufSize / 2;

        unsigned uncompLength = full_length;
        std::unique_ptr<byte_t> uncomp = std::unique_ptr<byte_t>(new byte_t[uncompLength]);

        z_stream stream;
        stream.next_in = (Bytef*)buf;
        stream.avail_in = bufSize;
        stream.total_out = 0;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;

        if (inflateInit2(&stream, (16 + MAX_WBITS)) != Z_OK)
            throw std::runtime_error("unable to decompress gzip stream");

        while (true) {
            // If our output buffer is too small  
            if (stream.total_out >= uncompLength) {
                // Increase size of output buffer  
                auto old = std::move(uncomp);
                uncomp = std::unique_ptr<byte_t>(new byte_t[uncompLength + half_length]);
                memcpy(uncomp.get(), old.get(), uncompLength);
                uncompLength += half_length;
            }

            stream.next_out = (Bytef*)(uncomp.get() + stream.total_out);
            stream.avail_out = uncompLength - stream.total_out;

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

        std::copy(uncomp.get(), uncomp.get() + stream.total_out, *out);
        return true;
    }

    std::vector<byte_t> Decompress(const byte_t* compressedBytes, int count) {
        std::vector<byte_t> ret;

        if (count == 0)
            return ret;

        unsigned full_length = count;
        unsigned half_length = count / 2;

        unsigned uncompLength = full_length;
        std::unique_ptr<byte_t> uncomp = std::unique_ptr<byte_t>(new byte_t[uncompLength]);

        z_stream stream;
        stream.next_in = (Bytef*)compressedBytes;
        stream.avail_in = count;
        stream.total_out = 0;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;

        bool done = false;

        if (inflateInit2(&stream, (16 + MAX_WBITS)) != Z_OK)
            throw std::runtime_error("unable to decompress gzip stream");

        while (!done) {
            // If our output buffer is too small  
            if (stream.total_out >= uncompLength) {
                // Increase size of output buffer  
                auto old = std::move(uncomp);
                uncomp = std::unique_ptr<byte_t>(new byte_t[uncompLength + half_length]);
                memcpy(uncomp.get(), old.get(), uncompLength);
                uncompLength += half_length;
            }

            stream.next_out = (Bytef*)(uncomp.get() + stream.total_out);
            stream.avail_out = uncompLength - stream.total_out;

            // Inflate another chunk.  
            int err = inflate(&stream, Z_SYNC_FLUSH);
            if (err == Z_STREAM_END) done = true;
            else if (err != Z_OK) {
                throw std::runtime_error("decompression error " + std::to_string(err));
            }
        }

        if (inflateEnd(&stream) != Z_OK)
            throw std::runtime_error("unable to end decompressing gzip stream");

        ret.insert(ret.begin(), uncomp.get(), uncomp.get() + stream.total_out);

        return ret;
    }

    std::random_device rd;     //Get a random seed from the OS entropy device, or whatever
    std::mt19937_64 eng(rd()); //Use the 64-bit Mersenne Twister 19937 generator
                               //and seed it with entropy.

    uuid_t GenerateUID() {
        //Define the distribution, by default it goes from 0 to MAX(unsigned long long)
        //or what have you.
        std::uniform_int_distribution<int64_t> distr;
        return distr(eng);
    }

    int32_t GetStableHashCode(const std::string& str) {
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

    // exclude any 0b10xxxxxx (as these are trail bytes)
    // this function is optimized under the assumption that the
    // input string is correctly utf-8 encoded
    // it will work incorrectly if it is not encoded expectedly
    //int32_t GetUTF8Count(const char* p) {
    //    int count = 0;
    //    for (p; *p != 0; ++p)
    //        count += ((*p & 0xc0) != 0x80);
    //
    //    return count;
    //}

    // https://en.wikipedia.org/wiki/UTF-8#Encoding
    int32_t GetUTF8Count(const byte_t*p) {
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
                *p != '\0', i < n; /*min bounds check*/ \
                ++p, ++i) /*increment*/ \
            { \
                if (((*p) >> 6) != 0b10) { \
                    return -1; \
                } \
            } \
            /* if string ended prematurely, panic */ \
            if (i != n) \
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

    uuid_t StringToUID(const std::string& s) {
        std::stringstream ss(s);
        uuid_t uid;
        ss >> uid;
        return uid;
    }

    bool IsAddress(const std::string& s) {
        //address make_address(const char* str,
        //    asio::error_code & ec) ASIO_NOEXCEPT
        asio::error_code ec;
        asio::ip::make_address(s, ec);
        return ec ? false : true;
    }

    std::string Join(std::vector<std::string_view>& strings) {
        std::string result;
        for (auto& s : strings) {
            result += s;
        }
        return result;
    }

    std::vector<std::string_view> Split(std::string_view s, char ch) {
        //std::string s = "scott>=tiger>=mushroom";

        // split in Java appears to be a recursive decay function (for the pattern)

        int off = 0;
        int next = 0;
        std::vector<std::string_view> list;
        while ((next = s.find(ch, off)) != std::string::npos) {
            list.push_back(s.substr(off, next));
            off = next + 1;
        }
        // If no match was found, return this
        if (off == 0)
            return { s };

        // Add remaining segment
        list.push_back(s.substr(off));

        // Construct result
        int resultSize = list.size();
        while (resultSize > 0 && list[resultSize - 1].empty()) {
            resultSize--;
        }

        return std::vector<std::string_view>(list.begin(), list.begin() + resultSize);






        //std::vector<std::string_view> res;
        //
        //size_t pos = 0;
        //while ((pos = s.find(delimiter)) != std::string::npos) {
        //    //std::cout << token << std::endl;
        //    res.push_back(s.substr(0, pos));
        //    //s.erase(0, pos + delimiter.length());
        //    if (pos + delimiter.length() == s.length())
        //        s = s.substr(pos + delimiter.length());
        //    else s = s.substr(pos + delimiter.length());
        //}
        //if (res.empty())
        //    res.push_back(s);
        ////std::cout << s << std::endl;
        //return res;
    }
}
