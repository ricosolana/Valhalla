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

    //std::ifstream GetInFile(const fs::path &path, std::ios) {
    //    return std::ifstream(GetPath(path), std::ios::nbinary);
    //}
    //
    //std::ofstream GetOutFile(const fs::path& path) {
    //    return std::ofstream(GetPath(path), std::ios::binary);
    //}

    bool ReadFileBytes(const fs::path& path, BYTE_t* buf, int size) {
        //auto file = GetInFile(path);
        auto file = std::ifstream(GetPath(path), std::ios::binary);

        // Stop eating new lines in binary mode!!!
        file.unsetf(std::ios::skipws);

        if (!file)
            return false;

        std::copy(std::istream_iterator<BYTE_t>(file),
            std::istream_iterator<BYTE_t>(),
            buf);

        return true;
    }

    bool ReadFileBytes(const fs::path &path, BYTES_t &vec) {
        //auto file = GetInFile(path);
        auto file = std::ifstream(GetPath(path), std::ios::binary);
        
        // Stop eating new lines in binary mode!!!
        file.unsetf(std::ios::skipws);

        if (!file)
            return false;

        // read the data:
        vec.insert(vec.begin(),
            std::istream_iterator<BYTE_t>(file),
            std::istream_iterator<BYTE_t>());

        return true;
    }

    bool ReadFileBytes(const fs::path& path, std::string& str) {
        //auto file = GetInFile(path);
        auto file = std::ifstream(GetPath(path), std::ios::binary);

        // Stop eating new lines in binary mode!!!
        // im not sure whether this is redundant
        file.unsetf(std::ios::skipws);

        if (!file)
            return false;

        // read the data:
        str.insert(str.begin(),
            std::istream_iterator<BYTE_t>(file),
            std::istream_iterator<BYTE_t>());

        return true;
    }

    bool ReadFileLines(const fs::path& path, std::vector<std::string>& out) {
        //auto file = GetInFile(path);
        auto file = std::ifstream(GetPath(path));

        if (!file)
            return false;

        std::string line;
        while (std::getline(file, line)) {
            out.push_back(std::move(line));
        }

        return true;
    }



    bool WriteFileBytes(const fs::path& path, const BYTE_t* buf, int size) {
        //auto file = GetOutFile(path);
        auto file = std::ofstream(GetPath(path), std::ios::binary);

        if (!file)
            return false;

        std::copy(buf, buf + size, std::ostream_iterator<BYTE_t>(file));

        file.close();

        return true;
    }

    bool WriteFileBytes(const fs::path& path, const BYTES_t& vec) {
        //auto file = GetOutFile(path);
        auto file = std::ofstream(GetPath(path), std::ios::binary);

        if (!file)
            return false;

        std::copy(vec.begin(), vec.end(), std::ostream_iterator<BYTE_t>(file));

        file.close();

        return true;
    }

    bool WriteFileBytes(const fs::path& path, const std::string& str) {
        //auto file = GetOutFile(path);
        auto file = std::ofstream(GetPath(path), std::ios::binary);

        if (!file)
            return false;

        std::copy(str.begin(), str.end(), std::ostream_iterator<BYTE_t>(file));

        file.close();

        return true;
    }

    bool WriteFileLines(const fs::path& path, const std::vector<std::string>& in) {
        auto file = std::ofstream(GetPath(path));

        if (!file)
            return false;

        for (auto &&str : in) {
            file << str << "\n";
        }

        file.close();

        return true;
    }

}
