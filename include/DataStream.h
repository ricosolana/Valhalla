#pragma once

#include <variant>
#include <fstream>
#ifdef ESP_PLATFORM
//#include "esp_vfs_fat.h"
//#include "sdmmc_cmd.h"
//#include "driver/sdmmc_host.h"
#endif
#include "VUtils.h"

class DataStream {
public:
    struct ScopedFile {
        std::FILE* m_file;
        
        //ScopedFile() : m_file(nullptr) {}

        ScopedFile(const char* name, const char* mode) {
            m_file = std::fopen(name, mode);
            if (!m_file)
                throw std::runtime_error("failed to open scoped file");;
        }

        ScopedFile(const ScopedFile&) = delete;
        ScopedFile(ScopedFile&& other) {
            this->m_file = other.m_file;
            other.m_file = nullptr;
        }

        void operator=(ScopedFile&& other) {
            Close();
            this->m_file = other.m_file;
            other.m_file = nullptr;
        }

        operator FILE* () {
            return this->m_file;
        }

        operator FILE* () const {
            return this->m_file;
        }

        void Close() {
            if (this->m_file) {
                std::fclose(this->m_file);
                this->m_file = nullptr;
            }
        }

        ~ScopedFile() {
            if (m_file) {
                if (std::fclose(m_file) != 0) {
                    //exit(0);
                }
            }
        }
    };

    enum class Type : uint8_t {
        READ,
        WRITE
    };

    std::variant<
        std::pair<std::reference_wrapper<BYTES_t>, size_t>, 
        std::pair<BYTE_VIEW_t, size_t>, 
        //std::pair<std::FILE*, Type>
        ScopedFile
    > m_data;

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
    explicit DataStream(BYTE_VIEW_t buf) : m_data(std::make_pair(buf, 0ULL)) {}
    explicit DataStream(BYTES_t& buf) : m_data(std::pair(std::ref(buf), 0ULL)) {}
    explicit DataStream(ScopedFile file) : m_data(std::move(file)) {}

public:
    size_t Position() const {
        return std::visit(VUtils::Traits::overload{
            [](const std::pair<std::reference_wrapper<BYTES_t>, size_t> &pair) -> size_t { return pair.second; },
            [](const std::pair<BYTE_VIEW_t, size_t> &pair) -> size_t { return pair.second; },
            [](const ScopedFile &file) -> size_t {
                auto pos = ftell(file);
                if (pos == -1)
                    throw std::runtime_error("failed to get file pos");

                return (size_t) pos;
            }
        }, this->m_data);
    }

    // Sets the position of this stream
    void SetPos(size_t newpos) {
        //AssertPosition(pos);

        std::visit(VUtils::Traits::overload{
            [&](std::pair<std::reference_wrapper<BYTES_t>, size_t>& pair) { 
                auto&& buf = pair.first.get();
                auto&& pos = pair.second;

                if (newpos > buf.size())
                    throw std::runtime_error("position exceeds vector bounds");
                pos = newpos; 
            },
            [&](std::pair<BYTE_VIEW_t, size_t>& pair) { 
                auto&& buf = pair.first;
                auto&& pos = pair.second;

                if (newpos > buf.size())
                    throw std::runtime_error("position exceeds array bounds");
                pos = newpos; 
            },
            [&](ScopedFile &file) {
                if (std::fseek(file, newpos, SEEK_SET) != 0)
                    throw std::runtime_error("failed to fseek to pos");
            }
        }, this->m_data);
    }

    size_t size() const {
        return std::visit(VUtils::Traits::overload{
            [](const std::pair<std::reference_wrapper<BYTES_t>, size_t>& pair) { return pair.first.get().size(); },
            [](const std::pair<BYTE_VIEW_t, size_t>& pair) { return pair.first.size(); },
            [](const ScopedFile& file) { 
                auto prev = ftell(file); 
                if (prev == -1)
                    throw std::runtime_error("failed to set file pos (0)");
                if (std::fseek(file, 0, SEEK_END) != 0)
                    throw std::runtime_error("failed to set file pos (1)");

                auto s = (size_t) ftell(file); 
                if (s == -1)
                    throw std::runtime_error("failed to set file pos (2)");
                if (fseek(file, prev, SEEK_SET) != 0);
                    throw std::runtime_error("failed to set file pos (3)");
                
                return s; 
            }
        }, this->m_data);
    }

    // Skip ahead x bytes
    void Skip(size_t offset) {

        /*
        std::visit(VUtils::Traits::overload{
            [](std::pair<std::reference_wrapper<BYTES_t>, size_t>& pair) { return pair.first.get().size(); },
            [](std::pair<BYTE_VIEW_t, size_t>& pair) { return pair.first.size(); },
            [](std::pair<std::FILE*, Type*>& pair) { 
                auto&& file = pair.first;

                auto prev = ftell(file); 
                if (prev == -1)
                    throw std::runtime_error("failed to set file pos (0)");
                if (std::fseek(file, 0, SEEK_END) != 0)
                    throw std::runtime_error("failed to set file pos (1)");

                auto s = (size_t) ftell(file); 
                if (s == -1)
                    throw std::runtime_error("failed to set file pos (2)")
                if (fseek(file, prev, SEEK_SET) != 0);
                    throw std::runtime_error("failed to set file pos (3)")
                
                return s; 
            }
        }, this->m_data);*/

        // This is slow, two visits are performed 
        // not really slow just redundant, and 2 potential file seeks are done (which is almost guaranteed to be slow)
        this->SetPos(this->Position() + offset);
    }
};
