#pragma once

#include <map>
#include <string>
#include <vector>
#include <filesystem>

#include "VUtils.h"
#include "VUtilsTraits.h"
#include "VUtilsString.h"

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


    // Read a file into a buffer object
    //  Buffer can be a byte vector, string, or 
    //  other (preferably) contiguous data structure
    template<typename Buffer = BYTES_t>
    std::optional<Buffer> ReadFile(const std::filesystem::path& path) {
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

        file.seekg(0, std::ios::end);
        auto fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        //VLOG(1) << "Reading file " << path << " (" << fileSize << " bytes)";

        Buffer result {};
        result.resize(fileSize);
        file.read(reinterpret_cast<std::ifstream::char_type*>(result.data()),
            fileSize);

        return result;
    }



    // Read a file into separate lines
    //  Iterable can be any container type consisting of any buffer object
    template<typename Iterable = std::vector<std::string>> requires
        (VUtils::Traits::is_iterable_v<Iterable>
            && !std::is_same_v<typename Iterable::value_type, std::string_view>
            && VUtils::Traits::is_iterable_v<typename Iterable::value_type>)
        std::optional<Iterable> ReadFile(const std::filesystem::path& path, bool includeBlanks = false)
    {
        auto opt = ReadFile<std::string>(path);
        if (!opt)
            return std::nullopt;
        
        return VUtils::String::template Split<Iterable>(opt.value(), '\n', includeBlanks);

        /*
        auto size = opt.value().size();
        auto data = opt.value().data();

        Iterable lines{};

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

        return lines;*/
    }

    // Read a file into separate lines
    //  Iterable can be any container type consisting of any buffer object
    //  This method is the most preferred over the Iterable<string> method
    template<typename Iterable = std::vector<std::string_view>> requires
        (VUtils::Traits::is_iterable_v<Iterable>)
    std::optional<Iterable> ReadFile(const std::filesystem::path& path, std::string& out, bool includeBlanks = false) {
        {
            auto opt = ReadFile<std::string>(path);
            if (!opt)
                return std::nullopt;

            out = std::move(opt.value());
        }

        return VUtils::String::template Split<Iterable>(out, '\n', includeBlanks);
    }

    /*
    // Map overload to read in pairs
    template<typename Iterable> requires
        (VUtils::Traits::is_iterable_v<Iterable>
            && VUtils::Traits::has_key_type_v<Iterable>)
        std::optional<Iterable> ReadFile(const fs::path& path) 
    {
        auto opt = ReadFile<std::string>(path);
        if (opt) {
            Iterable result{};
            for 

            //out = std::move(opt.value());




            return VUtils::String::template Split<Iterable>(out, '\n', includeBlanks);
        }
    }*/



        
    bool WriteFile(const std::filesystem::path& path, const BYTE_t* buf, size_t size);
    bool WriteFile(const std::filesystem::path& path, const BYTES_t& buffer);
    bool WriteFile(const std::filesystem::path& path, std::string_view str);

    // Write a Container<std::string> as lines to a file
    template<typename Iterable> requires 
        (VUtils::Traits::is_iterable_v<Iterable> 
            && std::is_same_v<typename Iterable::value_type, std::string>)
    bool WriteFile(const std::filesystem::path& path, const Iterable& in) {
        std::ofstream file(path, std::ios::binary);

        if (!file)
            return false;

        for (auto&& str : in) {
            file << str << "\n";
        }

        /*
#ifndef ELPP_DISABLE_VERBOSE_LOGS
        if (el::Loggers::verboseLevel() == 1) {
            file.seekp(0, std::ios::end);
            auto fileSize = file.tellp();
            file.seekp(0, std::ios::beg);

            //VLOG(1) << "Writing txt file " << path << " (" << fileSize << " bytes)";
        }
#endif*/

        file.close();

        return true;
    }
};
