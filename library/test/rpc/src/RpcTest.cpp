#include "Peer.h"
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

TEST(AvledetRpc, bTreeTransparency)
{
    // test that hash-based retrieval is working is expected
    auto my_rpc = avledet::rpc::RpcBase<int>();

    ASSERT_FALSE(my_rpc.m_methods.contains(3));

    my_rpc.register_method(3, [](int) {});
    ASSERT_TRUE(my_rpc.m_methods.contains(3));

    my_rpc.register_method(3, [](int) {});
    ASSERT_EQ(my_rpc.m_methods.size(), 1);

    my_rpc.register_method(14, [](int) {});
    ASSERT_TRUE(my_rpc.m_methods.contains(14));
    ASSERT_EQ(my_rpc.m_methods.size(), 2);
}
