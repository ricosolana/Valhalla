#include <fstream>
#include <iostream>
#include <sstream>

#include "VUtilsResource.h"

namespace VUtils::Resource {

    bool WriteFile(fs::path const &path, avledet::util::Byte const *buf, std::size_t size)
    {
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

        file.write(reinterpret_cast<char const *>(buf), size);

        return true;
    }

    bool WriteFile(fs::path const &path, avledet::util::Bytes const &vec)
    {
        return WriteFile(path, vec.data(), vec.size());
    }

    bool WriteFile(fs::path const &path, std::string_view str)
    {
        return WriteFile(path, reinterpret_cast<avledet::util::Byte const *>(str.data()), str.size());
    }


}// namespace VUtils::Resource
