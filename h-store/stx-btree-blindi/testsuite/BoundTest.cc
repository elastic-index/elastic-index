/*******************************************************************************
 * testsuite/BoundTest.cc
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
#include <stx/btree_map.h>

#include <common/SimpleKeyExtractor.h>
#include "NaiveIntSerializer.h"
#include <cstdlib>
#include <set>
#include <type_traits>
#include "tpunit.h"

template<int leaf_slots, int inner_slots>
struct BoundTest : public tpunit::TestFixture {
    BoundTest() : tpunit::TestFixture(
            TEST(BoundTest::test_3200_10),
            TEST(BoundTest::test_320_1000)
    ) {}

    template<typename KeyType>
    struct traits_nodebug : stx_blindi::btree_default_set_traits<KeyType> {
        static const bool selfverify = true;
        static const bool debug = false;

        static const int leafslots = leaf_slots;
        static const int innerslots = inner_slots;
    };

    void test_multi(const unsigned int insnum, const int modulo) {
        typedef stx_blindi::btree_multimap<unsigned int, unsigned int *, SimpleKeyExtractor<unsigned int>,
                NaiveIntSerializer, std::less<unsigned int>, traits_nodebug<unsigned int>> btree_type;

        SimpleKeyExtractor<unsigned int> key_extractor;
        NaiveIntSerializer key_serializer;

        auto bt = btree_type(key_extractor, key_serializer);

        typedef std::multiset<unsigned int> multiset_type;
        multiset_type set;

        // *** insert
        srand(34234235);
        unsigned int data[insnum];
        for (unsigned int i = 0; i < insnum; i++) {
            unsigned int k = (unsigned int) rand() % modulo;
            data[i] = k;
            unsigned int *v = data + i;

            ASSERT(bt.size() == set.size());
            bt.insert2(k, v);
            set.insert(k);
            ASSERT(bt.count(k) == set.count(k));

            ASSERT(bt.size() == set.size());
        }

        ASSERT(bt.size() == insnum);

        // *** iterate
        {
            auto bi = bt.begin();
            multiset_type::const_iterator si = set.begin();
            for (; bi != bt.end() && si != set.end(); ++bi, ++si) {
                ASSERT(*si == bi.key());
            }
            ASSERT(bi == bt.end());
            ASSERT(si == set.end());
        }

        // *** existence
        srand(34234235);
        for (unsigned int i = 0; i < insnum; i++) {
            unsigned int k = data[i];

            ASSERT(bt.exists(k));
        }

        // *** counting
        srand(34234235);
        for (unsigned int i = 0; i < insnum; i++) {
            unsigned int k = data[i];

            ASSERT(bt.count(k) == set.count(k));
        }

        // *** lower_bound
        for (int k = 0; k < modulo + 100; k++) {
            //   std::cout << k << '\n';
            multiset_type::const_iterator si = set.lower_bound(k);
            auto bi = bt.lower_bound(k);

            if (bi == bt.end())
                ASSERT(si == set.end());
            else if (si == set.end())
                ASSERT(bi == bt.end());
            else {
                // std::cout<< *si << ',';
                //std::cout<< bi.key() << '\n';

                ASSERT(*si == bi.key());
            }

        }

        // *** upper_bound
        for (int k = 0; k < modulo + 100; k++) {
            multiset_type::const_iterator si = set.upper_bound(k);
            auto bi = bt.upper_bound(k);

            if (bi == bt.end())
                ASSERT(si == set.end());
            else if (si == set.end())
                ASSERT(bi == bt.end());
            else
                ASSERT(*si == bi.key());
        }

        // *** equal_range
        for (int k = 0; k < modulo + 100; k++) {
            std::pair<multiset_type::const_iterator, multiset_type::const_iterator> si = set.equal_range(k);
            auto bi = bt.equal_range(k);

            if (bi.first == bt.end())
                ASSERT(si.first == set.end());
            else if (si.first == set.end())
                ASSERT(bi.first == bt.end());
            else
                ASSERT(*si.first == bi.first.key());

            if (bi.second == bt.end())
                ASSERT(si.second == set.end());
            else if (si.second == set.end())
                ASSERT(bi.second == bt.end());
            else
                ASSERT(*si.second == bi.second.key());
        }

        // *** deletion
        srand(34234235);
        for (unsigned int i = 0; i < insnum; i++) {
            unsigned int k = data[i];

            if (set.find(k) != set.end()) {
                ASSERT(bt.size() == set.size());

                ASSERT(bt.exists(k));
                ASSERT(bt.erase_one(k));
                set.erase(set.find(k));

                ASSERT(bt.size() == set.size());
            }
        }

        ASSERT(bt.empty());
        ASSERT(set.empty());
    }

    void test_3200_10() {
        test_multi(3200, 10);
    }

    void test_320_1000() {
        test_multi(320, 1000);
    }
};// BoundTest

// template order: <leafslots, innerslots, unique>
struct BoundTest<8, 8> Bound8_8;
struct BoundTest<16, 8> Bound16_8;
struct BoundTest<32, 8> Bound32_8;
struct BoundTest<64, 8> Bound64_8;
struct BoundTest<128, 8> Bound128_8;

/******************************************************************************/
