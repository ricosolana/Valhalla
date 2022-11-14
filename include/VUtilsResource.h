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

    bool ReadFileBytes(const std::string& path, BYTE_t* buf, int size);
    bool ReadFileBytes(const std::string& path, BYTES_t &buffer);
    bool ReadFileBytes(const std::string& path, std::string &s);
    bool ReadFileLines(const std::string& path, std::vector<std::string>& out);

    bool WriteFileBytes(const std::string& path, const BYTE_t* buf, int size);
    bool WriteFileBytes(const std::string& path, const BYTES_t& buffer);
    bool WriteFileBytes(const std::string& path, const std::string& str);
    bool WriteFileLines(const std::string& path, const std::vector<std::string>& in);
};
