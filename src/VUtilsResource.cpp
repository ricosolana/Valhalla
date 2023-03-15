#include <iostream>
#include <sstream>
#include <fstream>
#include <robin_hood.h>

#include "VUtilsResource.h"

namespace VUtils::Resource {
    // Instantiate static variables
    //fs::path root;

    //void SetRoot(const std::string &r) {
    //    root = r;
    //}
    //
    //fs::path GetPath(const std::string &path) {
    //    return root / path;
    //}

    //std::ifstream GetInFile(const std::string &path) {
    //    return std::ifstream(GetPath(path), std::ios::binary);
    //}
    //
    //std::ofstream GetOutFile(const std::string& path) {
    //    return std::ofstream(GetPath(path), std::ios::binary);
    //}

    // https://stackoverflow.com/questions/15138353/how-to-read-a-binary-file-into-a-vector-of-unsigned-chars
    std::optional<BYTES_t> ReadFileBytes(const fs::path& path) {
        //auto file = GetInFile(path);
        std::ifstream file(path, std::ios::binary);
        
        if (!file)
            return std::nullopt;


        BYTES_t vec;

        file.unsetf(std::ios::skipws);

        std::streampos fileSize;

        file.seekg(0, std::ios::end);
        fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        //vec.resize(fileSize);
        //file.read(reinterpret_cast<std::ifstream::char_type*>(&vec.front()), 
        //    fileSize);

        //return vec;



        vec.reserve(fileSize);

        vec.insert(vec.begin(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        return vec;


        // https://www.reddit.com/r/cpp_questions/comments/m93tjb/comment/grkst7r/?utm_source=share&utm_medium=web2x&context=3

        return BYTES_t(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());
    }

    std::optional<std::string> ReadFileString(const fs::path& path) {
        //auto file = GetInFile(path);
        std::ifstream file(path, std::ios::binary);

        if (!file)
            return std::nullopt;

        return std::string(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());
    }

    /*
    std::optional<std::vector<std::string>> ReadFileLines(const fs::path& path) {
        //auto file = GetInFile(path);
        std::ifstream file(path, std::ios::binary);

        if (!file)
            return std::nullopt;

        std::vector<std::string> out;

        std::string line;
        while (std::getline(file, line)) {
            out.push_back(line);
        }

        return out;
    }*/



    bool WriteFileBytes(const fs::path& path, const BYTE_t* buf, int size) {
        //auto file = GetOutFile(path);
        std::ofstream file(path, std::ios::binary);

        if (!file)
            return false;

        std::copy(buf, buf + size, 
            std::ostream_iterator<BYTE_t>(file));

        file.close();

        return true;
    }

    bool WriteFileBytes(const fs::path& path, const BYTES_t& vec) {
        //auto file = GetOutFile(path);
        std::ofstream file(path, std::ios::binary);

        if (!file)
            return false;

        std::copy(vec.begin(), vec.end(), 
            std::ostream_iterator<BYTE_t>(file));

        file.close();

        return true;
    }

    bool WriteFileString(const fs::path& path, const std::string& str) {
        //auto file = GetOutFile(path);

        std::ofstream file(path, std::ios::binary);

        if (!file)
            return false;

        std::copy(str.begin(), str.end(), 
            std::ostream_iterator<BYTE_t>(file));

        file.close();

        return true;
    }

    /*
    bool WriteFileLines(const fs::path& path, const std::vector<std::string>& in) {
        //auto file = GetOutFile(path);

        std::ofstream file(path, std::ios::binary);

        if (!file)
            return false;

        for (auto &&str : in) {
            file << str << "\n";
        }

        file.close();

        return true;
    }*/

}
