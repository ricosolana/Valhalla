#pragma once

#include <map>
#include <string>
#include <vector>
#include <filesystem>

#include "VUtils.h"

namespace VUtils::Resource {

    // Set the data root directory
    //void SetRoot(const std::string &root);

    // Returns the combined root directory with a path
    //fs::path GetPath(const std::string &path);

    //std::ifstream GetInFile(const fs::path& path);
    //std::ofstream GetOutFile(const fs::path& path);

    std::optional<BYTES_t> ReadFileBytes(const fs::path& path);
    std::optional<std::string> ReadFileString(const fs::path& path);
    std::optional<std::vector<std::string>> ReadFileLines(const fs::path& path);
    
    bool WriteFileBytes(const fs::path& path, const BYTE_t* buf, int size);
    bool WriteFileBytes(const fs::path& path, const BYTES_t& buffer);
    bool WriteFileString(const fs::path& path, const std::string& str);
    bool WriteFileLines(const fs::path& path, const std::vector<std::string>& in);
};
