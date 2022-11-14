#include <iostream>
#include <sstream>
#include <fstream>
#include <robin_hood.h>

#include "VUtilsResource.h"

namespace VUtils::Resource {
    // Instantiate static variables
    fs::path root;

    void SetRoot(const std::string &r) {
        root = r;
    }

    fs::path GetPath(const std::string &path) {
        return root / path;
    }

    //std::ifstream GetInFile(const fs::path &path, std::ios) {
    //    return std::ifstream(GetPath(path), std::ios::nbinary);
    //}
    //
    //std::ofstream GetOutFile(const fs::path& path) {
    //    return std::ofstream(GetPath(path), std::ios::binary);
    //}

    bool ReadFileBytes(const std::string& path, BYTE_t* buf, int size) {
        auto filesize = std::filesystem::file_size(GetPath(path));
        if (filesize > size)
            return false;

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

    bool ReadFileBytes(const std::string &path, BYTES_t &vec) {
        auto file = std::ifstream(GetPath(path), std::ios::binary);
        
        // Stop eating new lines in binary mode!!!
        file.unsetf(std::ios::skipws);

        if (!file)
            return false;

        vec.clear();

        // read the data:
        vec.insert(vec.begin(),
            std::istream_iterator<BYTE_t>(file),
            std::istream_iterator<BYTE_t>());

        return true;
    }

    bool ReadFileBytes(const std::string& path, std::string& str) {
        //auto file = GetInFile(path);
        auto file = std::ifstream(GetPath(path), std::ios::binary);

        // Stop eating new lines in binary mode!!!
        // im not sure whether this is redundant
        file.unsetf(std::ios::skipws);

        if (!file)
            return false;

        str.clear();

        // read the data:
        str.insert(str.begin(),
            std::istream_iterator<BYTE_t>(file),
            std::istream_iterator<BYTE_t>());

        return true;
    }

    bool ReadFileLines(const std::string& path, std::vector<std::string>& out) {
        //auto file = GetInFile(path);
        auto file = std::ifstream(GetPath(path));

        if (!file)
            return false;

        out.clear();

        std::string line;
        while (std::getline(file, line)) {
            out.push_back(line);
        }

        return true;
    }



    bool WriteFileBytes(const std::string& path, const BYTE_t* buf, int size) {
        //auto file = GetOutFile(path);
        auto file = std::ofstream(GetPath(path), std::ios::binary);

        if (!file)
            return false;

        std::copy(buf, buf + size, 
            std::ostream_iterator<BYTE_t>(file));

        file.close();

        return true;
    }

    bool WriteFileBytes(const std::string& path, const BYTES_t& vec) {
        //auto file = GetOutFile(path);
        auto file = std::ofstream(GetPath(path), std::ios::binary);

        if (!file)
            return false;

        std::copy(vec.begin(), vec.end(), 
            std::ostream_iterator<BYTE_t>(file));

        file.close();

        return true;
    }

    bool WriteFileBytes(const std::string& path, const std::string& str) {
        //auto file = GetOutFile(path);
        auto file = std::ofstream(GetPath(path), std::ios::binary);

        if (!file)
            return false;

        std::copy(str.begin(), str.end(), 
            std::ostream_iterator<BYTE_t>(file));

        file.close();

        return true;
    }

    bool WriteFileLines(const std::string& path, const std::vector<std::string>& in) {
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
