#pragma once

#include <map>
#include <string>

#include <vector>
#include <filesystem>
#include "Utils.h"

namespace fs = std::filesystem;

namespace ResourceManager {

    // Set the data root directory
    void SetRoot(const fs::path &root);

    // Returns the combined root directory with a path
    fs::path GetPath(const fs::path &path);

    bool ReadFileBytes(const fs::path& path, byte_t* buf, int size);
    bool ReadFileBytes(const fs::path &path, bytes_t &buffer);
    bool ReadFileBytes(const fs::path& path, std::string &s);

    bool WriteFileBytes(const fs::path& path, const byte_t* buf, int size);
    bool WriteFileBytes(const fs::path& path, const bytes_t& buffer);
    bool WriteFileBytes(const fs::path& path, const std::string &s);
};