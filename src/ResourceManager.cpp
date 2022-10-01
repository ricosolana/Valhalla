#include "ResourceManager.h"

#include <iostream>
#include <sstream>
#include <fstream>

#include <robin_hood.h>

namespace ResourceManager {
    // Instantiate static variables
    fs::path root;

    void SetRoot(const fs::path &r) {
        root = r;
    }

    fs::path GetPath(const fs::path &path) {
        return root / path;
    }

    std::ifstream GetFile(const fs::path &path) {
        return std::ifstream(GetPath(path), std::ios::binary);
    }

    bool ReadFileBytes(const fs::path &path, std::vector<byte_t> &vec) {
        auto file = GetFile(path);
        
        //file.is_open();
        //file.open();

        // Stop eating new lines in binary mode!!!
        file.unsetf(std::ios::skipws);

        // get its size:
        std::streampos fileSize;

        file.seekg(0, std::ios::end);
        fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // fileSize == -1 when doesnt exists
        if (fileSize < 0)
            return false;

        // reserve capacity
        vec.reserve(fileSize);
        
        //std::cout << "file open1: " << file.is_open() << "\n";

        // read the data:
        vec.insert(vec.begin(),
            std::istream_iterator<byte_t>(file),
            std::istream_iterator<byte_t>());

        //std::cout << "file open2: " << file.is_open() << "\n";

        return true;
    }

    bool ReadFileBytes(const fs::path& path, std::string& str) {
        std::vector<byte_t> buffer;
        if (!ReadFileBytes(path, buffer))
            return false;

        str.reserve(buffer.size());

        str.insert(str.begin(), buffer.begin(), buffer.end());

        return true;
    }

}
