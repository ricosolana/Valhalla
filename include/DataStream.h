#pragma once

#include "VUtils.h"

// 56 Bytes w/ <vec, view>
// 32 Bytes w/ <vec&, view>
class DataStream {
public:
    std::variant<std::reference_wrapper<BYTES_t>, BYTE_VIEW_t> m_data;

protected:
    size_t m_pos{};

protected:
    // Returns whether the specified position exceeds container length
    bool check_pos(size_t pos) const {
        return pos > size();
    }

    // Throws if the specified position exceeds container length
    void try_pos(size_t pos) const {
        if (check_pos(pos))
            throw std::runtime_error("position exceeds length");
    }

    // Returns whether the specified offset from m_pos exceeds container length
    bool check_offset(size_t offset) const {
        return check_pos(get_pos() + offset);
    }

    // Throws if the specified offset from m_pos exceeds container length
    void try_offset(size_t offset) const {
        if (check_offset(offset))
            throw std::runtime_error("offset from position exceeds length");
    }

public:
    explicit DataStream(BYTE_VIEW_t buf) : m_data(buf) {}
    explicit DataStream(BYTES_t& buf) : m_data(std::ref(buf)) {}

    explicit DataStream(BYTE_VIEW_t buf, size_t pos) : m_data(buf) {
        set_pos(pos);
    }

    explicit DataStream(BYTES_t& buf, size_t pos) : m_data(std::ref(buf)) {
        set_pos(pos);
    }

public:
    //bool owned() const { return std::get_if< !this->m_data.data(); }

    size_t get_pos() const {
        return this->m_pos;
    }

    // Sets the position of this stream
    void set_pos(size_t pos) {
        try_pos(pos);

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

    size_t skip(size_t offset) {
        this->set_pos(this->get_pos() + offset);
    }
};
