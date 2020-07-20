/*******************************************************************************
 * testsuite/IteratorTest.cc
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
#include <vector>

#include "tpunit.h"

template <int Slots>
struct IteratorTest : public tpunit::TestFixture
{
    IteratorTest() : tpunit::TestFixture(
                         TEST(IteratorTest::test_iterator2),
                         TEST(IteratorTest::test_iterator3)
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


    void test_iterator2() {
        typedef stx_blindi::btree_multimap<unsigned int, unsigned int *, SimpleKeyExtractor<unsigned int>,
        NaiveIntSerializer, std::less<unsigned int>, traits_nodebug<unsigned int>> btree_type;

        std::vector<typename btree_type::value_type> vector;
        unsigned int data[3200];
        SimpleKeyExtractor<unsigned int> key_extractor;
        NaiveIntSerializer key_serializer;

        std::vector<unsigned int> v(3200);
        std::iota(v.begin(), v.end(), 0);

        std::random_shuffle(v.begin(), v.end());

        for (unsigned int i = 0; i < 3200; i++) {
            data[i] = v.at(i);
            unsigned int * v = &data[i];
            vector.push_back(typename btree_type::value_type(data[i], v));
        }

        ASSERT(vector.size() == 3200);



        // test construction and insert(iter, iter) function
        btree_type bt(vector.begin(), vector.end(), key_extractor, key_serializer);
        ASSERT(bt.size() == 3200);

        // copy for later use
        btree_type bt2 = bt;

        // empty out the first bt
        for (unsigned int i = 0; i < 3200; i++)
        {
            ASSERT(bt.size() == 3200 - i);
            ASSERT(bt.erase_one(data[i]));
            ASSERT(bt.size() == 3200 - i - 1);
        }

        ASSERT(bt.empty());

        // copy btree values back to a vector
        std::vector<typename btree_type::value_type> vector2;
        vector2.assign(bt2.begin(), bt2.end());

        // after sorting the vector, the two must be the same
        std::sort(vector.begin(), vector.end());

        ASSERT(vector == vector2);

        // test reverse iterator
        vector2.clear();
        vector2.assign(bt2.rbegin(), bt2.rend());

        std::reverse(vector.begin(), vector.end());

        typename btree_type::reverse_iterator ri = bt2.rbegin();
        for (unsigned int i = 0; i < vector2.size(); ++i, ++ri) {
            ASSERT(vector[i].first == vector2[i].first);
            ASSERT(vector[i].first == ri->first);
            ASSERT(vector[i].second == ri->second);
        }

        ASSERT(ri == bt2.rend());
    }

        void test_iterator3()
    {
        typedef stx_blindi::btree_map<unsigned int, const unsigned int *, SimpleKeyExtractor<unsigned int>,
                NaiveIntSerializer, std::less<unsigned int>, traits_nodebug<unsigned int>> btree_type;

        SimpleKeyExtractor<unsigned int> key_extractor;
        NaiveIntSerializer key_serializer;
        btree_type map(key_extractor, key_serializer);

        unsigned int maxnum = 1000;
        unsigned int data[maxnum];

        for (unsigned int i = 0; i < maxnum; ++i)
        {
            data[i] = i;
            map.insert(std::make_pair(data[i], &data[i]));
        }

        {   // test iterator prefix++
            unsigned int nownum = 0;

            for (typename btree_type::iterator i = map.begin();
                 i != map.end(); ++i)
            {
                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);

                nownum++;
            }

            ASSERT(nownum == maxnum);
        }

        {   // test iterator prefix--
            unsigned int nownum = maxnum;

            typename btree_type::iterator i;
            for (i = --map.end(); i != map.begin(); --i)
            {
                nownum--;

                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);
            }

            nownum--;

            ASSERT(data[nownum] == i->first);
            ASSERT(&data[nownum] == i->second);

            ASSERT(nownum == 0);
        }

        {   // test const_iterator prefix++
            unsigned int nownum = 0;

            for (typename btree_type::const_iterator i = map.begin();
                 i != map.end(); ++i)
            {
                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);

                nownum++;
            }

            ASSERT(nownum == maxnum);
        }

        {   // test const_iterator prefix--
            unsigned int nownum = maxnum;

            typename btree_type::const_iterator i;
            for (i = --map.end(); i != map.begin(); --i)
            {
                nownum--;

                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);
            }

            nownum--;

            ASSERT(data[nownum] == i->first);
            ASSERT(&data[nownum] == i->second);

            ASSERT(nownum == 0);
        }

        {   // test reverse_iterator prefix++
            unsigned int nownum = maxnum;

            for (typename btree_type::reverse_iterator i = map.rbegin();
                 i != map.rend(); ++i)
            {
                nownum--;

                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);
            }

            ASSERT(nownum == 0);
        }

        {   // test reverse_iterator prefix--
            unsigned int nownum = 0;

            typename btree_type::reverse_iterator i;
            for (i = --map.rend(); i != map.rbegin(); --i)
            {
                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);

                nownum++;
            }

            ASSERT(data[nownum] == i->first);
            ASSERT(&data[nownum] == i->second);

            nownum++;

            ASSERT(nownum == maxnum);
        }

        {   // test const_reverse_iterator prefix++
            unsigned int nownum = maxnum;

            for (typename btree_type::const_reverse_iterator i = map.rbegin();
                 i != map.rend(); ++i)
            {
                nownum--;

                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);
            }

            ASSERT(nownum == 0);
        }

        {   // test const_reverse_iterator prefix--
            unsigned int nownum = 0;

            typename btree_type::const_reverse_iterator i;
            for (i = --map.rend(); i != map.rbegin(); --i)
            {
                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);

                nownum++;
            }

            ASSERT(data[nownum] == i->first);
            ASSERT(&data[nownum] == i->second);

            nownum++;

            ASSERT(nownum == maxnum);
        }

        // postfix

        {   // test iterator postfix++
            unsigned int nownum = 0;

            for (typename btree_type::iterator i = map.begin();
                 i != map.end(); i++)
            {
                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);

                nownum++;
            }

            ASSERT(nownum == maxnum);
        }

        {   // test iterator postfix--
            unsigned int nownum = maxnum;

            typename btree_type::iterator i;
            for (i = --map.end(); i != map.begin(); i--)
            {
                nownum--;

                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);
            }

            nownum--;

            ASSERT(data[nownum] == i->first);
            ASSERT(&data[nownum] == i->second);

            ASSERT(nownum == 0);
        }

        {   // test const_iterator postfix++
            unsigned int nownum = 0;

            for (typename btree_type::const_iterator i = map.begin();
                 i != map.end(); i++)
            {
                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);

                nownum++;
            }

            ASSERT(nownum == maxnum);
        }

        {   // test const_iterator postfix--
            unsigned int nownum = maxnum;

            typename btree_type::const_iterator i;
            for (i = --map.end(); i != map.begin(); i--)
            {
                nownum--;

                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);
            }

            nownum--;

            ASSERT(data[nownum] == i->first);
            ASSERT(&data[nownum] == i->second);

            ASSERT(nownum == 0);
        }

        {   // test reverse_iterator postfix++
            unsigned int nownum = maxnum;

            for (typename btree_type::reverse_iterator i = map.rbegin();
                 i != map.rend(); i++)
            {
                nownum--;

                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);
            }

            ASSERT(nownum == 0);
        }

        {   // test reverse_iterator postfix--
            unsigned int nownum = 0;

            typename btree_type::reverse_iterator i;
            for (i = --map.rend(); i != map.rbegin(); i--)
            {
                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);

                nownum++;
            }

            ASSERT(data[nownum] == i->first);
            ASSERT(&data[nownum] == i->second);

            nownum++;

            ASSERT(nownum == maxnum);
        }

        {   // test const_reverse_iterator postfix++
            unsigned int nownum = maxnum;

            for (typename btree_type::const_reverse_iterator i = map.rbegin();
                 i != map.rend(); i++)
            {
                nownum--;

                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);
            }

            ASSERT(nownum == 0);
        }

        {   // test const_reverse_iterator postfix--
            unsigned int nownum = 0;

            typename btree_type::const_reverse_iterator i;
            for (i = --map.rend(); i != map.rbegin(); i--)
            {
                ASSERT(data[nownum] == i->first);
                ASSERT(&data[nownum] == i->second);

                nownum++;
            }

            ASSERT(data[nownum] == i->first);
            ASSERT(&data[nownum] == i->second);

            nownum++;

            ASSERT(nownum == maxnum);
        }
    }


    void test_erase_iterator1()
    {
        typedef stx_blindi::btree_multimap<int, int *, SimpleKeyExtractor<int>,
                NaiveIntSerializer, std::less<int>, traits_nodebug<unsigned int>> btree_type;

        SimpleKeyExtractor<int> key_extractor;
        NaiveIntSerializer key_serializer;
        btree_type map(key_extractor, key_serializer);

        const int size1 = 32;
        const int size2 = 256;

        int data[size1][size2];

        for (int i = 0; i < size1; ++i)
        {
            for (int j = 0; j < size2; ++j)
            {
                data[i][j] = i;
                map.insert2(i, &data[i][j]);
            }
        }

        ASSERT(map.size() == size1 * size2);

        // erase in reverse order. that should be the worst case for
        // erase_iter()

        for (int i = size1 - 1; i >= 0; --i)
        {
            for (int j = size2 - 1; j >= 0; --j)
            {
                // find iterator
                typename btree_type::iterator it = map.find(i);

                while (it != map.end() && it.key() == i && it.data() != &data[i][j])
                    ++it;

                ASSERT(it.key() == i);
                ASSERT(it.data() == &data[i][j]);

                unsigned int mapsize = map.size();
                map.erase(it);
                ASSERT(map.size() == mapsize - 1);
            }
        }

        ASSERT(map.size() == 0);
    }


};

// test on different slot sizes
struct IteratorTest<8> _IteratorTest8;
struct IteratorTest<16> _IteratorTest16;
struct IteratorTest<32> _IteratorTest32;
struct IteratorTest<48> _IteratorTest48;
struct IteratorTest<64> _IteratorTest64;
struct IteratorTest<128> _IteratorTest128;

/******************************************************************************/
