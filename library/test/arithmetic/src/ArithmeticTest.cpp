#include "Quaternion.h"
#include "VUtilsPhysics.h"
#include <cmath>
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

TEST(AvledetArithmetic, BoxBoxOverlap)
{

            

    ASSERT_TRUE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(0, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0),
        Vector3f(0, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0)
    ));

    ASSERT_TRUE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(9.99f, 1, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0),
        Vector3f(10, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0)
    ));

    ASSERT_FALSE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(20, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0),
        Vector3f(20, 1.01f, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0)
    ));

    ASSERT_TRUE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(30, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0),
        Vector3f(30.99f, 0.99f, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0)
    ));

    ASSERT_TRUE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(39, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0),
        Vector3f(39.99f, 0.99f, 0.99f), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0)
    ));

    ASSERT_FALSE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(49, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0),
        Vector3f(50.01f, 1.01f, 1.01f), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0)
    ));

    ASSERT_FALSE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(60, 0, 0), Vector3f(1, 4, 1), Quaternion::euler(0, 0, 0),
        Vector3f(60.9f, 2.4f, 0), Vector3f(1, 4, 1), Quaternion::euler(0, 0, 45)
    ));

    ASSERT_TRUE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(70, 0, 0), Vector3f(1, 4, 1), Quaternion::euler(0, -45, 0),
        Vector3f(70, 0, 1.4f), Vector3f(1, 4, 1), Quaternion::euler(0, -45, 0)
    ));

    ASSERT_FALSE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(80, -0.298f, 0.066f), Vector3f(1, 4, 1), Quaternion::euler(-34.727f, -51.238f, 1.952f),
        Vector3f(80, 0.298f, 1.544f), Vector3f(1, 4, 1), Quaternion::euler(-36.663f, -50.099f, 2.502f)
    ));

    ASSERT_FALSE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(90, 0.535f, 0.222f), Vector3f(1, 4, 1), Quaternion::euler(8.421f, -30.361f, -59.639f),
        Vector3f(90, -0.535f, 1.292f), Vector3f(1, 4, 1), Quaternion::euler(8.421f, -30.361f, -59.639f)
    ));









    /*
    // box dup box
    // A-B
    ASSERT_TRUE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(0, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0), 
        Vector3f(0, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0)
    ));

    // box dup box
    // A-B(tiny)
    ASSERT_TRUE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(0, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0), 
        Vector3f(0, 0, 0), Vector3f(1, 1, 1) * .5f, Quaternion::euler(0, 0, 0)
    ));

    // side-by-side, face overlap
    // A
    // B
    ASSERT_TRUE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(0, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0), 
        Vector3f(0, 1, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0)
    ));

    // side-by-side, no face overlap
    // A
    // B
    ASSERT_FALSE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(0, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0), 
        Vector3f(0, 1.001, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0)
    ));

    // side-by-side, B diamond rotation
    // A
    // B (rot)
    ASSERT_TRUE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(0, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0), 
        Vector3f(0, 1, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 45, 0)
    ));

    // B is tiny and diamond rotation
    // A
    // B (rot, tiny)
    ASSERT_FALSE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(0, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0), 
        Vector3f(0, 1, 0), Vector3f(1, 1, 1) * 0.5f, Quaternion::euler(0, 45, 0)
    ));

    // B is diagonal on all axis from A
    // A
    //  B
    ASSERT_TRUE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(0, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0), 
        Vector3f(1, 1, 1), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0)
    ));

    // side-by-side, long rotated rects
    // A /
    //  B /
    ASSERT_TRUE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(0, 0, 0), Vector3f(1, 10, 1), Quaternion::euler(0, 0, 45.0f), 
        Vector3f((float)(2.0 * (0.5/std::cos(45.0 * 3.14159265435/180.0))), 0, 0), Vector3f(1, 10, 1), Quaternion::euler(0, 0, 45.0f)
    ));

    // long rotated rects
    // A /
    //  B /
    ASSERT_FALSE(VUtils::Physics::BoxBoxOverlap(
        Vector3f(0, 0, 0), Vector3f(1, 10, 1), Quaternion::euler(0, 0, 45.0f), 
        Vector3f((float)(2.0 * (0.5/std::cos(45.0 * 3.14159265435/180.0))) + 0.001f, 0, 0), Vector3f(1, 10, 1), Quaternion::euler(0, 0, 45.0f)
    ));
    */
}
