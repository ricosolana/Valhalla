#include <gtest/gtest.h>
#include <Hashes.h>
#include <VUtils.h>

//TODO impl
//TEST(AvledetCrypto, MD5_Hash) {
//    // hash: '5f4dcc3b5aa765d61d8327deb882cf99'
//    auto str = "password";
//
//    auto hash = avledet::crypto::md5(str);
//
//    uint8_t expected_bytes[] = { 0x5f, 0x4d, 0xcc, 0x3b, 0x5a, 0xa7, 0x65, 0xd6,
//                          0x1d, 0x83, 0x27, 0xde, 0xb8, 0x82, 0xcf, 0x99 };
//
//    auto expected = std::string_view((char*)expected_bytes, sizeof(expected_bytes));
//
//    ASSERT_TRUE(hash == expected);
//}

TEST(AvledetCrypto, StableHash)
{
    ASSERT_EQ(avledet::util::get_stable_hash("PeerInfo"), -725574882);
    ASSERT_EQ(avledet::util::get_stable_hash("Disconnect"), 838896224);
    ASSERT_EQ(avledet::util::get_stable_hash("ServerHandshake"), 1233642074);
    ASSERT_EQ(avledet::util::get_stable_hash("Kicked"), 197523735);
    ASSERT_EQ(avledet::util::get_stable_hash("Error"), 22442200);
}
