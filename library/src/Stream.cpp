#include "DataStream.h"
#include <cstddef>
#include <stdexcept>

namespace avledet::util {

    Stream::Stream() :
        m_pos(0)
    {
    }

    Stream::Stream(std::vector<char> buf) :
        m_buf(std::move(buf)),
        m_pos(0)
    {
    }

    Stream::Stream(std::vector<char> buf, std::size_t pos) :
        m_buf(std::move(buf)),
        m_pos(pos)
    {
        check_pos(pos);
    }

    bool Stream::is_valid_pos(std::size_t pos) const
    {
        return pos <= this->size();
    }

    bool Stream::is_valid_offset(std::size_t offset) const
    {
        return this->is_valid_pos(this->get_pos() + offset);
    }

    void Stream::check_pos(std::size_t pos) const
    {
        if (!this->is_valid_pos(pos))
            throw std::runtime_error("invalid pos");
    }

    void Stream::check_offset(std::size_t offset) const
    {
        if (!this->is_valid_offset(offset))
            throw std::runtime_error("invalid offset");
    }

    std::size_t Stream::get_pos() const
    {
        return m_pos;
    }

    void Stream::set_pos(std::size_t pos)
    {
        this->check_pos(pos);

        this->unsafe_set_pos(pos);
    }

    void Stream::seek(std::size_t offset)
    {
        this->check_offset(offset);

        this->unsafe_seek(offset);
    }

    void Stream::unsafe_set_pos(std::size_t pos)
    {
        m_pos = pos;
    }

    void Stream::unsafe_seek(std::size_t offset)
    {
        m_pos += offset;
    }

    bool Stream::empty() const
    {
        return m_buf.empty();
    }

    std::size_t Stream::size() const
    {
        return m_buf.size();
    }

    char *Stream::data()
    {
        return m_buf.data();
    }

    char const *Stream::data() const
    {
        return m_buf.data();
    }

    std::vector<char> &Stream::get_buf()
    {
        return m_buf;
    }

}// namespace avledet::util