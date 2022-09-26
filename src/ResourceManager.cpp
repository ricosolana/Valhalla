#include "ResourceManager.h"

#include <iostream>
#include <sstream>
#include <fstream>

#include <robin_hood.h>

namespace ResourceManager {
    // Instantiate static variables
    std::string root;

    void SetRoot(const char* r) {
        root = r;
    }

    //FILE* OpenFile(const char* path) {
    //    return OpenFile(path);
    //}

    FILE* OpenFile(const std::string &path) {
        return fopen((root + path).c_str(), "rb");
    }

    bool ReadFileBytes(const char* path, std::vector<unsigned char> &buffer) {
        auto f = OpenFile(path);

        if (!f)
            return false;

        fseek(f, 0, SEEK_END);
        auto buffer_size = ftell(f);
        fseek(f, 0, SEEK_SET);

        buffer.resize(buffer_size);

        //char* buffer = new char[buffer_size];
        fread(buffer.data(), 1, buffer_size, f);
        fclose(f);
        return true;
    }

    bool ReadFileBytes(const char* path, std::string& buffer) {
        auto f = OpenFile(path);

        if (!f)
            return false;

        fseek(f, 0, SEEK_END);
        auto buffer_size = ftell(f);
        fseek(f, 0, SEEK_SET);

        buffer.resize(buffer_size);

        //char* buffer = new char[buffer_size];
        fread(buffer.data(), 1, buffer_size, f);
        fclose(f);
        return true;
    }
}
