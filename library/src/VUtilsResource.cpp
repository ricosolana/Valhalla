#include <fstream>
#include <iostream>
#include <sstream>

#include "VUtilsResource.h"

namespace VUtils::Resource {

    bool WriteFile(std::filesystem::path const &path, avledet::util::Byte const *buf, std::size_t size)
    {
        std::ofstream file;

        // Explicitly ensure ordinary stream errors become state flags,
        // not exceptions.
        file.exceptions(std::ios::goodbit);

        file.open(path, std::ios::binary | std::ios::trunc);

        if (!file.is_open() || file.fail())
            return false;
        
        const auto* data = reinterpret_cast<char const *>(buf);
        std::size_t remaining = size;

        // std::ostream::write takes std::streamsize, so write in bounded chunks.
        constexpr auto max_chunk =
            static_cast<std::size_t>(
                std::numeric_limits<std::streamsize>::max());

        while (remaining != 0) {
            const std::size_t chunk = std::min(remaining, max_chunk);

            file.write(
                data,
                static_cast<std::streamsize>(chunk));

            if (!file)
                return false;

            data += chunk;
            remaining -= chunk;
        }

        file.flush();

        if (!file)
            return false;

        file.close();

        // close() can discover errors that write()/flush() did not.
        return !file.fail();
    }

    bool WriteFile(std::filesystem::path const &path, avledet::util::Bytes const &vec)
    {
        return WriteFile(path, vec.data(), vec.size());
    }

    bool WriteFile(std::filesystem::path const &path, std::string_view str)
    {
        return WriteFile(path, reinterpret_cast<avledet::util::Byte const *>(str.data()), str.size());
    }


}// namespace VUtils::Resource
