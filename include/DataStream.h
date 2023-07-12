#pragma once

#include "VUtils.h"

// 56 Bytes w/ <vec, view>
// 32 Bytes w/ <vec&, view>
class DataStream {
public:
    std::variant<std::reference_wrapper<BYTES_t>, BYTE_SPAN_t> m_data;

protected:
    size_t m_pos{};

protected:
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
    explicit DataStream(BYTE_SPAN_t buf) : m_data(buf) {}
    explicit DataStream(BYTES_t& buf) : m_data(std::ref(buf)) {}

    explicit DataStream(BYTE_SPAN_t buf, size_t pos) : m_data(buf) {
        SetPosition(pos);
    }

    explicit DataStream(BYTES_t& buf, size_t pos) : m_data(std::ref(buf)) {
        SetPosition(pos);
    }

public:
    //bool owned() const { return std::get_if< !this->m_data.data(); }

    size_t Position() const {
        return this->m_pos;
    }

    // Sets the position of this stream
    void SetPosition(size_t pos) {
        AssertPosition(pos);

        this->m_pos = pos;
    }

    // TODO rename size() for standard conformance
    size_t size() const {
        return std::visit(VUtils::Traits::overload{
            [](std::reference_wrapper<BYTES_t> buf) -> size_t { return buf.get().size(); },
            [](BYTE_SPAN_t buf) -> size_t { return buf.size(); }
        }, this->m_data);
    }

    BYTE_t* data() {
        return std::visit(VUtils::Traits::overload{
            [](std::reference_wrapper<BYTES_t> buf) -> BYTE_t* { return buf.get().data(); },
            [](BYTE_SPAN_t buf) -> BYTE_t* { return buf.data(); }
        }, this->m_data);
    }

    const BYTE_t* data() const {
        return std::visit(VUtils::Traits::overload{
            [](std::reference_wrapper<BYTES_t> buf) { return buf.get().data(); },
            [](BYTE_SPAN_t buf) { return buf.data(); }
        }, this->m_data);
    }

    size_t Skip(size_t offset) {
        this->SetPosition(this->Position() + offset);
    }
};
