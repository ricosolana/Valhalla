#include "DataStream.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace avledet::util {

    Reader::Reader() {}

    Reader::Reader(std::vector<char> buf) :
        Stream(std::move(buf))
    {
    }

    Reader::Reader(std::vector<char> buf, std::size_t pos) :
        Stream(std::move(buf), pos)
    {
    }

    //Reader Reader::from_file(std::filesystem::path path) {
    //    std::ifstream file(path, std::ios::binary);

    //    if (!file)
    //        throw std::runtime_error("file not found: " + path.string());

    //    file.unsetf(std::ios::skipws);

    //    file.seekg(0, std::ios::end);
    //    auto fileSize = file.tellg();
    //    file.seekg(0, std::ios::beg);

    //    if (fileSize < 0)
    //        throw std::runtime_error("file reading failure: " + path.string());

    //    std::vector<char> result {};
    //    result.resize(static_cast<std::uint64_t>(fileSize));
    //    file.read(reinterpret_cast<std::ifstream::char_type*>(result.data()),
    //        fileSize);

    //    return Reader(std::move(result));
    //}


    void Reader::internal_read_bytes(char *outBuf, std::size_t outBufSize)
    {
        this->check_offset(outBufSize);

        std::copy(this->data() + m_pos, this->data() + m_pos + outBufSize, outBuf);

        m_pos += outBufSize;
    }

    std::int32_t Reader::read_varint()
    {
        std::uint32_t out  = 0;
        std::uint32_t num2 = 0;
        while (num2 != 35) {
            auto b = this->read<std::uint8_t>();
            out |= static_cast<decltype(out)>(b & 127) << num2;
            num2 += 7;
            if ((b & 128) == 0) {
                return static_cast<std::int32_t>(out);
            }
        }
        throw std::runtime_error("bad varint");
    }

}// namespace avledet::util
