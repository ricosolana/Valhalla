#pragma once

#include <map>
#include <string>

#include <vector>

namespace ResourceManager {

    void SetRoot(const char* r);

    bool ReadFileBytes(const char* path, std::vector<unsigned char> &buffer);
    bool ReadFileBytes(const char* path, std::string& buffer);
};
