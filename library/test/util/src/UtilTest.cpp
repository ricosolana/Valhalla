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

// Remember, strict mapping requires that each positional key is individually unique across instances
//  - (1, 2)
//  - (0, 2)
//  Above results in a chash (second keys collide)
TEST(AvledetUtil, MonotonicStrict)
{
    avledet::util::mono::strict_btree_map<
        int, // value
        int, int> m;

    ASSERT_TRUE(m.insert({{ 3, 5 }, 10}).second);   // A
    ASSERT_FALSE(m.insert({{ 2, 5 }, 20 }).second);   // clash -> returns {itr,false}
    ASSERT_TRUE(m.insert({{ 0, 2 }, 30 }).second);   // C inserted at front
    ASSERT_THROW(m.insert({{ 1, 6 }, 40 }), std::logic_error);   // THROW (ordering rule impossible)
    ASSERT_EQ((m[{ 3, 5 }]), 10);
    ASSERT_EQ(m.find({ 3, 1 }), m.end());
    ASSERT_THROW((m[{ 3, 1 }]), std::logic_error);
    ASSERT_TRUE(m.find({ 3, 1 })->second);
}

// Weak mapping
//  Requires that 
TEST(AvledetUtil, MonotonicWeak)
{
    avledet::util::mono::weak_btree_map<
        int, // value
        int, int> m;

    ASSERT_TRUE(m.insert({{ 3, 5 }, 10 }).second);   // A
    ASSERT_TRUE(m.insert({{ 2, 5 }, 20 }).second);   // good, no collides
    ASSERT_TRUE(m.insert({{ 0, 2 }, 30 }).second);   // C inserted at front
    ASSERT_THROW(m.insert({{ 1, 6 }, 40 }), std::logic_error);   // THROW (ordering rule impossible)
    ASSERT_FALSE(m.insert({{ 3, 5 }, 10 }).second); // collision with A
    ASSERT_EQ((m[{ 3, 5 }]), 10);
    ASSERT_EQ(m.find({ 3, 1 }), m.end());
    ASSERT_THROW((m[{ 3, 1 }]), std::logic_error);
    ASSERT_TRUE(m.find({ 3, 1 })->second);
}

TEST(AvledetUtil, MonotonicMulti)
{
    // This is the weirdest container
    //  - duplicate keys are allowed
    //  - where should insert place keys?

    // behavior
    //  container manipulations will ALWAYS insert a new object (or throw if illegal order)
    //  container reads, however, will return the object nearest to the beginning
    // *NOTE
    //  - index operator[] functions as an insert then return the value-ref
    //      so, index[] will actually always create a new object, rendering it 
    //      semi-useless.
    //  - index[] op retrievals return Object default ctor (0),
    //  - while index[] assignment will always insert a new object
    // kinda interesting isnt it..? anyone?

    // this behavior is well defined,
    //  however very confusing and unexpected in usage.


    avledet::util::mono::multi_btree_map<
        int, // value
        int, int> m;

    ASSERT_TRUE(m.insert({{ 3, 5 }, 10 }).second);   // A
    ASSERT_TRUE(m.insert({{ 2, 5 }, 20 }).second);   // good, no collides
    ASSERT_TRUE(m.insert({{ 0, 2 }, 30 }).second);   // C inserted at front
    ASSERT_THROW(m.insert({{ 1, 6 }, 40 }), std::logic_error);   // THROW (ordering rule impossible)
    ASSERT_TRUE(m.insert({{ 3, 5 }, 50 }).second); // good
    ASSERT_TRUE(m.insert({{ 3, 5 }, 60 }).second); // good
    ASSERT_TRUE(m.insert({{ 3, 5 }, 70 }).second); // good
    ASSERT_TRUE(m.insert({{ 0, 2 }, 80 }).second); // good
    ASSERT_THROW(m.insert({{ 2, 10 }, 90 }), std::logic_error);   // THROW (ordering rule impossible)
    
    //
    ASSERT_EQ((m[{ 0, 2 }]), 0);
    m[{ 0, 2 }] = 10; // adds a new 0,2, because this is a duplicating map
    ASSERT_EQ((m[{ 0, 2 }]), 0); // idx[] operator
    ASSERT_TRUE(m.insert({{ 0, 2 }, 110 }).second);
    ASSERT_EQ(m.find({ 0 , 2 })->second, 110);
    ASSERT_TRUE(m.insert({{ 0, 2 }, 120 }).second);
    ASSERT_EQ(m.find({ 0 , 2 })->second, 120);
}
