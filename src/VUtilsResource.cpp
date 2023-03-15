#include <iostream>
#include <sstream>
#include <fstream>
#include <robin_hood.h>

#include "VUtilsResource.h"

namespace VUtils::Resource {

    bool WriteFile(const fs::path& path, const BYTE_t* buf, int size) {
        ScopedFile file = fopen(path.string().c_str(), "wb");

        if (!file) return false;

        auto sizeWritten = fwrite(buf, 1, size, file);
        if (sizeWritten != size) {
            return false;
        }

        return true;
    }

    bool WriteFile(const fs::path& path, const BYTES_t& vec) {
        return WriteFile(path, vec.data(), vec.size());
    }

    bool WriteFile(const fs::path& path, const std::string& str) {
        return WriteFile(path, reinterpret_cast<const BYTE_t*>(str.data()), str.size());
    }


}
