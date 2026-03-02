#include "CompileSettings.h"
#include "DataStream.h"
#include "Quaternion.h"
#include "Types.h"
#include "Vector.h"
#include "VUtils.h"
#include <cstdint>
#include <luaconf.h>
#include <sol/overload.hpp>
#include <sol/resolve.hpp>
#include <string_view>

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)

    #include <sol/forward.hpp>

    #include "ModManager.h"

void IScriptManager::load_userdata_types()
{
    LOG_DEBUG(AVL_LOGGER, "Initializing API types - types");

    using namespace avledet::util;

    // clang-format off

    m_state.new_enum("Type", 
        "BOOL", StreamType::BOOL,

        "STRING", StreamType::STRING, "str", StreamType::STRING,
        "STRINGS", StreamType::STRINGS, "strs", StreamType::STRINGS,

        "BYTES", StreamType::BYTES,

        "ZDOID", StreamType::ZDOID, "zid", StreamType::ZDOID,
        "VECTOR3f", StreamType::VECTOR3f, "vec3f", StreamType::VECTOR3f, 
        "VECTOR2i", StreamType::VECTOR2i, "vec2i", StreamType::VECTOR2i,
        "QUATERNION", StreamType::QUATERNION, "quat", StreamType::QUATERNION,

        "HASH", StreamType::INT32, 

        "INT8", StreamType::INT8, "i8", StreamType::INT8, 
        "INT16", StreamType::INT16, "SHORT", StreamType::INT16, "i16", StreamType::INT16,
        "INT32", StreamType::INT32, "INT", StreamType::INT32, "i32", StreamType::INT32, 
        "INT64", StreamType::INT64, "LONG", StreamType::INT64, "i64", StreamType::INT64,
        "UINT8", StreamType::UINT8, "BYTE", StreamType::UINT8, "u8", StreamType::UINT8, 
        "UINT16", StreamType::UINT16, "USHORT", StreamType::UINT16, "u16", StreamType::UINT16, 
        "UINT32", StreamType::UINT32, "UINT", StreamType::UINT32, "u32", StreamType::UINT32, 
        "UINT64", StreamType::UINT64, "ULONG", StreamType::UINT64, "u64", StreamType::UINT64,
        "FLOAT", StreamType::FLOAT, "f32", StreamType::FLOAT,
        "DOUBLE", StreamType::DOUBLE, "f64", StreamType::DOUBLE,
        "CHAR16", StreamType::CHAR16, "utf", StreamType::CHAR16
    );

    // TODO
    //  this seems like some very unsafe / sketchy usage (wrapping vector...)
    // TODO
    //  looks like msvc compiler is failing to find a (or some) >>ost operators, because dum... even though op>> is optional
    //this->new_usertype<Bytes>("Bytes", 
    //    sol::constructors<Bytes(), Bytes(Bytes const &)>(), 
    //    "assign", [](Bytes &self, Bytes const &other) { self = other; }, 
    //    "move", [](Bytes &self, Bytes &other) { self = std::move(other); }, 
    //    "swap", [](Bytes &self, Bytes &other) { self.swap(other); }
    //);

    // TODO impl
    //state.new_usertype<UserProfile>("UserProfile",
    //    sol::constructors<UserProfile(std::string, std::string, std::string)>(),
    //    "name", &UserProfile::m_name,
    //    "tag", &UserProfile::m_gamerTag, // TODO change name
    //    "nid", &UserProfile::m_networkUserId // TODO change name
    //);

    this->new_usertype<Writer>("Writer", 
        sol::constructors<Writer(), Writer(Bytes)>(),
        "pos", sol::property(&Writer::get_pos, &Writer::set_pos),//& DataWriter::m_pos,
        "seek", &Writer::seek,
        "write_bool", &Writer::write<bool>, 
        "write_string", &Writer::write<std::string_view>,
        "write_strings", &Writer::write<Strings>,
        "write_bytes", &Writer::write<Bytes>, 
        "write_zdoid", &Writer::write<ZDOID>, 
        "write_vec3f", &Writer::write<Vector3f>, 
        "write_vec2i", &Writer::write<Vector2i>, 
        "write_quat", &Writer::write<Quaternion>,
        //"write_profile", &DataWriter::write<UserProfile>, //TODO
        "write_i8", sol::overload(
            &Writer::write<std::int8_t>
        ),
        "write_i16", sol::overload(
            &Writer::write<std::int16_t>
        ), 
        "write_i32", sol::overload(
            &Writer::write<std::int32_t>
        ), 
        "write_i64", sol::overload(
            &Writer::write<std::int64_t>
        ), 
        //unsigned
        "write_u8", sol::overload(
            &Writer::write<std::uint8_t>
        ),
        "write_u16", sol::overload(
            &Writer::write<std::uint16_t>
        ), 
        "write_u32", sol::overload(
            &Writer::write<std::uint32_t>
        ), 
        "write_u64", sol::overload(
            &Writer::write<std::uint64_t>
        ), 



        //"write_i8", &Writer::write<std::int8_t>,
        //"write_i16", &Writer::write<std::int16_t>, 
        //"write_i32", &Writer::write<std::int32_t>,
        //"write_i64", &Writer::write<Int64Wrapper>,
        //"write_u8", &Writer::write<std::uint8_t>, 
        //"write_u16", &Writer::write<std::uint16_t>,
        //"write_u32", &Writer::write<std::uint32_t>, 
        //"write_u64", &Writer::write<UInt64Wrapper>,
        "write_float", &Writer::write<std::float_t>, 
        "write_double", &Writer::write<std::double_t>, 
        "write_char16", &Writer::write<char16_t>, 
        "write", sol::overload( // lazy write overloads
            &Writer::write<bool>,
            &Writer::write<std::string_view>,
            &Writer::write<Bytes>,
            &Writer::write<ZDOID>,
            &Writer::write<Vector3f>,
            &Writer::write<Vector2i>,
            &Writer::write<Quaternion>,
            &Writer::write<StreamType, sol::object>, //usage: writer:write(Type.INT16, my_num)
            // variadics
            &Writer::write<StreamTypes, sol::variadic_args> //usage: writer:write({Type...}, a, b, c)
        )
    );

    // Package read/write types
    this->new_usertype<Reader>("Reader", 
        sol::constructors<Reader(Bytes)>(),
        "pos", sol::property(&Reader::get_pos, &Reader::set_pos),
        "seek", &Reader::seek,
        "read_bool", &Reader::read<bool>,
        "read_string", &Reader::read<std::string>,
        "read_strings", &Reader::read<Strings>,
        "read_bytes", &Reader::read<Bytes>,
        "read_zdoid", &Reader::read<ZDOID>,
        "read_vec3f", &Reader::read<Vector3f>,
        "read_vec2i", &Reader::read<Vector2i>,
        "read_quat", &Reader::read<Quaternion>,
        //"ReadProfile", [](Reader& self) { return self.read<UserProfile>(); }, //&Reader::read<UserProfile>, //TODO impl
        "read_s8", &Reader::read<std::int8_t>,
        "read_s16", &Reader::read<std::int16_t>,
        "read_s32", &Reader::read<std::int32_t>,
        "read_s64", &Reader::read<std::int64_t>,
        "read_u8", &Reader::read<std::uint8_t>,
        "read_u16", &Reader::read<std::uint16_t>,
        "read_u32", &Reader::read<std::uint32_t>,
        "read_u64", &Reader::read<std::uint64_t>,
        "read_float", &Reader::read<std::float_t>,
        "read_double", &Reader::read<std::double_t>,
        "read_char16", &Reader::read<char16_t>,
        //  local reader = Reader.new()
        //  local a, b, c = reader:read(Type.INT, Type.FLOAT, Type.QUATERNION)
        "read", [](Reader &self, sol::state_view state, sol::variadic_args args) {
            return self.read(StreamTypes(args.begin(), args.end()), state);
        }
    );

    m_state.new_enum("TimeOfDay", 
        "MORNING", TIME_MORNING, 
        "DAY", TIME_DAY, 
        "AFTERNOON", TIME_AFTERNOON,
        "NIGHT", TIME_NIGHT
    );


    this->new_usertype<ZStdCompressor>("ZStdCompressor",
        sol::constructors<ZStdCompressor(int), ZStdCompressor(), ZStdCompressor(Bytes const &)>(),
        "compress", sol::resolve<std::optional<Bytes>(Bytes const &) const>(&ZStdCompressor::Compress)
    );

    this->new_usertype<ZStdDecompressor>("ZStdDecompressor", 
        sol::constructors<ZStdDecompressor(), ZStdDecompressor(Bytes const &)>(),
        "decompress", sol::resolve<std::optional<Bytes>(Bytes const &) const>(&ZStdDecompressor::Decompress)
    );


    this->new_usertype<Deflater>("Deflator", 
        sol::no_constructor,
        "gz", sol::property(sol::resolve<Deflater()>(Deflater::Gz)), 
        "zlib", sol::property(sol::resolve<Deflater()>(Deflater::ZLib)), 
        "raw", sol::property(sol::resolve<Deflater()>(Deflater::Raw)), 
        "compress", sol::resolve<std::optional<Bytes>(ByteView const &) const>(&Deflater::Compress)
    );

    this->new_usertype<Inflater>("Inflator",
        sol::no_constructor,
        //"any", sol::property(Inflater::Any),
        "gz", sol::property(Inflater::Gz), 
        "zlib", sol::property(Inflater::ZLib), 
        "auto", sol::property(Inflater::Auto), //TODO rename to Any / Magic
        "raw", sol::property(Inflater::Raw), 
        "decompress", sol::resolve<std::optional<Bytes>(ByteView const &) const>(&Inflater::Decompress)
    );

    // clang-format on
}

#endif
