#pragma once

#include <map>
#include <string>
#include <vector>
#include <filesystem>

#include "VUtils.h"

namespace VUtils::Resource {

    // Set the data root directory
    void SetRoot(const std::string &root);

    // Returns the combined root directory with a path
    fs::path GetPath(const std::string &path);

    std::ifstream GetInFile(const std::string& path);
    std::ofstream GetOutFile(const std::string& path);

    std::optional<BYTES_t> ReadFileBytes(const std::string& path);
    std::optional<std::string> ReadFileString(const std::string& path);
    std::optional<std::vector<std::string>> ReadFileLines(const std::string& path);

    bool WriteFileBytes(const std::string& path, const BYTE_t* buf, int size);
    bool WriteFileBytes(const std::string& path, const BYTES_t& buffer);
    bool WriteFileString(const std::string& path, const std::string& str);
    bool WriteFileLines(const std::string& path, const std::vector<std::string>& in);
};
