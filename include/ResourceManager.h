#pragma once

#include <map>
#include <string>
#include <vector>
#include <filesystem>

#include "VUtils.h"

namespace ResourceManager {

    // Set the data root directory
    void SetRoot(const fs::path &root);

    // Returns the combined root directory with a path
    fs::path GetPath(const fs::path &path);

    bool ReadFileBytes(const fs::path& path, BYTE_t* buf, int size);
    bool ReadFileBytes(const fs::path &path, BYTES_t &buffer);
    bool ReadFileBytes(const fs::path& path, std::string &s);
    bool ReadFileLines(const fs::path& path, std::vector<std::string>& out);

    bool WriteFileBytes(const fs::path& path, const BYTE_t* buf, int size);
    bool WriteFileBytes(const fs::path& path, const BYTES_t& buffer);
    bool WriteFileBytes(const fs::path& path, const std::string& str);
    bool WriteFileLines(const fs::path& path, const std::vector<std::string>& in);
};
