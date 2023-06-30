#pragma once

#include <stdio.h>

#include "VUtils.h"

// 56 Bytes w/ <vec, view>
// 32 Bytes w/ <vec&, view>
class DataStream {
public:
    // Variant data
    std::variant<
        std::pair<std::reference_wrapper<BYTES_t>, size_t>, 
        std::pair<BYTE_SPAN_t, size_t>, 
        std::ifstream,
        std::ofstream
    > m_data;

protected:
    explicit DataStream(BYTE_SPAN_t buf) : m_data(std::pair(buf, 0)) {}
    explicit DataStream(BYTES_t& buf) : m_data(std::pair(std::ref(buf), 0)) {}
    explicit DataStream(std::ifstream file) : m_data(std::move(file)) {
        file.exceptions(std::ifstream::failbit);
        file.unsetf(std::ios::skipws);
    }
    explicit DataStream(std::ofstream file) : m_data(std::move(file)) {
        file.exceptions(std::ofstream::failbit);
        file.unsetf(std::ios::skipws);
    }

public:
    //bool owned() const { return std::get_if< !this->m_data.data(); }

    size_t Position() const {
        return std::visit(VUtils::Traits::overload{
            [](const std::pair<std::reference_wrapper<BYTES_t>, size_t>& pair) -> size_t { return pair.second; },
            [](const std::pair<BYTE_SPAN_t, size_t>& pair) -> size_t { return pair.second; },
            [](const std::ifstream& f) -> size_t {
                auto&& file = const_cast<std::ifstream&>(f);

                file.exceptions(std::ifstream::failbit);
                return file.tellg();
            },
            [](const std::ofstream& f) -> size_t {
                auto&& file = const_cast<std::ofstream&>(f);

                file.exceptions(std::ifstream::failbit);
                return file.tellp();
            },
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
            [&](std::ifstream& file) {
                file.clear();
                file.seekg(pos, std::ios::beg);
            },
            [&](std::ofstream& file) {
                file.clear();
                file.seekp(pos, std::ios::beg);
            },
        }, this->m_data);
    }

    // TODO must const qualify this
    size_t size() const {
        return std::visit(VUtils::Traits::overload{
            [](const std::pair<std::reference_wrapper<BYTES_t>, size_t> &buf) -> size_t { return buf.first.get().size(); },
            [](const std::pair<BYTE_SPAN_t, size_t> &buf) -> size_t { return buf.first.size(); },
            [](const std::ifstream &f) -> size_t {
                auto&& file = const_cast<std::ifstream&>(f);

                file.clear();

                file.seekg(0, std::ios::end);
                auto fileSize = file.tellg();
                file.seekg(0, std::ios::beg);

                return fileSize;
            },
            [](const std::ofstream& f) -> size_t {
                auto&& file = const_cast<std::ofstream&>(f);

                file.clear();

                file.seekp(0, std::ios::end);
                auto fileSize = file.tellp();
                file.seekp(0, std::ios::beg);

                return fileSize;
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
