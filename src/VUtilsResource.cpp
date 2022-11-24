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

    std::ifstream GetInFile(const std::string &path) {
        return std::ifstream(GetPath(path), std::ios::binary);
    }
    
    std::ofstream GetOutFile(const std::string& path) {
        return std::ofstream(GetPath(path), std::ios::binary);
    }

    std::optional<BYTES_t> ReadFileBytes(const std::string &path) {
        auto file = GetInFile(path);

        if (!file)
            return std::nullopt;

        // https://www.reddit.com/r/cpp_questions/comments/m93tjb/comment/grkst7r/?utm_source=share&utm_medium=web2x&context=3

        return BYTES_t(
            std::istreambuf_iterator<BYTE_t>(file),
            std::istreambuf_iterator<BYTE_t>());
    }

    std::optional<std::string> ReadFileString(const std::string& path) {
        auto file = GetInFile(path);

        if (!file)
            return std::nullopt;

        return std::string(
            std::istreambuf_iterator<BYTE_t>(file),
            std::istreambuf_iterator<BYTE_t>());
    }

    std::optional<std::vector<std::string>> ReadFileLines(const std::string& path) {
        auto file = GetInFile(path);

        if (!file)
            return std::nullopt;

        std::vector<std::string> out;

        std::string line;
        while (std::getline(file, line)) {
            out.push_back(line);
        }

        return out;
    }



    bool WriteFileBytes(const std::string& path, const BYTE_t* buf, int size) {
        auto file = GetOutFile(path);

        if (!file)
            return false;

        std::copy(buf, buf + size, 
            std::ostream_iterator<BYTE_t>(file));

        file.close();

        return true;
    }

    bool WriteFileBytes(const std::string& path, const BYTES_t& vec) {
        auto file = GetOutFile(path);

        if (!file)
            return false;

        std::copy(vec.begin(), vec.end(), 
            std::ostream_iterator<BYTE_t>(file));

        file.close();

        return true;
    }

    bool WriteFileString(const std::string& path, const std::string& str) {
        auto file = GetOutFile(path);

        if (!file)
            return false;

        std::copy(str.begin(), str.end(), 
            std::ostream_iterator<BYTE_t>(file));

        file.close();

        return true;
    }

    bool WriteFileLines(const std::string& path, const std::vector<std::string>& in) {
        auto file = GetOutFile(path);

        if (!file)
            return false;

        for (auto &&str : in) {
            file << str << "\n";
        }

        file.close();

        return true;
    }

}
