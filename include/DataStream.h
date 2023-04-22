#pragma once

#include "VUtils.h"

class DataStream {
protected:
    size_t m_pos{};

protected:
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
        return CheckPosition(Position() + offset);
    }

    // Throws if the specified offset from m_pos exceeds container length
    void AssertOffset(size_t offset) const {
        if (CheckOffset(offset))
            throw std::runtime_error("offset from position exceeds length");
    }

public:
    DataStream() {}

    size_t Position() const {
        return this->m_pos;
    }

    // Sets the position of this stream
    void SetPos(size_t pos) {
        Assert31U(pos);
        AssertPosition(pos);

        this->m_pos = pos;
    }

    virtual size_t Length() const = 0;

    //virtual BYTE_t* data() = 0;

    //virtual const BYTE_t* data() const = 0;

    size_t Skip(size_t offset) {
        this->SetPos(this->Position() + offset);
    }
};
