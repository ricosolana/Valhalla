#include <Stream.h>

namespace avledet::util {

    Writer::Writer() { }
    Writer::Writer(std::vector<char> buf) : Stream(std::move(buf)) { }
    Writer::Writer(std::vector<char> buf, std::size_t pos) : Stream(std::move(buf), pos) { }

    void Writer::internal_write_bytes(const char* inBuf, std::size_t inBufSize) {
        if (!this->is_valid_offset(inBufSize))
            m_buf.resize(m_pos + inBufSize);

        std::copy(inBuf,
            inBuf + inBufSize,
            this->data() + m_pos);

        m_pos += inBufSize;
    }

    void Writer::write_varint(std::int32_t value) {
        auto num = static_cast<std::uint32_t>(value);
        for (; num >= 128U; num >>= 7)
            this->write(static_cast<std::uint8_t>(num | 128U));

        this->write(static_cast<std::uint8_t>(num));
    }

}// namespace avledet::util
