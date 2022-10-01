#pragma once

#include <map>
#include <string>

#include <vector>
#include <filesystem>
#include <Utils.h>

namespace fs = std::filesystem;

namespace ResourceManager {

    // Set the data root directory
    void SetRoot(const fs::path &root);

    // Returns the combined root directory with a path
    fs::path GetPath(const fs::path &path);

    bool ReadFileBytes(const fs::path &path, std::vector<byte_t> &buffer);
    bool ReadFileBytes(const fs::path& path, std::string &str);
};
