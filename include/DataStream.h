#pragma once

#include <stdio.h>

#include "VUtils.h"

class SharedFile {
private:
    std::FILE* m_file;
    bool m_isRead;
    bool m_isOwned;
    //uint32_t m_size;

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

    SharedFile(const SharedFile& other) {
        this->m_file = other.m_file;
        this->m_isOwned = false; // important to avoid UB
        this->m_isRead = other.m_isRead;
    }

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
        if (!this->m_file)
            throw std::runtime_error("file was unable to be opened");

        return this->m_file;
    }
};

// 56 Bytes w/ <vec, view>
// 32 Bytes w/ <vec&, view>
class DataStream {
public:
    // Variant data
    std::variant<
        std::pair<std::reference_wrapper<BYTES_t>, size_t>, 
        std::pair<BYTE_SPAN_t, size_t>, 
        SharedFile
    > m_data;

protected:
    explicit DataStream(BYTE_SPAN_t buf) : m_data(std::pair(buf, 0)) {}
    explicit DataStream(BYTES_t& buf) : m_data(std::pair(std::ref(buf), 0)) {}
    explicit DataStream(SharedFile file) : m_data(std::move(file)) {}

public:
    //bool owned() const { return std::get_if< !this->m_data.data(); }

    size_t Position() {
        return std::visit(VUtils::Traits::overload{
            [](const std::pair<std::reference_wrapper<BYTES_t>, size_t>& pair) -> size_t { return pair.second; },
            [](const std::pair<BYTE_SPAN_t, size_t>& pair) -> size_t { return pair.second; },
            [](SharedFile& file) -> size_t {
                auto pos = std::ftell(file);
                if (pos == -1)
                    throw std::runtime_error("failed to get file pos");

                return (size_t) pos;
            }
        }, this->m_data);
    }

    // TODO rename SetPosition
    // Sets the position of this stream
    void SetPosition(size_t pos) {
        return std::visit(VUtils::Traits::overload{
            [&](std::pair<std::reference_wrapper<BYTES_t>, size_t>& pair) { 
                auto&& buf = pair.first.get();

                if (pos > buf.size())
                    throw std::runtime_error("position exceeds vector bounds");

                pair.second = pos;
            },
            [&](std::pair<BYTE_SPAN_t, size_t>& pair) {
                auto&& buf = pair.first;

                if (pos > buf.size())
                    throw std::runtime_error("position exceeds span bounds");
                
                pair.second = pos;
            },
            [&](SharedFile& file) {
                if (std::fseek(file, pos, SEEK_SET) != 0)
                    throw std::runtime_error("failed to fseek to pos");
            }
        }, this->m_data);
    }

    // TODO must const qualify this
    size_t size() const {
        return std::visit(VUtils::Traits::overload{
            [](const std::pair<std::reference_wrapper<BYTES_t>, size_t> &buf) -> size_t { return buf.first.get().size(); },
            [](const std::pair<BYTE_SPAN_t, size_t> &buf) -> size_t { return buf.first.size(); },
            [](const SharedFile &f) -> size_t {
                //fstat()
                // use ifstream or something
                // i hate how fstream is 128+ bytes....
                
                // This should technically be valid
                //  and safe because fp is mutable
                auto&& file = const_cast<SharedFile&>(f);

                auto prev = std::ftell(file);
                if (prev == -1)
                    throw std::runtime_error("failed to get file pos (0)");
                if (std::fseek(file, 0, SEEK_END) != 0)
                    throw std::runtime_error("failed to set file pos (1)");

                auto s = (size_t) std::ftell(file);
                if (s == -1)
                    throw std::runtime_error("failed to reset file pos (2)");
                if (fseek(file, prev, SEEK_SET) != 0)
                    throw std::runtime_error("failed to set file pos (3)");

                return s;
            },
        }, this->m_data);
    }

    BYTE_t* data() {
        return std::visit(VUtils::Traits::overload{
            [](std::pair<std::reference_wrapper<BYTES_t>, size_t>& buf) -> BYTE_t* { return buf.first.get().data(); },
            [](std::pair<BYTE_SPAN_t, size_t>& buf) -> BYTE_t* { return buf.first.data(); },
            [](SharedFile& file) -> BYTE_t* { throw std::runtime_error("cannot call data() on file"); }
        }, this->m_data);
    }

    const BYTE_t* data() const {
        return std::visit(VUtils::Traits::overload{
            [](const std::pair<std::reference_wrapper<BYTES_t>, size_t>& buf) -> const BYTE_t* { return buf.first.get().data(); },
            [](const std::pair<BYTE_SPAN_t, size_t>& buf) -> const BYTE_t* { return buf.first.data(); },
            [](const SharedFile& file) -> const BYTE_t* { throw std::runtime_error("cannot call data() on file"); }
        }, this->m_data);
    }

    // Move forward by x bytes
    //  Does not add bytes
    void Skip(size_t count) {
        // TODO do this in 1 visit to avoid redundant visits
        //std::visit(VUtils::Traits::overload)

        this->SetPosition(this->Position() + count);
    }

    // Move forward by x bytes
    //  Bytes can be allocated
    //void Advance(size_t count) {
    //    Extend(count);
    //    Skip(count);
    //}
};

// https://akrzemi1.wordpress.com/2017/08/12/your-own-error-condition/
//template <> struct std::is_error_condition_enum<DataStream::Error> : true_type {};
