#include "ModManager.h"
#include <sol/forward.hpp>

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)

void IScriptManager::load_userdata_types()
{
    LOG_DEBUG(AVL_LOGGER, "Initializing API types - types");

    using namespace avledet::util;

    this->new_usertype<ZDOID>(
            "ZDOID",
            //sol::constructors<ZDOID(UserID userID, std::uint32_t id)>(),
            sol::factories(
                    [](Int64Wrapper uuid, std::uint32_t id) { return ZDOID((std::int64_t) uuid, id); }),
            "NONE", sol::var(ZDOID::NONE),// sol::property([]() { return ZDOID::NONE; }),
            "user_id",
            sol::property([](ZDOID &self) { return (Int64Wrapper) self.get_user_id(); },
                          [](ZDOID &self, Int64Wrapper value) { self.set_user_id((std::int64_t) value); }),
            "id", sol::property(&ZDOID::get_id, &ZDOID::set_id));

    m_state.new_enum("Type", "BOOL", StreamType::BOOL,

                     "STRING", StreamType::STRING, "STRINGS", StreamType::STRINGS,

                     "BYTES", StreamType::BYTES,

                     "ZDOID", StreamType::ZDOID, "VECTOR3f", StreamType::VECTOR3f, "vec3f",
                     StreamType::VECTOR3f, "VECTOR2i", StreamType::VECTOR2i, "vec2i", StreamType::VECTOR2i,
                     "QUATERNION", StreamType::QUATERNION, "quat", StreamType::QUATERNION,

                     "INT8", StreamType::INT8, "s8", StreamType::INT8, "INT16", StreamType::INT16, "SHORT",
                     StreamType::INT16, "s16", StreamType::INT16, "INT32", StreamType::INT32, "INT",
                     StreamType::INT32, "HASH", StreamType::INT32, "s32", StreamType::INT32, "INT64",
                     StreamType::INT64, "LONG", StreamType::INT64, "s64", StreamType::INT64,

                     "UINT8", StreamType::UINT8, "BYTE", StreamType::UINT8, "u8", StreamType::UINT8, "UINT16",
                     StreamType::UINT16, "USHORT", StreamType::UINT16, "u16", StreamType::UINT16, "UINT32",
                     StreamType::UINT32, "UINT", StreamType::UINT32, "u32", StreamType::UINT32, "UINT64",
                     StreamType::UINT64, "ULONG", StreamType::UINT64, "u64", StreamType::UINT64,

                     "FLOAT", StreamType::FLOAT, "DOUBLE", StreamType::DOUBLE,

                     "CHAR16", StreamType::CHAR16);

    // TODO
    //  this seems like some very unsafe / sketchy usage
    this->new_usertype<Bytes>(
            "Bytes", sol::constructors<Bytes(), Bytes(Bytes const &)>(), "assign",
            [](Bytes &self, Bytes const &other) { self = other; }, "move",
            [](Bytes &self, Bytes &other) { self = std::move(other); }, "swap",
            [](Bytes &self, Bytes &other) { self.swap(other); });

    // TODO impl
    //state.new_usertype<UserProfile>("UserProfile",
    //    sol::constructors<UserProfile(std::string, std::string, std::string)>(),
    //    "name", &UserProfile::m_name,
    //    "tag", &UserProfile::m_gamerTag, // TODO change name
    //    "nid", &UserProfile::m_networkUserId // TODO change name
    //);

    this->new_usertype<DataWriter>(
            "Writer", sol::constructors<DataWriter(Bytes)>(),

            //"ToReader", &DataWriter::ToReader,
            //"buf", &DataWriter::get_buf, //TODO currently unsafe
            "pos", sol::property(&DataWriter::get_pos, &DataWriter::set_pos),//& DataWriter::m_pos,

            //"Clear", &DataWriter::Clear,

            "write_bool", &DataWriter::write<bool>, "write_string", &DataWriter::write<std::string_view>,
            "write_bytes", &DataWriter::write<Bytes>, "write_zdoid", &DataWriter::write<ZDOID>, "write_vec3f",
            &DataWriter::write<Vector3f>, "write_vec2i", &DataWriter::write<Vector2i>, "write_quat",
            &DataWriter::write<Quaternion>,
            //"write_profile", &DataWriter::write<UserProfile>, //TODO impl

            //TODO impl everything
            "write_s8",
            &DataWriter::write<
                    std::int8_t>,// static_cast<void (DataWriter::*)(std::int8_t)>(&DataWriter::write),
            "write_s16", &DataWriter::write<std::int16_t>, "write_s32", &DataWriter::write<std::int32_t>,
            //"write_s64", &DataWriter::write<std::int64_t>,// TODO impl: intwrapper
            "write_u8", &DataWriter::write<std::uint8_t>, "write_u16", &DataWriter::write<std::uint16_t>,
            "write_u32", &DataWriter::write<std::uint32_t>, "write_u64", &DataWriter::write<std::uint64_t>,
            "write_float", &DataWriter::write<std::float_t>, "write_double",
            &DataWriter::write<std::double_t>, "write_char16", &DataWriter::write<char16_t>, "write",
            sol::overload([](DataWriter &self, bool val) { return self.write(val); },
                          [](DataWriter &self, std::string_view val) { return self.write(val); },
                          [](DataWriter &self, Bytes const &val) { return self.write(val); },
                          [](DataWriter &self, ZDOID const &val) { return self.write(val); },
                          [](DataWriter &self, Vector3f const &val) { return self.write(val); },
                          [](DataWriter &self, Vector2i const &val) { return self.write(val); },
                          [](DataWriter &self, Quaternion const &val) { return self.write(val); },
                          // Variadic serializers:::
                          [](DataWriter &self, StreamType type, sol::object obj) { self.write(type, obj); },
                          //{ &DataWriter::write<IScriptManager::StreamType, sol::object> },
                          [](DataWriter &self, StreamTypes const &types, sol::variadic_args args) {
                              self.write(types, sol::variadic_results(args.begin(), args.end()));
                          }));

    // Package read/write types
    this->new_usertype<DataReader>(
            "Reader", sol::constructors<DataReader(Bytes)>(),

            //"ToWriter", &DataReader::ToWriter,
            //"buf", &DataReader::m_buf,
            /*
        "buf", sol::property(
            sol::overload(
                [](DataReader& self, Bytes& value) { self.m_data = std::ref(value); },
                [](DataReader& self, ByteView value) { self.m_data = value; }
            ),
            [this](DataReader& self) {
                return std::visit(VUtils::Traits::overload{
                    [this](std::reference_wrapper<Bytes> buf) { return sol::make_object(state, buf); },
                    [this](ByteView buf) { return sol::make_object(state, buf); }
                }, self.m_data);
            }
        ),*/
            //"buf", &DataReader::m_data, // TODO ref change
            "pos", sol::property(&DataReader::get_pos, &DataReader::set_pos),//& DataWriter::m_pos,

            "read_bool", [](DataReader &self) { return self.read<bool>(); },

            "read_string", [](DataReader &self) { return self.read<std::string>(); }, "read_strings",
            [](DataReader &self) { return self.read<Strings>(); },// ReadStrings,

            "read_bytes", [](DataReader &self) { return self.read<Bytes>(); },

            "read_zdoid", [](DataReader &self) { return self.read<ZDOID>(); },//&DataReader::read<ZDOID>,
            "read_vec3f",
            [](DataReader &self) { return self.read<Vector3f>(); },  //&DataReader::read<CSU::Vector3f>,
            "read_vec2i",
            [](DataReader &self) { return self.read<Vector2i>(); },  //&DataReader::read<CSU::Vector2i>,
            "read_quat",
            [](DataReader &self) { return self.read<Quaternion>(); },//&DataReader::read<CSU::Quaternion>,
            //"ReadProfile", [](DataReader& self) { return self.read<UserProfile>(); }, //&DataReader::read<UserProfile>, //TODO impl

            "read_s8",
            [](DataReader &self) { return self.read<std::int8_t>(); }, //&DataReader::read<std::int8_t>,
            "read_s16",
            [](DataReader &self) { return self.read<std::int16_t>(); },//&DataReader::read<std::int16_t>,
            "read_s32",
            [](DataReader &self) { return self.read<std::int32_t>(); },//&DataReader::read<std::int32_t>,
            //"read_s64", &DataReader::ReadInt64Wrapper, //TODO Streamer impl

            "read_u8",
            [](DataReader &self) { return self.read<std::uint8_t>(); }, //&DataReader::read<std::uint8_t>,
            "read_u16",
            [](DataReader &self) { return self.read<std::uint16_t>(); },//&DataReader::read<std::uint16_t>,
            "read_u32",
            [](DataReader &self) { return self.read<std::uint32_t>(); },//&DataReader::read<std::uint32_t>,
            //"read_u64", &DataReader::ReadUInt64Wrapper, //TODO Streamer impl

            "read_float",
            [](DataReader &self) { return self.read<std::float_t>(); }, //&DataReader::read<std::float_t>,
            "read_double",
            [](DataReader &self) { return self.read<std::double_t>(); },//&DataReader::read<std::double_t>,

            "read_char16",
            [](DataReader &self) { return self.read<char16_t>(); },     //&DataReader::read<char16_t>,

            // Generalized variadic read
            //  local reader = Reader.new()
            //  reader:read()
            "read",
            [](DataReader &self, sol::state_view state, sol::variadic_args args) {
                return self.read(StreamTypes(args.begin(), args.end()), state);
            });


    this->new_usertype<Int64Wrapper>(
            "Int64",
            sol::constructors<Int64Wrapper(), Int64Wrapper(std::int64_t),
                              Int64Wrapper(std::uint32_t, std::uint32_t),
                              Int64Wrapper(std::string const &)>(),

            "tonumber", [](Int64Wrapper &self) { return (std::int64_t) self; }, sol::meta_function::addition,
            &Int64Wrapper::operator+, sol::meta_function::subtraction,
            sol::resolve<Int64Wrapper(Int64Wrapper const &) const>(&Int64Wrapper::operator-),
            sol::meta_function::multiplication, &Int64Wrapper::operator*, sol::meta_function::division,
            &Int64Wrapper::operator/, sol::meta_function::floor_division, &Int64Wrapper::__divi,
            sol::meta_function::unary_minus, sol::resolve<Int64Wrapper() const>(&Int64Wrapper::operator-),
            sol::meta_function::equal_to, &Int64Wrapper::operator==, sol::meta_function::less_than,
            &Int64Wrapper::operator<, sol::meta_function::less_than_or_equal_to, &Int64Wrapper::operator<=);

    this->new_usertype<UInt64Wrapper>(
            "UInt64",
            sol::constructors<UInt64Wrapper(), UInt64Wrapper(std::uint64_t),
                              Int64Wrapper(std::uint32_t, std::uint32_t),
                              UInt64Wrapper(std::string const &)>(),

            "tonumber", [](UInt64Wrapper &self) { return (std::uint64_t) self; },
            sol::meta_function::addition, &UInt64Wrapper::operator+, sol::meta_function::subtraction,
            sol::resolve<UInt64Wrapper(UInt64Wrapper const &) const>(&UInt64Wrapper::operator-),
            sol::meta_function::multiplication, &UInt64Wrapper::operator*, sol::meta_function::division,
            &UInt64Wrapper::operator/, sol::meta_function::floor_division, &UInt64Wrapper::__divi,
            sol::meta_function::unary_minus, sol::resolve<UInt64Wrapper() const>(&UInt64Wrapper::operator-),
            sol::meta_function::equal_to, &UInt64Wrapper::operator==, sol::meta_function::less_than,
            &UInt64Wrapper::operator<, sol::meta_function::less_than_or_equal_to, &UInt64Wrapper::operator<=);

    m_state.new_enum("TimeOfDay", "MORNING", TIME_MORNING, "DAY", TIME_DAY, "AFTERNOON", TIME_AFTERNOON,
                     "NIGHT", TIME_NIGHT);


    this->new_usertype<ZStdCompressor>(
            "ZStdCompressor",
            sol::constructors<ZStdCompressor(int), ZStdCompressor(), ZStdCompressor(Bytes const &)>(),
            "compress", sol::resolve<std::optional<Bytes>(Bytes const &) const>(&ZStdCompressor::Compress));

    this->new_usertype<ZStdDecompressor>(
            "ZStdDecompressor", sol::constructors<ZStdDecompressor(), ZStdDecompressor(Bytes const &)>(),
            "decompress",
            sol::resolve<std::optional<Bytes>(Bytes const &) const>(&ZStdDecompressor::Decompress));


    this->new_usertype<Deflater>(
            "Deflater", "gz", sol::property(sol::resolve<Deflater()>(Deflater::Gz)), "zlib",
            sol::property(sol::resolve<Deflater()>(Deflater::ZLib)), "raw",
            sol::property(sol::resolve<Deflater()>(Deflater::Raw)), "compress",
            sol::resolve<std::optional<Bytes>(ByteView const &) const>(&Deflater::Compress));

    this->new_usertype<Inflater>(
            "Inflater",
            //"any", sol::property(Inflater::Any),
            "zlib", sol::property(Inflater::Gz), "gz", sol::property(Inflater::Gz), "auto",
            sol::property(Inflater::Auto), "raw", sol::property(Inflater::Raw), "decompress",
            sol::resolve<std::optional<Bytes>(ByteView const &) const>(&Inflater::Decompress));
}

#endif
