#include <iostream>
#include <sstream>
#include <fstream>

#include "VUtilsResource.h"

namespace VUtils::Resource {

    bool WriteFile(const fs::path& path, const avledet::util::Byte* buf, std::size_t size) {
        //ScopedFile file = fopen(path.string().c_str(), "wb");
        //
        //if (!file) return false;
        //
        //auto sizeWritten = fwrite(buf, 1, size, file);
        //if (sizeWritten != size) {
        //    return false;
        //}
        //
        //return true;

        std::ofstream file(path, std::ios::binary);

        if (!file)
            return false;

        //VLOG(1) << "Writing file " << path << " (" << size << " bytes)";

        file.write(reinterpret_cast<const char*>(buf), size);

        return true;
    }

    bool WriteFile(const fs::path& path, const avledet::util::Bytes& vec) {
        return WriteFile(path, vec.data(), vec.size());
    }

    bool WriteFile(const fs::path& path, std::string_view str) {
        return WriteFile(path, reinterpret_cast<const avledet::util::Byte*>(str.data()), str.size());
    }


}
