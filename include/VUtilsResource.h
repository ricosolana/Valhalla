#pragma once

#include <map>
#include <string>
#include <vector>
#include <filesystem>

#include "VUtils.h"
#include "VUtilsTraits.h"

namespace VUtils::Resource {

    /*
    class ScopedFile {
    private:
        FILE* m_file;

    public:
        ScopedFile(FILE* file)
            : m_file(file) {}

        ~ScopedFile() {
            if (m_file)
                fclose(m_file);
        }

        ScopedFile(const ScopedFile& other) = delete;

        operator FILE* () {
            return m_file;
        }
    };*/



    template<typename T = BYTES_t>
    std::optional<T> ReadFile(const fs::path& path) {
       //ScopedFile file(fopen(path.string().c_str(), "rb"));
       //
       //if (!file) return std::nullopt;
       //
       //if (!fseek(file, 0, SEEK_END)) return std::nullopt;
       //
       //auto size = ftell(file);
       //if (size == -1) return std::nullopt;
       //
       //if (!fseek(file, 0, SEEK_SET)) return std::nullopt;
       //
       //T result{};
       //
       //// somehow avoid the zero-initialization
       //result.resize(size);
       //
       //fread(result.data(), 1, size, file);
       //
       //return result;

        std::ifstream file(path, std::ios::binary);

        if (!file)
            return std::nullopt;

        file.unsetf(std::ios::skipws);

        std::streampos fileSize;

        file.seekg(0, std::ios::end);
        fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        T result {};
        result.resize(fileSize);
        file.read(reinterpret_cast<std::ifstream::char_type*>(&result.front()),
            fileSize);

        return result;
    }

    // TODO readlines methods are both similar to each other
    //  try to condense them together into 1 single  method, somehow or another
    template<typename Iterable = std::vector<std::string>> requires
        (VUtils::Traits::is_iterable_v<Iterable>
            && std::is_same_v<typename Iterable::value_type, std::string>)
        std::optional<Iterable> ReadFileLines(const fs::path& path, bool includeBlanks = false) {
        auto opt = ReadFile<std::string>(path);
        if (!opt)
            return std::nullopt;

        auto size = opt.value().size();
        auto data = opt.value().data();

        Iterable lines;

        int lineIdx = -1;
        int lineSize = 0;
        for (int i = 0; i < size; i++) {
            lineSize = i - lineIdx - 1;

            if (data[i] == '\n') {
                if (lineSize || includeBlanks) {
                    lines.insert(lines.end(), Iterable::value_type(data + lineIdx + 1, lineSize));
                }
                lineIdx = i;
            }
        }

        // this includes last line ONLY if it is not blank (has at least 1 character)
        if (lineIdx < size - 1) {
            lines.insert(lines.end(), Iterable::value_type(data + lineIdx + 1, size - lineIdx - 1));
        }

        return lines;
    }

    template<typename Iterable = std::vector<std::string_view>> requires
        (VUtils::Traits::is_iterable_v<Iterable>
            && std::is_same_v<typename Iterable::value_type, std::string_view>)
    std::optional<Iterable> ReadFileLines(const fs::path& path, std::string& out, bool includeBlanks = false) {
        auto opt = ReadFile<std::string>(path);
        if (!opt)
            return std::nullopt;
        
        out = std::move(opt.value());
        auto size = out.size();
        auto data = out.data();

        Iterable lines;

        int lineIdx = -1;
        int lineSize = 0;
        for (int i = 0; i < size; i++) {
            lineSize = i - lineIdx - 1;

            if (data[i] == '\n') {
                if (lineSize || includeBlanks) {
                    lines.push_back(Iterable::value_type(data + lineIdx + 1, lineSize));
                }
                lineIdx = i;
            }
        }

        // this includes last line ONLY if it is not blank (has at least 1 character)
        if (lineIdx < size - 1) {
            lines.push_back(Iterable::value_type(data + lineIdx + 1, size - lineIdx - 1));
        }

        return lines;
    }


        
    bool WriteFile(const fs::path& path, const BYTE_t* buf, int size);
    bool WriteFile(const fs::path& path, const BYTES_t& buffer);
    bool WriteFile(const fs::path& path, const std::string& str);

    template<typename Iterable> requires 
        (VUtils::Traits::is_iterable_v<Iterable> 
            && std::is_same_v<typename Iterable::value_type, std::string>)
    bool WriteFileLines(const fs::path& path, const Iterable& in) {
        std::ofstream file(path, std::ios::binary);

        if (!file)
            return false;

        for (auto&& str : in) {
            file << str << "\n";
        }

        file.close();

        return true;
    }
};
