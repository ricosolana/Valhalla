#pragma once

#include "VUtils.h"

class DataStream {
protected:
    // Returns whether count exceeds 2^32 - 1 (INT32_MAX)
    static bool Check31U(size_t count) {
        return count > static_cast<size_t>(std::numeric_limits<int32_t>::max());
    }

    // Throws if the count exceeds int32_t::max signed size
    static void Assert31U(size_t count) {
        if (Check31U(count))
            throw std::runtime_error("count is negative or exceeds 2^32 - 1");
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
        return CheckPosition(m_pos + offset);
    }

    // Throws if the specified offset from m_pos exceeds container length
    void AssertOffset(size_t offset) const {
        if (CheckOffset(offset))
            throw std::runtime_error("offset from position exceeds length");
    }

public:
    std::reference_wrapper<BYTES_t> m_provider;
    size_t m_pos;

public:
    DataStream(BYTES_t &bytes) : m_provider(bytes), m_pos(0) {}

    // Returns the length of this stream
    size_t Length() const {
        return m_provider.get().size(); // static_cast<int32_t>(m_provider.get().size());
    }

    // Returns the position of this stream
    size_t Position() const {
        return m_pos;
    }

    // Sets the positino of this stream
    void SetPos(size_t pos) {
        Assert31U(pos);
        AssertPosition(pos);

        m_pos = pos;
    }

};
