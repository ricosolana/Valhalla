#include "MonotonicMap.h"
#include <cmath>
#include <gtest/gtest.h>
#include <Hashes.h>
#include <VUtils.h>
#include <stdexcept>

//TEST(AvledetUtil, MonotonicTreeOld)
//{
//    using KeyTuple = std::tuple<int, float>;
//    //using CompareTuple = std::tuple<std::less<int>, std::less<int>>;
//
//    avledet::util::monotonic_tree<
//        KeyTuple,
//        std::string
//    > map;
//
//    ASSERT_EQ(map.find<0>(1), map.end());
//
//    ASSERT_TRUE(map.insert({ { 8, 3.14f }, "A" }).second);
//    ASSERT_NE(map.find<0>(8), map.end());
//    ASSERT_EQ(map.size(), 1);
//    ASSERT_EQ(map.find<1>(3.13f), map.end());
//
//    ASSERT_FALSE(map.insert({ { 8, 3.14f }, "A" }).second);
//
//    ASSERT_THROW(map.insert({ { 9, 2.0f }, "A" }), std::logic_error);
//
//    ASSERT_NO_THROW(map.insert({ { 9, 4.0f }, "A" }));
//
//    ASSERT_NO_THROW(map.insert({ { 10, 4.0f }, "A" }));
//    ASSERT_FALSE(map.find({ { 10, 4.0f }, "A" }));
//}

TEST(AvledetUtil, MonotonicStrict)
{
    avledet::util::mono::parallel_strict_map<
        int, // value
        int, int> m;

    ASSERT_TRUE(m.insert({ 3, 5 },10).second);   // A
    ASSERT_FALSE(m.insert({ 2, 5 }, 20).second);   // clash -> returns {itr,false}

    ASSERT_TRUE(m.insert({ 0, 2 }, 30).second);   // C inserted at front

    ASSERT_THROW(m.insert({ 1, 6 },40), std::logic_error);   // THROW (ordering rule impossible)
}

TEST(AvledetUtil, MonotonicWeak)
{
    avledet::util::mono::parallel_weak_map<
        int, // value
        int, int> m;

    ASSERT_TRUE(m.insert({ 3, 5 },10).second);   // A
    ASSERT_TRUE(m.insert({ 2, 5 }, 20).second);   // good, no collides

    ASSERT_TRUE(m.insert({ 0, 2 }, 30).second);   // C inserted at front

    ASSERT_THROW(m.insert({ 1, 6 },40), std::logic_error);   // THROW (ordering rule impossible)

    ASSERT_FALSE(m.insert({ 3, 5 },10).second); // collision with A
}

TEST(AvledetUtil, MonotonicMulti)
{
    avledet::util::mono::parallel_multi_map<
        int, // value
        int, int> m;

    ASSERT_TRUE(m.insert({ 3, 5 },10).second);   // A
    ASSERT_TRUE(m.insert({ 2, 5 }, 20).second);   // good, no collides

    ASSERT_TRUE(m.insert({ 0, 2 }, 30).second);   // C inserted at front

    ASSERT_THROW(m.insert({ 1, 6 },40), std::logic_error);   // THROW (ordering rule impossible)

    ASSERT_TRUE(m.insert({ 3, 5 },10).second); // good
    ASSERT_TRUE(m.insert({ 3, 5 },10).second); // good
    ASSERT_TRUE(m.insert({ 3, 5 },10).second); // good

    ASSERT_TRUE(m.insert({ 0, 2 },10).second); // good

    ASSERT_THROW(m.insert({ 2, 10 },40), std::logic_error);   // THROW (ordering rule impossible)
}
