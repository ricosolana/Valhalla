#pragma once

#include "VUtils.h"

// 56 Bytes w/ <vec, view>
// 32 Bytes w/ <vec&, view>
class DataStream {
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
        return pos > size();
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
    explicit DataStream(BYTE_VIEW_t buf) : m_unownedBuf(buf) {}
    explicit DataStream(BYTES_t buf) : m_ownedBuf(std::move(buf)) {}

    explicit DataStream(BYTE_VIEW_t buf, size_t pos) : m_unownedBuf(buf) {
        SetPos(pos);
    }

    explicit DataStream(BYTES_t buf, size_t pos) : m_ownedBuf(std::move(buf)) {
        SetPos(pos);
    }

public:
    bool owned() const { return !this->m_unownedBuf.data(); }

    size_t Position() const {
        return this->m_pos;
    }

    // Sets the position of this stream
    void SetPos(size_t pos) {
        Assert31U(pos);
        AssertPosition(pos);

        this->m_pos = pos;
    }

    // TODO rename size() for standard conformance
    size_t size() const {
        return this->owned() ? this->m_ownedBuf.size() : this->m_unownedBuf.size();
    }

    BYTE_t* data() {
        return this->owned() ? this->m_ownedBuf.data() : this->m_unownedBuf.data();
    }

    const BYTE_t* data() const {
        return this->owned() ? this->m_ownedBuf.data() : this->m_unownedBuf.data();
    }

    size_t Skip(size_t offset) {
        this->SetPos(this->Position() + offset);
    }
};
