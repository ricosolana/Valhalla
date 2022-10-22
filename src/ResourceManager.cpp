#include <iostream>
#include <sstream>
#include <fstream>
#include <robin_hood.h>

#include "ResourceManager.h"

namespace ResourceManager {
    // Instantiate static variables
    fs::path root;

    void SetRoot(const fs::path &r) {
        root = r;
    }

    fs::path GetPath(const fs::path &path) {
        return root / path;
    }

    std::ifstream GetInFile(const fs::path &path) {
        return std::ifstream(GetPath(path), std::ios::binary);
    }
    
    std::ofstream GetOutFile(const fs::path& path) {
        return std::ofstream(GetPath(path), std::ios::binary);
    }

    bool ReadFileBytes(const fs::path& path, byte_t* buf, int size) {
        auto file = GetInFile(path);

        // Stop eating new lines in binary mode!!!
        file.unsetf(std::ios::skipws);

        if (!file)
            return false;

        std::copy(std::istream_iterator<byte_t>(file),
            std::istream_iterator<byte_t>(),
            buf);

        return true;
    }

    bool ReadFileBytes(const fs::path &path, BYTES_t &vec) {
        auto file = GetInFile(path);
        
        // Stop eating new lines in binary mode!!!
        file.unsetf(std::ios::skipws);

        if (!file)
            return false;

        // read the data:
        vec.insert(vec.begin(),
            std::istream_iterator<byte_t>(file),
            std::istream_iterator<byte_t>());

        return true;
    }

    bool ReadFileBytes(const fs::path& path, std::string& str) {
        auto file = GetInFile(path);

        // Stop eating new lines in binary mode!!!
        // im not sure whether this is redundant
        file.unsetf(std::ios::skipws);

        if (!file)
            return false;

        // read the data:
        str.insert(str.begin(),
            std::istream_iterator<byte_t>(file),
            std::istream_iterator<byte_t>());

        return true;
    }

    bool WriteFileBytes(const fs::path& path, const byte_t* buf, int size) {
        auto file = GetOutFile(path);

        if (!file)
            return false;

        std::copy(buf, buf + size, std::ostream_iterator<byte_t>(file));

        return true;
    }

    bool WriteFileBytes(const fs::path& path, const BYTES_t& vec) {
        auto file = GetOutFile(path);

        if (!file)
            return false;

        std::copy(vec.begin(), vec.end(), std::ostream_iterator<byte_t>(file));

        return true;
    }

    bool WriteFileBytes(const fs::path& path, const std::string& str) {
        auto file = GetOutFile(path);

        if (!file)
            return false;

        std::copy(str.begin(), str.end(), std::ostream_iterator<byte_t>(file));

        return true;
    }

}
