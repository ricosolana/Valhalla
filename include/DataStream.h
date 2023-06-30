#pragma once

#include <stdio.h>

#include "VUtils.h"

class SharedFile {
private:
    std::FILE* m_file;
    bool m_isRead;
    bool m_isOwned;

public:
    SharedFile(const char* path, bool read) {
        this->m_file = std::fopen(path, read ? "rb" : "wb");
        //if (!this->m_file)
            //throw std::runtime_error("unable to open file");

        this->m_isRead = read;
        this->m_isOwned = true;
    }
    
    // how to handle shared cases
    SharedFile(FILE* file, bool read) {
        this->m_file = file;
        //if (!this->m_file)
            //throw std::runtime_error("unable to open file");

        this->m_isRead = read;
        this->m_isOwned = false;
    }

    ~SharedFile() {
        Close();
    }

    SharedFile(const SharedFile&) = delete;
    SharedFile(SharedFile&& other) {
        *this = std::move(other);
    }

    void operator=(const SharedFile&) = delete;

    void operator=(SharedFile&& other) {
        this->m_file = other.m_file;
        other.m_file = nullptr;
    }

    bool Close() {
        if (this->m_file && this->m_isOwned) {
            bool status = std::fclose(this->m_file) == 0;
            this->m_file = nullptr;
            return status;
        }
        return false;
    }

    operator std::FILE* () {
        return this->m_file;
    }
};

// 56 Bytes w/ <vec, view>
// 32 Bytes w/ <vec&, view>
class DataStream {
public:
    // Proposal for reading from file (with owned and unowned file variant subtypes)
    std::variant<
        std::pair<std::reference_wrapper<BYTES_t>, size_t>, 
        std::pair<BYTE_VIEW_t, size_t>, 
        SharedFile
    > m_data;

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

protected:
    explicit DataStream(BYTE_SPAN_t buf) : m_data(std::pair(buf, 0)) {}
    explicit DataStream(BYTES_t& buf) : m_data(std::pair(std::ref(buf), 0)) {}
    explicit DataStream(SharedFile file) : m_data(std::move(file)) {}

public:
    //bool owned() const { return std::get_if< !this->m_data.data(); }

    size_t Position() const {
        return this->m_pos;
    }

    // TODO rename SetPosition
    // Sets the position of this stream
    void SetPosition(size_t pos) {
        AssertPosition(pos);
        this->m_pos = pos;
    }

    size_t size() const {
        return std::visit(VUtils::Traits::overload{
            [](const std::pair<std::reference_wrapper<BYTES_t>, size_t> &buf) { return buf.first.get().size(); },
            [](const std::pair<BYTE_SPAN_t, size_t> &buf) { return buf.first.size(); },
            [](SharedFile file) {
                auto prev = std::ftell(file);
                if (prev == -1)
                    throw std::runtime_error("failed to set file pos (0)");
                if (std::fseek(file, 0, SEEK_END) != 0)
                    throw std::runtime_error("failed to set file pos (1)");

                auto s = (size_t)std::ftell(file);
                if (s == -1)
                    throw std::runtime_error("failed to set file pos (2)");
                if (fseek(file, prev, SEEK_SET) != 0);
                    throw std::runtime_error("failed to set file pos (3)");

                return s;
            },
        }, this->m_data);
    }

    BYTE_t* data() {
        return std::visit(VUtils::Traits::overload{
            [](std::pair<std::reference_wrapper<BYTES_t>, size_t>& buf) { return buf.first.get().data(); },
            [](std::pair<BYTE_SPAN_t, size_t>& buf) { return buf.first.data(); },
            [](SharedFile& file) { throw std::runtime_error("cannot call data() on file"); }
        }, this->m_data);
    }

    const BYTE_t* data() const {
        return std::visit(VUtils::Traits::overload{
            [](const std::pair<std::reference_wrapper<BYTES_t>, size_t>& buf) { return buf.first.get().data(); },
            [](const std::pair<BYTE_SPAN_t, size_t>& buf) { return buf.first.data(); },
            [](const SharedFile& file) { throw std::runtime_error("cannot call data() on file"); }
        }, this->m_data);
    }

    // Increases the size of this stream by x bytes
    //  This is only really useful for vector, nothing else though
    void Extend(size_t count) {
        std::get<

        std::visit(VUtils::Traits::overload{
            [this, count](std::pair<std::reference_wrapper<BYTES_t>, size_t>& buf) {
                if (this->CheckOffset(count))
                    buf.first.get().resize(this->m_pos + count);
            },
            [this, count](std::pair<BYTE_SPAN_t, size_t>& buf) { this->AssertOffset(count); },
            [this, count](SharedFile& file) {  }
        }, this->m_data);
    }


    // Move forward by x bytes
    //  Does not add bytes
    void Skip(size_t count) {
        this->SetPosition(this->Position() + count);
    }

    // Move forward by x bytes
    //  Bytes can be allocated
    void Advance(size_t count) {
        Extend(count);
        Skip(count);
    }
};

// https://akrzemi1.wordpress.com/2017/08/12/your-own-error-condition/
//template <> struct std::is_error_condition_enum<DataStream::Error> : true_type {};
