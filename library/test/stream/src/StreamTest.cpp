#include "CompileSettings.h"
#include "DataStream.h"
#include "Hashes.h"
#include "Peer.h"
#include "Quaternion.h"
#include "Types.h"
#include "Vector.h"
#include "VUtils.h"
#include "ZDOID.h"

#include <gtest/gtest.h>
#include <list>
#include <string_view>
#include <vector>

using namespace avledet::util;

TEST(AvledetStream, writeRead)
//void op()
{
    static auto const TEST1 = (int) 1234;
    static auto const TEST2 = std::string_view("abcdef");
    static auto const TEST3 = std::vector<int> {19,    293,      8129,     19,         28342,     239419,
                                                94591, 12932114, 19491841, 1298318390, 109810391, 982491};
    static auto const TEST4 = (char16_t) 0x4386;
    static auto const TEST5 = Vector3f(0.5f, 0.1f, 7.8f);
    static auto const TEST6 = Quaternion::IDENTITY;
    static auto const TEST7 = ZDOID(3981319197134917, 1283);
    static auto const TEST8
            = std::list<std::string_view> {"s", "tmp", "path", "bin", "id", "home", "oid", "db", "blah"};

    int last_pos = 0;
    Writer writer;

    auto const do_test = [&](auto const &value) -> void {
        writer.write(value);
        auto reader       = Reader(writer.get_buf(), last_pos);
        auto &&value_read = reader.read<decltype(value)>();

        ASSERT_EQ(value_read, value);

        last_pos = writer.get_pos();
    };

    do_test(TEST1);
    do_test(TEST2);
    do_test(TEST3);
    do_test(TEST4);
    do_test(TEST5);
    do_test(TEST6);
    do_test(TEST7);
    do_test(TEST8);
}

TEST(AvledetStream, writeReadScopedBlock)
{
    avledet::util::Writer writer;
    {
        avledet::util::WriterScopedEncap scoped(writer);

        writer.write(avledet::util::UserID(318390941983191341));
        writer.write(std::string_view(VConstants::GAME));
        writer.write(VConstants::NETWORK);
        writer.write(Vector3f::ZERO);      // dummy
        writer.write(std::string_view(""));// dummy

        //auto world = WorldManager::instance().GetWorld();

        writer.write(std::string_view(""));                       //world->m_name);
        writer.write(avledet::util::get_stable_hash("vj19gysh4"));//world->m_seed));
        writer.write(std::string_view("vj19gysh4"));//world->m_seedName);// Peer does not seem to use
        writer.write(avledet::util::UserID(981893791712731911));//world->m_uid);
        writer.write((int) 2);                                  //world->m_worldGenVersion);
        writer.write(2034.5);                                   //Avledet::instance().GetWorldTime());
    }
    //ASSERT_NO_THROW(statement)
    auto rpc = avledet::rpc::RpcBase<int>();
    rpc.register_method(avledet::util::hashes::Rpc::PeerInfo, [](int, avledet::util::Bytes bytes) {
        Reader reader(std::move(bytes));

        ASSERT_EQ(reader.read<avledet::util::UserID>(), 318390941983191341);
        ASSERT_EQ(reader.read<std::string>(), std::string_view(VConstants::GAME));
        ASSERT_EQ(reader.read<int>(), VConstants::NETWORK);
        ASSERT_EQ(reader.read<Vector3f>(), Vector3f::ZERO);
        ASSERT_EQ(reader.read<std::string>(), std::string_view(""));
    });

    Reader reader(writer.release());
    rpc.internal_invoke(0, avledet::util::hashes::Rpc::PeerInfo, reader);
}

TEST(AvledetStream, serializeDeserialize)
{
    static auto const TEST1 = (int) 1234;
    static auto const TEST2 = std::string_view("abcdef");
    static auto const TEST3 = std::vector<int> {19,    293,      8129,     19,         28342,     239419,
                                                94591, 12932114, 19491841, 1298318390, 109810391, 982491};
    static auto const TEST4 = (char16_t) 0x4386;
    static auto const TEST5 = Vector3f(0.5f, 0.1f, 7.8f);
    static auto const TEST6 = Quaternion::IDENTITY;
    static auto const TEST7 = ZDOID(3981319197134917, 1283);
    static auto const TEST8
            = std::list<std::string_view> {"s", "tmp", "path", "bin", "id", "home", "oid", "db", "blah"};

    auto bytes = Writer::serialize(TEST1, TEST2, TEST3, TEST4, TEST5, TEST6, TEST7, TEST8);

    Reader reader(bytes);

    ASSERT_EQ(reader.read<decltype(TEST1)>(), TEST1);
    ASSERT_EQ(reader.read<decltype(TEST2)>(), TEST2);
    ASSERT_EQ(reader.read<decltype(TEST3)>(), TEST3);
    ASSERT_EQ(reader.read<decltype(TEST4)>(), TEST4);
    ASSERT_EQ(reader.read<decltype(TEST5)>(), TEST5);
    ASSERT_EQ(reader.read<decltype(TEST6)>(), TEST6);
    ASSERT_EQ(reader.read<decltype(TEST7)>(), TEST7);
    ASSERT_EQ(reader.read<decltype(TEST8)>(), TEST8);
}