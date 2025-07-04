#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <type_traits>
#include <vector>

#include "VUtilsString.h"
#include "VUtilsTraits.h"

namespace VUtils::Resource {
    // Read a file into a buffer object
    //  Buffer can be a byte vector, string, or
    //  other (preferably) contiguous data structure
    template<typename Buffer = avledet::util::Bytes>
        requires std::is_arithmetic_v<typename Buffer::value_type>
    std::optional<Buffer> ReadFile(std::filesystem::path const &path)
    {
        std::ifstream file(path, std::ios::binary);

        if (!file)
            return std::nullopt;

        file.unsetf(std::ios::skipws);

        file.seekg(0, std::ios::end);
        auto fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        Buffer result {};
        result.resize(fileSize);
        file.read(reinterpret_cast<std::ifstream::char_type *>(result.data()), fileSize);

        return result;
    }

    // Read a file into separate lines
    //  Iterable can be any container type consisting of any buffer object
    template<typename Iterable = std::vector<std::string>>
        requires(VUtils::Traits::is_iterable<Iterable>
                 && !std::is_same_v<typename Iterable::value_type, std::string_view>
                 && VUtils::Traits::is_iterable<typename Iterable::value_type>)
    std::optional<Iterable> ReadFile(std::filesystem::path const &path, bool includeBlanks = false)
    {
        auto opt = ReadFile<std::string>(path);
        if (!opt)
            return std::nullopt;

        return avledet::lexicon::template split<Iterable>(opt.value(), '\n', includeBlanks);
    }

    // Read a file into separate lines
    //  Iterable can be any container type consisting of any buffer object
    //  This method is the most preferred over the Iterable<string> method
    template<typename Iterable = std::vector<std::string_view>>
        requires(VUtils::Traits::is_iterable<Iterable>)
    std::optional<Iterable> ReadFile(std::filesystem::path const &path, std::string &out,
                                     bool includeBlanks = false)
    {
        {
            auto opt = ReadFile<std::string>(path);
            if (!opt)
                return std::nullopt;

            out = std::move(opt.value());
        }

        return avledet::lexicon::template split<Iterable>(out, '\n', includeBlanks);
    }

    bool WriteFile(std::filesystem::path const &path, avledet::util::Byte const *buf, std::size_t size);
    bool WriteFile(std::filesystem::path const &path, avledet::util::Bytes const &buffer);
    bool WriteFile(std::filesystem::path const &path, std::string_view str);

    // Write a Container<std::string> as lines to a file
    template<typename Iterable>
        requires(VUtils::Traits::is_iterable<Iterable>
                 && std::is_same_v<typename Iterable::value_type, std::string>)
    bool WriteFile(std::filesystem::path const &path, Iterable const &in)
    {
        std::ofstream file(path, std::ios::binary);

        if (!file)
            return false;

        for (auto &&str : in) {
            file << str << "\n";
        }

        file.close();

        return true;
    }

};// namespace VUtils::Resource
