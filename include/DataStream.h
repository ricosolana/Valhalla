#pragma once

#include "VUtils.h"

// 56 Bytes w/ <vec, view>
// 32 Bytes w/ <vec&, view>
class DataStream {
public:
    std::variant<std::reference_wrapper<BYTES_t>, BYTE_VIEW_t> m_data;
    //BYTES_t *m_ownedBuf;
    //BYTE_VIEW_t m_unownedBuf;

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
    explicit DataStream(BYTE_VIEW_t buf) : m_data(buf) {}
    explicit DataStream(BYTES_t& buf) : m_data(std::ref(buf)) {}

    explicit DataStream(BYTE_VIEW_t buf, size_t pos) : m_data(buf) {
        SetPos(pos);
    }

    explicit DataStream(BYTES_t& buf, size_t pos) : m_data(std::ref(buf)) {
        SetPos(pos);
    }

public:
    //bool owned() const { return std::get_if< !this->m_data.data(); }

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
        return std::visit(VUtils::Traits::overload{
            [](std::reference_wrapper<BYTES_t> buf) { return buf.get().size(); },
            [](BYTE_VIEW_t buf) { return buf.size(); }
        }, this->m_data);
    }

    BYTE_t* data() {
        return std::visit(VUtils::Traits::overload{
            [](std::reference_wrapper<BYTES_t> buf) { return buf.get().data(); },
            [](BYTE_VIEW_t buf) { return buf.data(); }
        }, this->m_data);
    }

    const BYTE_t* data() const {
        return std::visit(VUtils::Traits::overload{
            [](std::reference_wrapper<BYTES_t> buf) { return buf.get().data(); },
            [](BYTE_VIEW_t buf) { return buf.data(); }
        }, this->m_data);
    }

    size_t Skip(size_t offset) {
        this->SetPos(this->Position() + offset);
    }
};
