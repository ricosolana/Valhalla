#pragma once

#include "VUtils.h"

// 56 Bytes w/ <vec, view>
// 32 Bytes w/ <vec&, view>
class DataStream {
public:
    //enum class Error {
    //    READ_ERROR = 1,
    //    WRITE_ERROR,
    //    POSITION_ERROR,
    //    FORMAT_ERROR,
    //};

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
    bool CheckOffset(size_t offset) const noexcept {
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

    size_t GetPosition(std::error_condition& ec) const noexcept {
        return this->m_pos;
    }

    size_t Position() const {
        std::error_condition ec;
        auto out = GetPosition(ec);
        if (ec) throw std::length_error("failed to get stream position");
        return out;
    }

    void SetPosition(size_t pos, std::error_condition& ec) noexcept {
        if (CheckPosition(pos)) {
            ec = std::make_error_condition(std::errc::invalid_seek);
        }
        else {
            this->m_pos = pos;
        }
    }

    // TODO rename SetPosition
    // Sets the position of this stream
    void SetPos(size_t pos) {
        std::error_condition ec;
        SetPosition(pos, ec);
        if (ec) throw std::length_error("set position lies outside of stream");
    }

    size_t size(std::error_condition& ec) const noexcept {
        return std::visit(VUtils::Traits::overload{
            [](std::reference_wrapper<BYTES_t> buf) { return buf.get().size(); },
            [](BYTE_VIEW_t buf) { return buf.size(); }
        }, this->m_data);
    }

    size_t size() const {
        std::error_condition ec;
        auto out = size(ec);
        if (ec) throw std::length_error("failed to retrieve stream size");
        return out;
    }

    BYTE_t* data(std::error_condition& ec) noexcept {
        return std::visit(VUtils::Traits::overload{
            [](std::reference_wrapper<BYTES_t> buf) { return buf.get().data(); },
            [](BYTE_VIEW_t buf) { return buf.data(); }
        }, this->m_data);
    }

    BYTE_t* data() {
        std::error_condition ec;
        auto out = data(ec);
        if (ec) throw std::length_error("failed to get stream data()");
        return out;
    }

    const BYTE_t* data(std::error_condition& ec) const noexcept {
        return std::visit(VUtils::Traits::overload{
            [](std::reference_wrapper<BYTES_t> buf) { return buf.get().data(); },
            [](BYTE_VIEW_t buf) { return buf.data(); }
            }, this->m_data);
    }

    const BYTE_t* data() const {
        std::error_condition ec;
        auto out = data(ec);
        if (ec) throw std::length_error("failed to get stream data()");
        return out;
    }

    void extend(size_t count) {
        std::visit(VUtils::Traits::overload{
            [this, count](std::reference_wrapper<BYTES_t> buf) {
                if (this->CheckOffset(count))
                    buf.get().resize(this->m_pos + count);
            },
            [this, count](BYTE_VIEW_t buf) { this->AssertOffset(count); }
        }, this->m_data);
    }



    void Skip(size_t count, std::error_condition& ec) noexcept {
        auto position = this->GetPosition(ec);
        if (!ec) this->SetPosition(position + count, ec);
    }

    // Skip forward x bytes
    //  Assumes the bytes already exist
    void Skip(size_t count) {
        std::error_condition ec;
        Skip(count, ec);
        if (ec) throw std::length_error("failed to skip");
    }

    // Move forward x bytes (assumes bytes do/dont exist).
    //  Should be able to 'create' bytes if they do not exist
    void Advance(size_t count) {
        extend(count);
        Skip(count);
    }
};

// https://akrzemi1.wordpress.com/2017/08/12/your-own-error-condition/
//template <> struct std::is_error_condition_enum<DataStream::Error> : true_type {};
