#pragma once

#include <cstdint>
#include <stdexcept>
#include <quill/Backend.h>
#include <quill/bundled/fmt/ostream.h>
#include "QuillHelperMacros.h"
#include "VUtils.h"
#include "BitPack.h"



#pragma once

#include <UserID.h>
#include <Stream.h>
#include <ankerl/unordered_dense.h>
#include <assert.h>

namespace avledet::util {

    class ZDOID {
    public:
        ZDOID();
        ZDOID(avledet::util::UserID userid, std::uint32_t id);

        //avledet::util::UserID get_userid() const;
        //std::uint32_t get_id() const;

        bool operator==(ZDOID const&) const;
        operator bool() const;

    public:
        avledet::util::UserID m_userid;
        std::uint16_t ___{}; // 0 padding
        std::uint32_t m_id{};
    };

}// namespace avledet::sync

template <>
struct ankerl::unordered_dense::hash<avledet::util::ZDOID> {
    using is_avalanching = void; // high quality hash

    static_assert(std::has_unique_object_representations_v<avledet::util::ZDOID>);

    auto operator()(avledet::util::ZDOID const& value) const noexcept -> std::uint64_t {
        return ankerl::unordered_dense::detail::wyhash::hash(&value, sizeof(value));
    }
};

template <>
struct avledet::util::Streamer<avledet::util::ZDOID> {
    void operator()(avledet::util::Writer& writer, avledet::util::ZDOID const& zdoid) {
        writer.write(zdoid.m_userid);
        writer.write(zdoid.m_id);
    }

    avledet::util::ZDOID operator()(avledet::util::Reader& reader) {
        return avledet::util::ZDOID(
            reader.read<avledet::util::UserID>(),
            reader.read<std::uint32_t>()
        );
    }
};

std::ostream& operator<<(std::ostream& st, avledet::util::ZDOID const& zdoid) {
    //st << (int64_t)zdoid.GetOwner() << ":" << zdoid.GetUID();
    return st;
}

QUILL_LOGGABLE_DIRECT_FORMAT(avledet::util::ZDOID)

using ZDOID = avledet::util::ZDOID;
