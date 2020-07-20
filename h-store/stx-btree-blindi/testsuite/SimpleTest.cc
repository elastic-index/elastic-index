/*******************************************************************************
 * testsuite/SimpleTest.cc
 *
 * STX B+ Tree Test Suite v0.9
 * Copyright (C) 2008-2013 Timo Bingmann
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include <stx/btree_multimap.h>
#include <common/SimpleKeyExtractor.h>
#include "NaiveIntSerializer.h"

#include <cstdlib>
#include <inttypes.h>
#include <iostream>

#include "tpunit.h"

template <int Slots>
struct SimpleTest : public tpunit::TestFixture
{
    SimpleTest() : tpunit::TestFixture(
                       TEST(SimpleTest::test_empty),
                       TEST(SimpleTest::test_map_insert_erase_3200)
                       )
    { }

    template <typename KeyType>
    struct traits_nodebug : stx_blindi::btree_default_set_traits<KeyType>
    {
        static const bool selfverify = true;
        static const bool debug = false;

        static const int  leafslots = Slots;
        static const int  innerslots = Slots;
    };

    void test_map_insert_erase_3200()
    {
        typedef stx_blindi::btree_multimap<unsigned int, const unsigned int* ,SimpleKeyExtractor<unsigned int>,NaiveIntSerializer,
                                    std::less<unsigned int>, traits_nodebug<unsigned int> > btree_type;

        SimpleKeyExtractor<unsigned int> key_extractor;
        NaiveIntSerializer key_serializer;
        btree_type bt(key_extractor,key_serializer);

        srand(34234235);
        unsigned int data[3200];
        for (unsigned int i = 0; i < 3200; i++)
        {
            ASSERT(bt.size() == i);
            data[i] = (unsigned int)rand();
            unsigned int* value = (unsigned int*)(&data[i]);
            bt.insert2(data[i], (unsigned int*)(&data[i]));
            ASSERT(bt.size() == i + 1);
        }

        srand(34234235);
        for (unsigned int i = 0; i < 3200; i++)
        {
            ASSERT(bt.size() == 3200 - i);
            ASSERT(bt.erase_one(data[i]));
            ASSERT(bt.size() == 3200 - i - 1);
        }

        ASSERT(bt.empty());
//        bt.verify();
    }

    void test_empty()
    {
        typedef stx_blindi::btree_multimap<unsigned int, const unsigned int* ,SimpleKeyExtractor<unsigned int>,NaiveIntSerializer,
                std::less<unsigned int>, traits_nodebug<unsigned int> > btree_type;

        SimpleKeyExtractor<unsigned int> key_extractor;
        NaiveIntSerializer key_serializer;
        btree_type bt(key_extractor, key_serializer), bt2(key_extractor, key_serializer);

        ASSERT(bt.erase(42) == 0);

        ASSERT(bt == bt2);
    }
};

// test on different slot sizes
struct SimpleTest<8> _SimpleTest8;
struct SimpleTest<10> _SimpleTest10;
struct SimpleTest<12> _SimpleTest12;
struct SimpleTest<14> _SimpleTest14;
struct SimpleTest<16> _SimpleTest16;
struct SimpleTest<20> _SimpleTest20;
struct SimpleTest<24> _SimpleTest24;
struct SimpleTest<32> _SimpleTest32;
struct SimpleTest<48> _SimpleTest48;
struct SimpleTest<64> _SimpleTest64;
struct SimpleTest<128> _SimpleTest128;

/******************************************************************************/
