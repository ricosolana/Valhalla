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
        ScopedFile file(fopen(path.string().c_str(), "rb"));
        
        if (!file)
            return std::nullopt;

        if (!fseek(file, 0, SEEK_END))
            return std::nullopt;

        auto size = ftell(file);
        if (size == -1)
            return std::nullopt;

        if (!fseek(file, 0, SEEK_SET))
            return std::nullopt;

        BYTES_t vec;

        // somehow avoid the zero-initialization
        vec.resize(size);

        fread_s(vec.data(), vec.size(), 1, size, file);

        return vec;
    }

    std::optional<std::string> ReadFileString(const fs::path& path) {
        ScopedFile file(fopen(path.string().c_str(), "rb"));

        if (!file)
            return std::nullopt;

        if (!fseek(file, 0, SEEK_END))
            return std::nullopt;

        auto size = ftell(file);
        if (size == -1)
            return std::nullopt;

        if (!fseek(file, 0, SEEK_SET))
            return std::nullopt;

        std::string str;

        // somehow avoid the zero-initialization
        str.resize(size);

        fread_s(str.data(), str.size(), 1, size, file);

        return str;
    }
    


    std::optional<std::list<std::string_view>> ReadFileLines(const fs::path& path, bool includeBlanks, std::string& out) {
        ScopedFile file = fopen(path.string().c_str(), "rb");

        if (!file)
            return std::nullopt;

        if (!fseek(file, 0, SEEK_END)) return std::nullopt;
        const int size = ftell(file); if (size == -1) return std::nullopt;
        if (!fseek(file, 0, SEEK_SET)) return std::nullopt;

        // somehow avoid the zero-initialization
        out.resize(size);

        char* data = out.data();

        if (fread(data, 1, size, file) != size && ferror(file))
            return std::nullopt;

        std::list<std::string_view> lines;

        int lineIdx = -1;
        int lineSize = 0;
        for (int i = 0; i < size; i++) {
            // if theres a newline at end of file, do not include

            // if at end of file and no newline at end

            lineSize = i - lineIdx - 1;

            // we all hate \r\n

            if (data[i] == '\n') {
                if (lineSize || includeBlanks) {
                    lines.push_back(std::string_view(data + lineIdx + 1, lineSize));
                }
                lineIdx = i;
            }
        }

        // this includes last line ONLY if it is not blank (has at least 1 character)
        if (lineIdx < size - 1) {
            lines.push_back(std::string_view(data + lineIdx + 1, size - lineIdx - 1));
        }

        return lines;
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
