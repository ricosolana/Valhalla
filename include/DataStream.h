#pragma once

#include "VUtils.h"

template<typename T>
    requires (std::is_same_v<T, BYTE_VIEW_t> || std::is_same_v<T, std::reference_wrapper<BYTES_t>>)
class IDataStream {
public:
    T m_buf;

protected:
    size_t m_pos{};

protected:
    bool Check31U(size_t count) {
        return count > static_cast<size_t>(std::numeric_limits<int32_t>::max());
    }

    // Throws if the count exceeds int32_t::max signed size
    void Assert31U(size_t count) {
        assert(!Check31U(count) && "count is negative or exceeds 2^32 - 1");
        //if (Check31U(count))
            //throw std::runtime_error("count is negative or exceeds 2^32 - 1");
    }

    // Returns whether the specified position exceeds container length
    bool CheckPosition(size_t pos) const {
        return pos > Length();
    }

    // Throws if the specified position exceeds container length
    void AssertPosition(size_t pos) const {
        if (CheckPosition(pos))
            throw std::runtime_error("position exceeds length");
    }

    // Returns whether the specified offset from m_pos exceeds container length
    bool CheckOffset(size_t offset) const {
        return CheckPosition(Position() + offset);
    }

    // Throws if the specified offset from m_pos exceeds container length
    void AssertOffset(size_t offset) const {
        if (CheckOffset(offset))
            throw std::runtime_error("offset from position exceeds length");
    }

public:
    IDataStream(T buf) : m_buf(buf) {}

    IDataStream(T buf, size_t pos) : m_buf(buf) {
        SetPos(pos);
    }

public:
    size_t Position() const {
        return this->m_pos;
    }

    // Sets the position of this stream
    void SetPos(size_t pos) {
        Assert31U(pos);
        AssertPosition(pos);

        this->m_pos = pos;
    }

    size_t Length() const {
        if constexpr (std::is_same_v<T, BYTE_VIEW_t>)
            return m_buf.size();
        else
            return m_buf.get().size();
    }

    BYTE_t* data() {
        if constexpr (std::is_same_v<T, BYTE_VIEW_t>)
            return m_buf.data();
        else
            return m_buf.get().data();
    }

    const BYTE_t* data() const {
        if constexpr (std::is_same_v<T, BYTE_VIEW_t>)
            return m_buf.data();
        else
            return m_buf.get().data();
    }

    size_t Skip(size_t offset) {
        this->SetPos(this->Position() + offset);
    }
};
