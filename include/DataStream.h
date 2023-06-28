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
    bool CheckPosition(size_t pos) const noexcept {
        return pos > size();
    }

    // Throws if the specified position exceeds container length
    void AssertPosition(size_t pos) const {
        if (CheckPosition(pos))
            throw std::runtime_error("position exceeds length");
    }

    // Returns whether the specified offset from m_pos exceeds container length
    bool CheckOffset(int offset, size_t pos) const noexcept {
        return CheckPosition(pos + offset);
    }

    bool CheckOffset(int offset) const noexcept {
        return CheckOffset(offset, Position());
    }

    // Throws if the specified offset from m_pos exceeds container length
    void AssertOffset(int offset, size_t pos) const {
        if (CheckOffset(offset, pos))
            throw std::runtime_error("offset from position exceeds length");
    }

    void AssertOffset(int offset) const {
        AssertOffset(offset, Position());
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

    // TODO rename SetPosition
    // Sets the position of this stream
    void SetPos(size_t pos) {
        AssertPosition(pos);
        this->m_pos = pos;
    }

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

    // Increases the size of this stream by x bytes
    void extend(size_t count) {
        std::visit(VUtils::Traits::overload{
            [this, count](std::reference_wrapper<BYTES_t> buf) {
                if (this->CheckOffset(count))
                    buf.get().resize(this->m_pos + count);
            },
            [this, count](BYTE_VIEW_t buf) { this->AssertOffset(count); }
        }, this->m_data);
    }


    // Move forward by x bytes
    //  Does not add bytes
    void Skip(size_t count) {
        this->SetPos(this->Position() + count);
    }

    // Move forward by x bytes
    //  Bytes can be allocated
    void Advance(size_t count) {
        extend(count);
        Skip(count);
    }
};

// https://akrzemi1.wordpress.com/2017/08/12/your-own-error-condition/
//template <> struct std::is_error_condition_enum<DataStream::Error> : true_type {};
