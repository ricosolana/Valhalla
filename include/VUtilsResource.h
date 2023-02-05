#pragma once

#include <map>
#include <string>
#include <vector>
#include <filesystem>

#include "VUtils.h"
#include "VUtilsTraits.h"

namespace VUtils::Resource {

    // Set the data root directory
    //void SetRoot(const std::string &root);

    // Returns the combined root directory with a path
    //fs::path GetPath(const std::string &path);

    //std::ifstream GetInFile(const fs::path& path);
    //std::ofstream GetOutFile(const fs::path& path);

    std::optional<BYTES_t> ReadFileBytes(const fs::path& path);
    std::optional<std::string> ReadFileString(const fs::path& path);
    //std::optional<std::vector<std::string>> ReadFileLines(const fs::path& path);

    template<typename Iterable = std::vector<std::string>> requires
        (VUtils::Traits::is_iterable_v<Iterable>
            && std::is_same_v<typename Iterable::value_type, std::string>)
    std::optional<Iterable> ReadFileLines(const fs::path& path) {
        std::ifstream file(path, std::ios::binary);

        if (!file)
            return std::nullopt;

        Iterable out;

        std::string line;
        while (std::getline(file, line)) {
            out.insert(out.end(), line);
        }

        return out;
    }
    
    bool WriteFileBytes(const fs::path& path, const BYTE_t* buf, int size);
    bool WriteFileBytes(const fs::path& path, const BYTES_t& buffer);
    bool WriteFileString(const fs::path& path, const std::string& str);
    //bool WriteFileLines(const fs::path& path, const std::vector<std::string>& in);

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
