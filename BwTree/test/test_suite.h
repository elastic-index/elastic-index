
/*
 * test_suite.cpp
 *
 * This files includes basic testing infrastructure and function declarations
 *
 * by Ziqi Wang
 */
//#define PRINT_RESULTS
//#define RUN_REMOVE
//#define GET_RANGE
//#define RANGE_SIZE 10 
//#define DEBUG_PRINT
#pragma once

#include <cstring>
#include <string>
#include <unordered_map>
#include <random>
#include <map>
#include <sstream>
#include <fstream>
#include <iostream>

#include <pthread.h>

#include "../src/bwtree.h"
#include "../benchmark/stx_btree/btree_multimap.h"
#include "../benchmark/stx_blindi/blindi_btree_hybrid_nodes_multimap.h"
#include "../benchmark/stx_blindi/blindi_btree_multimap_trie.h"
#include "../benchmark/stx_blindi/blindi_btree_multimap_seqtree.h"
#include "../benchmark/stx_blindi/blindi_btree_multimap_subtrie.h"
#include "../benchmark/libcuckoo/cuckoohash_map.hh"
#include "../benchmark/art/art.h"
#include "../benchmark/hot/hot/singlethreaded/HOTSingleThreaded.hpp"
#include "../benchmark/hot/idx/contenthelpers/IdentityKeyExtractor.hpp"
#include "../benchmark/hot/idx/contenthelpers/OptionalValue.hpp"
#include "../benchmark/skip_list/skip_list_seq.hpp"
#include "../benchmark/skip_list/skip_list_single_lock.hpp"
#include "../benchmark/skip_list/skip_list_lazy.hpp"
#include "../benchmark/skip_list/skip_list_lock_free.hpp"
#include "../benchmark/BTreeOLC/BTreeOLC.hpp"

#include "../benchmark/BTreeOLC/blindi_BTreeOLC_seqtree.hpp"
#include "../benchmark/BTreeOLC/blindi_BTreeOLC_trie.hpp"
#include "../benchmark/BTreeOLC/blindi_BTreeOLC_subtrie.hpp"

// #define PRINT_RESULTS 1

#ifdef BWTREE_PELOTON
using namespace peloton::index;
#else
using namespace wangziqi2013::bwtree;
#endif

#define BTREEOLC_PRINTF(...)  ;

using namespace stx;

struct String {
  uint8_t val[128];

public:
  bool operator<(const String& rhs) const {
    return memcmp(val, rhs.val, sizeof(val)) < 0;
  }

  bool operator==(const String& other) const {
    return memcmp(val, other.val, sizeof(val)) == 0;
  }
};

namespace std {
template<>
struct hash<String> {
public:
  size_t operator()(const String &s) const {
      size_t h = 0;
      for (unsigned int i = 0; i < sizeof(s.val); i++)
        h ^= std::hash<uint8_t>()(s.val[i]);
      return h;
  }
};
}

struct UUID {
  uint64_t val[2];

public:
  bool operator<(const UUID& rhs) const {
    if (val[1] == rhs.val[1])
      return val[0] < rhs.val[0];
    else
      return val[1] < rhs.val[1];
  }

  bool operator==(const UUID& other) const {
    return val[0] == other.val[0] && val[1] == other.val[1];
  }
};

namespace std {
template<>
struct hash<UUID> {
public:
    size_t operator()(const UUID &s) const
    {
        size_t h1 = std::hash<uint64_t>()(s.val[0]);
        size_t h2 = std::hash<uint64_t>()(s.val[1]);
        return h1 ^ h2;
    }
};
}

struct GenericIndex {
public:
    virtual std::string name(void) = 0;
    virtual void PrintStat(void) {}
    virtual void UpdateThreadLocal(int num_threads) {}
    virtual void AssignGCID(int thread_id) {}
    virtual void UnregisterThread(int thread_id) {}
    virtual void PrintStats(void) {}
};

template<typename KeyType, typename ValueType>
struct Index : GenericIndex {
public:
    virtual bool Insert(const KeyType& key, const ValueType& value) = 0;
    virtual void GetValue(const KeyType &key,
                          std::vector<ValueType> &value_list)  = 0;
};


template<typename KeyType, typename ValueType>
struct BwTreeType : Index<KeyType, ValueType> {
    BwTree<KeyType, ValueType> tree{true};

public:
    BwTreeType() {
        print_flag = false;
        // By default let is serve single thread (i.e. current one)
        // and assign gc_id = 0 to the current thread
        tree.UpdateThreadLocal(1);
        tree.AssignGCID(0);
    }

    std::string name(void) { return "BwTree"; }

    bool Insert(const KeyType& key, const ValueType& value) {
        return tree.Insert(key, value);
    }

    void GetValue(const KeyType &key,
                  std::vector<ValueType> &value_list) {
        tree.GetValue(key, value_list);
    }

    void PrintStat(void) {
        printf("Insert op = %lu; abort = %lu; abort rate = %lf\n",
               tree.insert_op_count.load(),
               tree.insert_abort_count.load(),
               (double)tree.insert_abort_count.load() / (double)tree.insert_op_count.load());

        printf("Delete op = %lu; abort = %lu; abort rate = %lf\n",
                tree.delete_op_count.load(),
                tree.delete_abort_count.load(),
                (double)tree.delete_abort_count.load() / (double)tree.delete_op_count.load());
    }

    void UpdateThreadLocal(int num_threads) {
        tree.UpdateThreadLocal(num_threads);
    }

    void AssignGCID(int thread_id) {
        tree.AssignGCID(thread_id);
    }

    void UnregisterThread(int thread_id) {
        tree.UnregisterThread(thread_id);
    }
};


template<typename KeyType, typename ValueType, template<typename, int> typename BlindiBtreeHybridType, int InnerSlots, int LeafSlots>
struct BlindiBTreeHybridNodes : Index<KeyType, ValueType> {
    blindi_btree_hybrid_nodes_multimap<KeyType, ValueType, BlindiBtreeHybridType, InnerSlots, LeafSlots> tree;
    std::string blindi_type;

public:
    BlindiBTreeHybridNodes(std::string type) : blindi_type(type) {}

    std::string name(void) { return blindi_type + "-" + std::to_string(InnerSlots) + "-" + std::to_string(LeafSlots); }

    void PrintStats(void) {
     auto stats = tree.get_stats();
     uint64_t capacity = stats.indexcapacity;
     uint64_t breathing_capacity = 0.0;
     uint64_t insert_breath = breathing_count;
#ifdef BREATHING_BLINDI_STSTS	
     breathing_capacity = sizeof(uint8_t*) * breathing_sum;
     capacity += breathing_capacity;
#endif
	auto avg_leaves = (int) ((stats.btreeleaves + stats.blindinodes1) *  stats.leafslots + 2 * stats.leafslots * stats.blindinodes2 + 4 * stats.leafslots * stats.blindinodes4 + 8 * stats.leafslots * stats.blindinodes8) /(stats.btreeleaves + stats.blindileaves);
        std::cout << "* itemcount:         " << stats.itemcount <<          ", leaves:            " << stats.leaves << std::endl;
	std::cout << "* btree_leaves:      " << stats.btreeleaves <<        ", blindi_leaves:     " << stats.blindileaves<< std::endl; 
	std::cout << "* blindi_node1:      " << stats.blindinodes1 <<       ", blindi_node2:      "  << stats.blindinodes2 << std::endl;
	std::cout << "* blindi_node4:      "  << stats.blindinodes4 <<      ", blindi_node8:      "  << stats.blindinodes8 << std::endl;
	std::cout << "* btree2blindi1:     " << stats.btree2blindi1trans << ", btree2blindi2:     " << stats.btree2blindi2trans << std::endl;
	std::cout << "* blindi1toblindi2:  " << stats.blindi1to2trans <<    ", blindi21toblindi4: " << stats.blindi2to4trans << std::endl;
	std::cout << "* blindi41toblindi8: " << stats.blindi4to8trans <<    ", stats_capacity:    " << stats.indexcapacity << std::endl;
	std::cout << "* first Compression item:  " << stats.firsttimeCompressitemscount << std::endl;
	std::cout << "* status:            " << stats.hybridstatus <<       ", must: " << stats.limit2MustCompress  << std::endl;
	std::cout << "* should com: " << stats.limit2ShouldCompress <<      ", ReturnToNormal     " << stats.limit2ReturmFromShouldCompress<< std::endl;
	std::cout << "* avgfill_leaves:    " << stats.avgfill_leaves() <<   ", avgileavessize:    " << stats.avgfill_leaves() * stats.leafslots << std::endl;
	std::cout << "* avg_leaves:        " << avg_leaves <<               ", leaves_%:          " << (stats.avgfill_leaves() * stats.leafslots)/ avg_leaves  << std::endl;
	std::cout << "* innernodes:        " << stats.innernodes <<         ", leafslots:         " << stats.leafslots << std::endl;
	std::cout << "* btree leaf size:   " << stats.btreeleafsize <<      ", blindi1_leaf_size: " << stats.blindileafsize << std::endl;
	std::cout << "* inner_node_size:   " << stats.nodesize << std::endl;
	std::cout<<  "* innernodes:        " << stats.innernodes <<         ", num_slot:          "<< stats.leafslots << std::endl;
	std::cout << "* breathing_sum:     " << breathing_sum <<            ", breathing_capacity " << breathing_capacity << std::endl;  
	std::cout << "* theoretical index size: " << capacity << std::endl;
        std::cout << ", avgBYTE:           " << 1.0 * capacity/ stats.itemcount << ", insert_breathing:  " << insert_breath <<  std::endl;

    }

    bool Insert(const KeyType& key, const ValueType& value) {
        tree.insert(key, (uint64_t)&key);
     return true;
    }


    void print_key (uint8_t *key) {
 	for (int j = sizeof(KeyType) - 1; j>= 0; j--) {
			printf ("%d ", key[j]);
	}
	printf ("  ");
    }
		

    void GetValue(const KeyType &key,
                  std::vector<ValueType> &value_list) {

#ifdef GET_RANGE
        std::vector<pair<KeyType, ValueType> > value_pair;
        auto it = tree.find(key);
        for (int i = 0; i < RANGE_SIZE; i++) {
            if (it == tree.end()) {
                #ifdef DEBUG_PRINT
                std::cout << "WARNING: scan reached the end of the index prematurely" << std::endl;
                #endif
                break;
            }
            value_pair.push_back(make_pair(it->first, it->second));
            it++;
        }
        #ifdef DEBUG_PRINT
        std::cout << "results of scan called with: range = " << RANGE_SIZE  << " key= "; 
        print_key((uint8_t *)&key);
        std::cout << "  res.size()=" << value_pair.size() << std::endl;
        for (auto i = 0; i < value_pair.size(); ++i) {
            std::cout << "returned_key: ";
            print_key((uint8_t *)&value_pair[i].first); 
//            if (value_pair[i].first != *((KeyType*)value_pair[i].second))
//                std::cout << "Error: scan returned an incorrect pair" << std::endl;
        }
        getchar();
        #endif



#else

        auto it = tree.find(key);
        if (it != tree.end())
            value_list.push_back(it->second);
#ifdef PRINT_RESULTS
        for (auto i = value_list.begin(); i != value_list.end(); ++i) {
		std::cout << *i << ' ';
         }
#endif
///////////////////// add  to check remove ///////////////////////////
#ifdef RUN_REMOVE
        bool remove_succeeded = tree.erase_one(key);
#ifdef PRINT_RESULTS
	std::cout << " remove succeeded " << remove_succeeded << std::endl;
	if(remove_succeeded == false)
	{
		std::cout << "\n we didn't remove anything " << std::endl;
	}
	else {
		it = tree.lower_bound(key);
		if (it != tree.end()) {
			std::cout << "\n position after remove " << it->second << std::endl;
		}
	}
#endif 
#endif
#endif
    }

};



template<typename KeyType, typename ValueType, int InnerSlots, int LeafSlots>
struct BTreeType : Index<KeyType, ValueType> {
    btree_multimap<KeyType, ValueType, InnerSlots, LeafSlots> tree;

public:
    std::string name(void) { return std::string("BTree") + "-" + std::to_string(InnerSlots) + "-" + std::to_string(LeafSlots); }

    void PrintStats(void) {
        auto stats = tree.get_stats();
        std::cout   << "STX BTree stats:\n";
        std::cout   << "* leaf size:    " << stats.leafsize << std::endl 	
                    << "* leaf num:     " << stats.leaves << std::endl
                    << "* leaf slots:   " << stats.leafslots << std::endl
                    << "* inner size:   " << stats.nodesize << std::endl
                    << "* inner num:    " << stats.innernodes << std::endl
                    << "* inner slots:  " << stats.innerslots << std::endl
                    << "* theoretical index size: " << (stats.leaves * stats.leafsize) + (stats.innernodes * stats.nodesize) << std::endl
                    << "* index capacity:         " << stats.indexcapacity << std::endl
                    << "* overall item count:     " << stats.itemcount << std::endl;
    }
    // void PrintStats(void) {
    //     auto stats = tree.get_stats();
    //     std::cout << "itemcount: " << stats.itemcount << " leaves: " << stats.leaves
    //               << " innernodes: " << stats.innernodes << " leafslots: " << stats.leafslots
    //               << " innerslots: " << stats.innerslots
    //               << " leaf size: " << stats.leafsize << " node size: " << stats.nodesize
    //               << " index size (theoretical): " << (stats.leaves * stats.leafsize) + (stats.innernodes * stats.nodesize) << "\n";
    // }

    bool Insert(const KeyType& key, const ValueType& value) {
        tree.insert(key, value);
        //static auto counter = 0;
    //     if (0) { // if ((counter++) % 1000000 == 1) {
		// auto stats = tree.get_stats();
	  //     std::cout << "itemcount: " << stats.itemcount << " leaves " << stats.leaves << " avgfill_leaves " << stats.avgfill_leaves() <<   " innernodes " << stats.innernodes << std::endl;
    //     std::cout << " leaf size: " << stats.leafsize << " node size: " << stats.nodesize
    //               << " index size (theoretical): " << (stats.leaves * stats.leafsize) + (stats.innernodes * stats.nodesize)
    //               << " indexcapacity: " << stats.indexcapacity
    //                << std::endl << std::endl;
    //     }
        return true;
    }

    void print_key (uint8_t *key) {
 	for (int j = sizeof(KeyType) - 1; j>= 0; j--) {
			printf ("%d ", key[j]);
	}
	printf ("  ");
    }
	



    void GetValue(const KeyType &key,
                  std::vector<ValueType> &value_list) {
#ifdef GET_RANGE
        std::vector<pair<KeyType, ValueType> > value_pair;
        auto it = tree.find(key);
        for (int i = 0; i < RANGE_SIZE; i++) {
            if (it == tree.end()) {
                #ifdef DEBUG_PRINT
                std::cout << "WARNING: scan reached the end of the index prematurely" << std::endl;
                #endif
                break;
            }
            value_pair.push_back(make_pair(it->first, it->second));
            it++;
        }
        #ifdef DEBUG_PRINT
        std::cout << "results of scan called with: range = " << RANGE_SIZE  << " key= "; 
        print_key((uint8_t *)&key);
        std::cout << "  res.size()=" << value_pair.size() << std::endl;
        for (auto i = 0; i < value_pair.size(); ++i) {
            std::cout << "returned_key: ";
            print_key((uint8_t *)&value_pair[i].first); 
//            if (value_pair[i].first != *((KeyType*)value_pair[i].second))
//                std::cout << "Error: scan returned an incorrect pair" << std::endl;
        }
        getchar();
        #endif



#else
        auto it = tree.lower_bound(key);
        if (it != tree.end())
            value_list.push_back(it->second);
#ifdef PRINT_RESULTS
        for (auto i = value_list.begin(); i != value_list.end(); ++i) {
		std::cout << *i << ' ';
         }
#endif
///////////////////// add  to check remove ///////////////////////////
#ifdef RUN_REMOVE
        bool remove_succeeded = tree.erase_one(key);
#ifdef PRINT_RESULTS
	std::cout << " remove succeeded " << remove_succeeded << std::endl;
	if(remove_succeeded == false)
	{
		std::cout << "\n we didn't remove anything " << std::endl;
	}
	else {
		it = tree.lower_bound(key);
		if (it != tree.end()) {
			std::cout << "\n position after remove " << it->second << std::endl;
		}
	}
#endif 
#endif
#endif

    }

     void GetRange(const KeyType &key,
                  std::vector<ValueType> &value_list) {
        auto it = tree.lower_bound(key);
        for (int i=0; i < 1; i++) {
		if (it == tree.end())
			break;
		value_list.push_back(it->second);
		it++;
	}
    }
};

template<typename KeyType, typename ValueType, template<typename, int> typename BlindiTrieType, int InnerSlots, int LeafSlots>
struct BlindiBTreeTrieType : Index<KeyType, ValueType> {
    blindi_btree_multimap_trie<KeyType, ValueType, BlindiTrieType, InnerSlots, LeafSlots> tree;
    std::string blindi_type;

public:
    BlindiBTreeTrieType(std::string type) : blindi_type(type) {}

    std::string name(void) { return blindi_type + "-" + std::to_string(InnerSlots) + "-" + std::to_string(LeafSlots); }

    void PrintStats(void) {
        auto stats = tree.get_stats();
        std::cout << "itemcount: " << stats.itemcount << " leaves: " << stats.leaves
                  << " innernodes: " << stats.innernodes << " leafslots: " << stats.leafslots
                  << " innerslots: " << stats.innerslots
                  << " leaf size: " << stats.leafsize << " node size: " << stats.nodesize
                  << " index size (theoretical): " << (stats.leaves * stats.leafsize) + (stats.innernodes * stats.nodesize) << "\n";
    }

    bool Insert(const KeyType& key, const ValueType& value) {
        tree.insert(key, value);
//        auto stat = tree.get_stats();
//        std::cout << "itemcount: " << stat.itemcount << " leaves " << stat.leaves << " avgfill_leaves " << stat.avgfill_leaves() <<   " innernodes " << stat.innernodes <<  std::endl;
        return true;
    }

    void GetValue(const KeyType &key,
                  std::vector<ValueType> &value_list) {
        auto it = tree.find(key);
        if (it != tree.end())
            value_list.push_back(it->second);
#ifdef PRINT_RESULTS
        for (auto i = value_list.begin(); i != value_list.end(); ++i) {
		std::cout << *i << ' ';
         }
#endif
///////////////////// add  to check remove //////////////////////////
#ifdef RUN_REMOVE
        bool remove_succeeded = tree.erase_one(key);
#ifdef PRINT_RESULTS
	std::cout << " remove succeeded " << remove_succeeded << std::endl;
	if(remove_succeeded == false)
	{
		std::cout << "\n we didn't remove anything " << std::endl;
	}
	else {
		it = tree.lower_bound(key);
		if (it != tree.end()) {
			std::cout << "\n position after remove " << it->second << std::endl;
		}
	}
#endif
#endif
    }

    void GetRange(const KeyType &key,
                  std::vector<ValueType> &value_list) {
        auto it = tree.upper_bound(key);
	for (int i=0; i < 1; i++) {
		if (it == tree.end())
			break;
		value_list.push_back(it->second);
		it++;
	}
    }
};


template<typename KeyType, typename ValueType, template<typename, int> typename BlindiSeqTreeType, int InnerSlots, int LeafSlots>
struct BlindiBTreeSeqTreeType : Index<KeyType, ValueType> {
    blindi_btree_multimap_seqtree<KeyType, ValueType, BlindiSeqTreeType, InnerSlots, LeafSlots> tree;
    std::string blindi_type;

public:
    BlindiBTreeSeqTreeType(std::string type) : blindi_type(type) {}

    std::string name(void) { return blindi_type + "-" + std::to_string(InnerSlots) + "-" + std::to_string(LeafSlots); }

    void PrintStats(void) {
        auto stats = tree.get_stats();
	uint64_t capacity = 0.0;
	uint64_t breathing_capacity = 0.0;
	uint64_t insert_breath = breathing_count;
#ifdef BREATHING_BLINDI_STSTS	
	breathing_capacity = sizeof(uint8_t*) * breathing_sum;
	capacity = stats.indexcapacity + breathing_capacity;
#endif
        std::cout   << "STX Seqtree stats:\n";
        std::cout   << "* leaf size:    " << stats.leafsize << std::endl 	
                    << "* leaf num:     " << stats.leaves << std::endl
                    << "* leaf slots:   " << stats.leafslots << std::endl
                    << "* inner size:   " << stats.nodesize << std::endl
                    << "* inner num:    " << stats.innernodes << std::endl
                    << "* inner slots:  " << stats.innerslots << std::endl
                    << "* theoretical index size: " << (stats.leaves * stats.leafsize) + (stats.innernodes * stats.nodesize) << std::endl
                    << "* index capacity:         " << stats.indexcapacity << std::endl
                    << "* overall item count:     " << stats.itemcount << std::endl
	            << " breathing_sum: " << breathing_sum << ", breathing_capacity " << breathing_capacity << std::endl  
 	            << " total_capacity: " << capacity   << std::endl
                    << " avgBYTE: " << 1.0 * capacity/ stats.itemcount << std::endl
		    << " insert_breathing: " << insert_breath <<  std::endl;




    }
    // void PrintStats(void) {
    //     auto stats = tree.get_stats();
    //     std::cout << "itemcount: " << stats.itemcount << " leaves: " << stats.leaves
    //               << " innernodes: " << stats.innernodes << " leafslots: " << stats.leafslots
    //               << " innerslots: " << stats.innerslots
    //               << " leaf size: " << stats.leafsize << " node size: " << stats.nodesize
    //               << " index size (theoretical): " << (stats.leaves * stats.leafsize) + (stats.innernodes * stats.nodesize) << "\n";
    // }

    bool Insert(const KeyType& key, const ValueType& value) {
        tree.insert(key, value);
//        auto stat = tree.get_stats();
//	std::cout << "itemcount: " << stat.itemcount << " leaves " << stat.leaves << " avgfill_leaves " << stat.avgfill_leaves() <<   " innernodes " << stat.innernodes <<  std::endl;
	return true;
    }

    bool Erase_one(const KeyType& key, const ValueType& value) {
        tree.erase_one(key);
        return true;
    }

    void print_key (uint8_t *key) {
 	for (int j = sizeof(KeyType) - 1; j>= 0; j--) {
			printf ("%d ", key[j]);
	}
	printf ("  ");
    }
	



    void GetValue(const KeyType &key,
                  std::vector<ValueType> &value_list) {

#ifdef GET_RANGE
        std::vector<pair<KeyType, ValueType> > value_pair;
        auto it = tree.find(key);
        for (int i = 0; i < RANGE_SIZE; i++) {
            if (it == tree.end()) {
                #ifdef DEBUG_PRINT
                std::cout << "WARNING: scan reached the end of the index prematurely" << std::endl;
                #endif
                break;
            }
            value_pair.push_back(make_pair(it->first, it->second));
            it++;
        }
        #ifdef DEBUG_PRINT
        std::cout << "results of scan called with: range = " << RANGE_SIZE  << " key= "; 
        print_key((uint8_t *)&key);
        std::cout << "  res.size()=" << value_pair.size() << std::endl;
        for (auto i = 0; i < value_pair.size(); ++i) {
            std::cout << "returned_key: ";
            print_key((uint8_t *)&value_pair[i].first); 
//            if (value_pair[i].first != *((KeyType*)value_pair[i].second))
//                std::cout << "Error: scan returned an incorrect pair" << std::endl;
        }
        getchar();
        #endif
#else



        auto it = tree.find(key);
        if (it != tree.end())
            value_list.push_back(it->second);   
    #ifdef PRINT_RESULTS
    for (auto i = value_list.begin(); i != value_list.end(); ++i) {
      std::cout << *i << ' ';
    }
    #endif            

#ifdef PRINT_RESULTS
        for (auto i = value_list.begin(); i != value_list.end(); ++i) {
		std::cout << *i << ' ';
         }
#endif
///////////////////// add  to check remove //////////////////////////
#ifdef RUN_REMOVE
        bool remove_succeeded = tree.erase_one(key);
#ifdef PRINT_RESULTS
	std::cout << " remove succeeded " << remove_succeeded << std::endl;
	if(remove_succeeded == false)
	{
		std::cout << "\n we didn't remove anything " << std::endl;
	}
	else {
		it = tree.lower_bound(key);
		if (it != tree.end()) {
			std::cout << "\n position after remove " << it->second << std::endl;
		}
	}
#endif
#endif
#endif
    }

    void GetRange(const KeyType &key,
                  std::vector<ValueType> &value_list) {
        auto it = tree.upper_bound(key);
	for (int i=0; i < 1; i++) {
		if (it == tree.end())
			break;
		value_list.push_back(it->second);
		it++;
	}
    }
};


template<typename KeyType, typename ValueType, template<typename, int> typename BlindiSubTrieType, int InnerSlots, int LeafSlots>
struct BlindiBTreeSubTrieType : Index<KeyType, ValueType> {
    blindi_btree_multimap_subtrie<KeyType, ValueType, BlindiSubTrieType, InnerSlots, LeafSlots> tree;
    std::string blindi_type;

public:
    BlindiBTreeSubTrieType(std::string type) : blindi_type(type) {}

    std::string name(void) { return blindi_type + "-" + std::to_string(InnerSlots) + "-" + std::to_string(LeafSlots); }

    void PrintStats(void) {
        auto stats = tree.get_stats();
        std::cout << "itemcount: " << stats.itemcount << " leaves: " << stats.leaves
                  << " innernodes: " << stats.innernodes << " leafslots: " << stats.leafslots
                  << " innerslots: " << stats.innerslots
                  << " leaf size: " << stats.leafsize << " node size: " << stats.nodesize
                  << " index size (theoretical): " << (stats.leaves * stats.leafsize) + (stats.innernodes * stats.nodesize) << "\n";
    }

    bool Insert(const KeyType& key, const ValueType& value) {
        tree.insert(key, value);
//        auto stat = tree.get_stats();
//        std::cout << "itemcount: " << stat.itemcount << " leaves " << stat.leaves << " avgfill_leaves " << stat.avgfill_leaves() <<   " innernodes " << stat.innernodes <<  std::endl;
	return true;
    }

    void GetValue(const KeyType &key,
                  std::vector<ValueType> &value_list) {
        auto it = tree.find(key);
        if (it != tree.end())
            value_list.push_back(it->second);
#ifdef PRINT_RESULTS
        for (auto i = value_list.begin(); i != value_list.end(); ++i) {
		std::cout << *i << ' ';
         }
#endif
///////////////////// add  to check remove ///////////////////////////
#ifdef RUN_REMOVE
        bool remove_succeeded = tree.erase_one(key);
#ifdef PRINT_RESULTS
	std::cout << " remove succeeded " << remove_succeeded << std::endl;
	if(remove_succeeded == false)
	{
		std::cout << "\n we didn't remove anything " << std::endl;
	}
	else {
		it = tree.lower_bound(key);
		if (it != tree.end()) {
			std::cout << "\n position after remove " << it->second << std::endl;
		}
	}

#endif
#endif
    }

    void GetRange(const KeyType &key,
                  std::vector<ValueType> &value_list) {
        auto it = tree.upper_bound(key);
	for (int i=0; i < 1; i++) {
		if (it == tree.end())
			break;
		value_list.push_back(it->second);
		it++;
	}
    }
};



template<typename KeyType, typename ValueType>
struct ARTType : Index<KeyType, ValueType> {
    art_tree t;

public:
    ARTType() {
        art_tree_init(&t);
    }

    std::string name(void) { return "ART"; }

    bool Insert(const KeyType &key, const ValueType &value) {
        return !art_insert(&t, (const unsigned char *)&key, sizeof(key), (void *)&value);
    }

    void GetValue(const KeyType &key,
                  std::vector<ValueType> &value_list) {
        ValueType* v = (ValueType *)art_search(&t, (unsigned char *)&key, sizeof(key));
        if (v)
            value_list.push_back(*v);
#ifdef PRINT_RESULTS
        for (auto i = value_list.begin(); i != value_list.end(); ++i) {
		std::cout << *i << ' ';
         }
#endif
    }
};

template<typename KeyType, typename ValueType>
struct HOTType : Index<KeyType, ValueType> {

#ifdef USE_HOT
    template<typename Val>
    struct KeyExtractor {

        inline KeyType operator()(Val const &ptr) const {
                return *((KeyType*)ptr);
        }
   };

    hot::singlethreaded::HOTSingleThreaded<void*, KeyExtractor> hot;
#endif

public:
    std::string name(void) { return "HOT"; }

    bool Insert(const KeyType &key, const ValueType &value) {
#ifdef USE_HOT
        return hot.insert((void*) &key);
#else
        return true;
#endif
    }

    void print_key (uint8_t *key) {
 	for (int j = sizeof(KeyType) - 1; j>= 0; j--) {
			printf ("%d ", key[j]);
	}
	printf ("  ");
    }
	
    void GetValue(const KeyType &key,
                  std::vector<ValueType> &value_list) {
#ifdef USE_HOT
#ifdef GET_RANGE 
        auto it = hot.lower_bound(key);
        std::vector<pair<KeyType, KeyType*> > value_pair;
//        std::vector< KeyType*>  value_pair;
	for (int i = 0; i < RANGE_SIZE;  ++i) {
            if (it == hot.end())
                break;
            auto tmp = (KeyType*)*it;
            value_pair.push_back(make_pair(*tmp, tmp)); 
//            value_pair.push_back(tmp); 
            ++it;
      }
        #ifdef DEBUG_PRINT
        std::cout << "results of scan called with: range = " << RANGE_SIZE  << " key= "; 
        print_key((uint8_t *)&key);
        std::cout << "  res.size()=" << value_pair.size() << std::endl;
        for (auto i = 0; i < value_pair.size(); ++i) {
            std::cout << "returned_key: ";
            print_key((uint8_t *)&value_pair[i].first); 
//            if (value_pair[i].first != *((KeyType*)value_pair[i].second))
//                std::cout << "Error: scan returned an incorrect pair" << std::endl;
        }
 
        getchar();
        #endif
#else
        auto res = hot.lookup(key);
        if (res.mIsValid) {
            KeyType* k = (KeyType *) res.mValue;
            ValueType* v = (ValueType *)(k + 1);
            value_list.push_back(*v);
        }
#endif
#endif
    }

};

template<typename KeyType, typename ValueType>
struct SeqSkipListType : Index<KeyType, ValueType> {
    skip_list_seq<KeyType, ValueType> lst;

public:
    std::string name(void) { return std::string("Sequential skip list"); }

    // void PrintStats(void) {
    //   ;
    // }

    bool Insert(const KeyType& key, const ValueType& value) {
        return lst.add(key, value);;
    }

    void GetValue(const KeyType &key,
                  std::vector<ValueType> &value_list) {
        auto it = lst.find(key);
        if (it != nullptr)
          value_list.push_back(it->data);
    }
};


template<typename KeyType, typename ValueType>
struct SingleLockSkipListType : Index<KeyType, ValueType> {
    skip_list_single_lock<KeyType, ValueType> lst;

public:
    std::string name(void) { return std::string("Skip list (single lock)"); }

    // void PrintStats(void) {
    //   ;
    // }

    bool Insert(const KeyType& key, const ValueType& value) {
        return lst.add(key, value);;
    }

    void GetValue(const KeyType &key,
                  std::vector<ValueType> &value_list) {
        auto it = lst.find(key);
        if (it != nullptr)
          value_list.push_back(it->data);
    }
};

template<typename KeyType, typename ValueType>
struct LazySkipListType : Index<KeyType, ValueType> {
    skip_list_lazy<KeyType, ValueType> lst;

public:
    std::string name(void) { return std::string("Lazy skip list"); }

    // void PrintStats(void) {
    //   ;
    // }

    bool Insert(const KeyType& key, const ValueType& value) {
        return lst.add(key, value);;
    }

    void GetValue(const KeyType &key,
                  std::vector<ValueType> &value_list) {
        auto it = lst.find(key);
        if (it != nullptr)
          value_list.push_back(it->data);
    }
};

template<typename KeyType, typename ValueType>
struct LockFreeSkipListType : Index<KeyType, ValueType> {
    skip_list_lock_free<KeyType, ValueType> lst;

public:
    std::string name(void) { return std::string("Lock-free skip list"); }

    // void PrintStats(void) {
    //   ;
    // }

    bool Insert(const KeyType& key, const ValueType& value) {
        return lst.add(key, value);;
    }

    void GetValue(const KeyType &key,
                  std::vector<ValueType> &value_list) {
        auto it = lst.find(key);
        if (it != nullptr)
          value_list.push_back(it->data);
    }
};

template <typename KeyType, typename ValueType,  uint64_t InnerSlots, uint64_t LeafSlots>
struct BTreeOLCType : Index<KeyType, ValueType> {
  btreeolc::BTree<KeyType, ValueType, InnerSlots, LeafSlots> btree;

 public:
  std::string name(void) { return std::string("B-Tree OLC"); }

  void PrintStats() {
      #ifdef BTREEOLC_DEBUG
      auto stats = btree.get_stats();
      uint64_t leaf_size = stats.leaf_size;
      uint64_t leaf_num = stats.leaf_num;
      uint64_t leaf_slots = stats.leaf_slots;
      uint64_t inner_size = stats.inner_size;
      uint64_t inner_num = stats.inner_num;
      uint64_t inner_slots = stats.inner_slots;
      std::cout   << "BTreeOLC stats:\n";
      std::cout   << "* leaf size:    " << leaf_size << std::endl 	
                  << "* leaf num:     " << leaf_num << std::endl
                  << "* leaf slots:   " << leaf_slots << std::endl
                  << "* inner size:   " << inner_size << std::endl
                  << "* inner num:    " << inner_num << std::endl
                  << "* inner slots:  " << inner_slots << std::endl
                  << "* theoretical index size: " << (leaf_size*leaf_num+inner_size*inner_num) << std::endl;
      #else
      printf("\nINDEX STATS REQUESTED: N/A\n");
      #endif
  }

  bool Insert(const KeyType &key, const ValueType &value) {
    BTREEOLC_PRINTF("  BENCHMARK ASKS TO INSERT (%p, %p)\n", &key, &value);
    btree.insert(key, value);
    return true;
  }

  void GetValue(const KeyType &key, std::vector<ValueType> &value_list) {
    BTREEOLC_PRINTF("  BENCHMARK ASKS TO LOOK UP %p\n", &key);
    ValueType output;
    bool res = btree.lookup(key, output);
    if (res)
      value_list.push_back(output);
    #ifdef PRINT_RESULTS
    for (auto i = value_list.begin(); i != value_list.end(); ++i) {
      std::cout << *i << ' ';
    }
    #endif
  }
};

template <typename KeyType, typename ValueType, uint64_t InnerSlots, uint64_t LeafSlots>
struct BlindiBTreeOLCType : Index<KeyType, ValueType> {
  blindi::BTree<KeyType, ValueType, InnerSlots, LeafSlots> btree;

public:
  std::string name(void) { return std::string("Blindi B-Tree OLC"); }

  void PrintStats() {
      #ifdef BTREEOLC_DEBUG
      auto stats = btree.get_stats();
      uint64_t leaf_size = stats.leaf_size;
      uint64_t leaf_num = stats.leaf_num;
      uint64_t leaf_slots = stats.leaf_slots;
      uint64_t inner_size = stats.inner_size;
      uint64_t inner_num = stats.inner_num;
      uint64_t inner_slots = stats.inner_slots;
      std::cout   << "BTreeOLC stats:\n";
      std::cout   << "* leaf size:    " << leaf_size << std::endl 	
                  << "* leaf num:     " << leaf_num << std::endl
                  << "* leaf slots:   " << leaf_slots << std::endl
                  << "* inner size:   " << inner_size << std::endl
                  << "* inner num:    " << inner_num << std::endl
                  << "* inner slots:  " << inner_slots << std::endl
                  << "* theoretical index size: " << (leaf_size*leaf_num+inner_size*inner_num) << std::endl;
      #else
      printf("\nINDEX STATS REQUESTED: N/A\n");
      #endif
  }

  bool Insert(const KeyType &key, const ValueType &value) {
    BTREEOLC_PRINTF("  BENCHMARK ASKS TO INSERT (%p, %p)\n", &key, &value);
    btree.insert(key, value);
    return true;
  }

  void GetValue(const KeyType &key, std::vector<ValueType> &value_list) {
    BTREEOLC_PRINTF("  BENCHMARK ASKS TO LOOK UP %p\n", &key);
    ValueType output;
    bool res = btree.lookup(key, output);
    if (res)
      value_list.push_back(output);
    #ifdef PRINT_RESULTS
    for (auto i = value_list.begin(); i != value_list.end(); ++i) {
      std::cout << *i << ' ';
    }
    #endif 
  }
};

template <typename KeyType, typename ValueType, uint64_t InnerSlots, uint64_t LeafSlots>
struct BlindiTrieBTreeOLCType : Index<KeyType, ValueType> {
  blindi_trie::BTree<KeyType, ValueType, InnerSlots, LeafSlots> btree;

public:
  std::string name(void) { return std::string("Blindi B-Tree OLC"); }

  // void PrintStats(void) {
  //   ;
  // }

  bool Insert(const KeyType &key, const ValueType &value) {
    BTREEOLC_PRINTF("  BENCHMARK ASKS TO INSERT (%p, %p)\n", &key, &value);
    btree.insert(key, value);
    return true;
  }

  void GetValue(const KeyType &key, std::vector<ValueType> &value_list) {
    BTREEOLC_PRINTF("  BENCHMARK ASKS TO LOOK UP %p\n", &key);
    ValueType output;
    bool res = btree.lookup(key, output);
    if (res)
      value_list.push_back(output);
    #ifdef PRINT_RESULTS
    for (auto i = value_list.begin(); i != value_list.end(); ++i) {
      std::cout << *i << ' ';
    }
    #endif
  }
};

template <typename KeyType, typename ValueType, uint64_t InnerSlots, uint64_t LeafSlots>
struct BlindiSubtrieBTreeOLCType : Index<KeyType, ValueType> {
  blindi_subtrie::BTree<KeyType, ValueType, InnerSlots, LeafSlots> btree;

public:
  std::string name(void) { return std::string("Blindi B-Tree OLC"); }

  // void PrintStats(void) {
  //   ;
  // }

  bool Insert(const KeyType &key, const ValueType &value) {
    BTREEOLC_PRINTF("  BENCHMARK ASKS TO INSERT (%p, %p)\n", &key, &value);
    btree.insert(key, value);
    return true;
  }

  void GetValue(const KeyType &key, std::vector<ValueType> &value_list) {
    BTREEOLC_PRINTF("  BENCHMARK ASKS TO LOOK UP %p\n", &key);
    ValueType output;
    bool res = btree.lookup(key, output);
    if (res)
      value_list.push_back(output);
    #ifdef PRINT_RESULTS
    for (auto i = value_list.begin(); i != value_list.end(); ++i) {
      std::cout << *i << ' ';
    }
    #endif
  }
};

/*
 * Common Infrastructure
 */

/*
 * LaunchParallelTestID() - Starts threads on a common procedure
 *
 * This function is coded to be accepting variable arguments
 *
 * NOTE: Template function could only be defined in the header
 *
 * tree_p is used to allocate thread local array for doing GC. In the meanwhile
 * if it is nullptr then we know we are not using BwTree, so just ignore this
 * argument
 */
template <typename Fn, typename... Args>
void LaunchParallelTestID(GenericIndex* tree_p,
                          uint64_t num_threads,
                          Fn &&fn,
                          Args &&...args) {
  std::vector<std::thread> thread_group;

  if(tree_p != nullptr) {
    // Update the GC array
    tree_p->UpdateThreadLocal(num_threads);
  }

  auto fn2 = [tree_p, &fn](uint64_t thread_id, Args ...args) {
    if(tree_p != nullptr) {
      tree_p->AssignGCID(thread_id);
    }

    fn(thread_id, args...);

    if(tree_p != nullptr) {
      // Make sure it does not stand on the way of other threads
      tree_p->UnregisterThread(thread_id);
    }

    return;
  };

  // Launch a group of threads
  for (uint64_t thread_itr = 0; thread_itr < num_threads; ++thread_itr) {
    thread_group.push_back(std::thread{fn2, thread_itr, std::ref(args...)});
  }

  // Join the threads with the main thread
  for (uint64_t thread_itr = 0; thread_itr < num_threads; ++thread_itr) {
    thread_group[thread_itr].join();
  }

  // Restore to single thread mode after all threads have finished
  if(tree_p != nullptr) {
    tree_p->UpdateThreadLocal(1);
  }

  return;
}

/*
 * class Random - A random number generator
 *
 * This generator is a template class letting users to choose the number
 *
 * Note that this object uses C++11 library generator which is slow, and super
 * non-scalable.
 *
 * NOTE 2: lower and upper are closed interval!!!!
 */
template <typename IntType>
class Random {
 private:
  std::random_device device;
  std::default_random_engine engine;
  std::uniform_int_distribution<IntType> dist;

 public:

  /*
   * Constructor - Initialize random seed and distribution object
   */
  Random(IntType lower, IntType upper) :
    device{},
    engine{device()},
    dist{lower, upper}
  {}

  /*
   * Get() - Get a random number of specified type
   */
  inline IntType Get() {
    return dist(engine);
  }

  /*
   * operator() - Grammar sugar
   */
  inline IntType operator()() {
    return Get();
  }
};

/*
 * class SimpleInt64Random - Simple paeudo-random number generator
 *
 * This generator does not have any performance bottlenect even under
 * multithreaded environment, since it only uses local states. It hashes
 * a given integer into a value between 0 - UINT64T_MAX, and in order to derive
 * a number inside range [lower bound, upper bound) we should do a mod and
 * addition
 *
 * This function's hash method takes a seed for generating a hashing value,
 * together with a salt which is used to distinguish different callers
 * (e.g. threads). Each thread has a thread ID passed in the inlined hash
 * method (so it does not pose any overhead since it is very likely to be
 * optimized as a register resident variable). After hashing finishes we just
 * normalize the result which is evenly distributed between 0 and UINT64_T MAX
 * to make it inside the actual range inside template argument (since the range
 * is specified as template arguments, they could be unfold as constants during
 * compilation)
 *
 * Please note that here upper is not inclusive (i.e. it will not appear as the
 * random number)
 */
template <uint64_t lower, uint64_t upper>
class SimpleInt64Random {
 public:

  /*
   * operator()() - Mimics function call
   *
   * Note that this function must be denoted as const since in STL all
   * hashers are stored as a constant object
   */
  inline uint64_t operator()(uint64_t value, uint64_t salt) const {
    //
    // The following code segment is copied from MurmurHash3, and is used
    // as an answer on the Internet:
    // http://stackoverflow.com/questions/5085915/what-is-the-best-hash-
    //   function-for-uint64-t-keys-ranging-from-0-to-its-max-value
    //
    // For small values this does not actually have any effect
    // since after ">> 33" all its bits are zeros
    //value ^= value >> 33;
    value += salt;
    value *= 0xff51afd7ed558ccd;
    value ^= value >> 33;
    value += salt;
    value *= 0xc4ceb9fe1a85ec53;
    value ^= value >> 33;

    return lower + value % (upper - lower);
  }
};

/*
 * class Timer - Measures time usage for testing purpose
 */
class Timer {
 private:
  std::chrono::time_point<std::chrono::system_clock> start;
  std::chrono::time_point<std::chrono::system_clock> end;

 public:

  /*
   * Constructor
   *
   * It takes an argument, which denotes whether the timer should start
   * immediately. By default it is true
   */
  Timer(bool start = true) :
    start{},
    end{} {
    if(start == true) {
      Start();
    }

    return;
  }

  /*
   * Start() - Starts timer until Stop() is called
   *
   * Calling this multiple times without stopping it first will reset and
   * restart
   */
  inline void Start() {
    start = std::chrono::system_clock::now();

    return;
  }

  /*
   * Stop() - Stops timer and returns the duration between the previous Start()
   *          and the current Stop()
   *
   * Return value is represented in double, and is seconds elapsed between
   * the last Start() and this Stop()
   */
  inline double Stop() {
    end = std::chrono::system_clock::now();

    return GetInterval();
  }

  /*
   * GetInterval() - Returns the length of the time interval between the latest
   *                 Start() and Stop()
   */
  inline double GetInterval() const {
    std::chrono::duration<double> elapsed_seconds = end - start;
    return elapsed_seconds.count();
  }
};

/*
 * class Envp() - Reads environmental variables
 */
class Envp {
 public:
  /*
   * Get() - Returns a string representing the value of the given key
   *
   * If the key does not exist then just use empty string. Since the value of
   * an environmental key could not be empty string
   */
  static std::string Get(const std::string &key) {
    char *ret = getenv(key.c_str());
    if(ret == nullptr) {
      return std::string{""};
    }

    return std::string{ret};
  }

  /*
   * operator() - This is called with an instance rather than class name
   */
  std::string operator()(const std::string &key) const {
    return Envp::Get(key);
  }

  /*
   * GetValueAsUL() - Returns the value by argument as unsigned long
   *
   * If the env var is found and the value is parsed correctly then return true
   * If the env var is not found then retrun true, and value_p is not modified
   * If the env var is found but value could not be parsed correctly then
   *   return false and value is not modified
   */
  static bool GetValueAsUL(const std::string &key,
                           unsigned long *value_p) {
    const std::string value = Envp::Get(key);

    // Probe first character - if is '\0' then we know length == 0
    if(value.c_str()[0] == '\0') {
      return true;
    }

    unsigned long result;

    try {
      result = std::stoul(value);
    } catch(...) {
      return false;
    }

    *value_p = result;

    return true;
  }
};

/*
 * class Zipfian - Generates zipfian random numbers
 *
 * This class is adapted from:
 *   https://github.com/efficient/msls-eval/blob/master/zipf.h
 *   https://github.com/efficient/msls-eval/blob/master/util.h
 *
 * The license is Apache 2.0.
 *
 * Usage:
 *   theta = 0 gives a uniform distribution.
 *   0 < theta < 0.992 gives some Zipf dist (higher theta = more skew).
 *
 * YCSB's default is 0.99.
 * It does not support theta > 0.992 because fast approximation used in
 * the code cannot handle that range.

 * As extensions,
 *   theta = -1 gives a monotonely increasing sequence with wraparounds at n.
 *   theta >= 40 returns a single key (key 0) only.
 */
class Zipfian {
 private:
  // number of items (input)
  uint64_t n;
  // skewness (input) in (0, 1); or, 0 = uniform, 1 = always zero
  double theta;
  // only depends on theta
  double alpha;
  // only depends on theta
  double thres;
  // last n used to calculate the following
  uint64_t last_n;

  double dbl_n;
  double zetan;
  double eta;
  uint64_t rand_state;

  /*
   * PowApprox() - Approximate power function
   *
   * This function is adapted from the above link, which was again adapted from
   *   http://martin.ankerl.com/2012/01/25/optimized-approximative-pow-in-c-and-cpp/
   */
  static double PowApprox(double a, double b) {
    // calculate approximation with fraction of the exponent
    int e = (int)b;
    union {
      double d;
      int x[2];
    } u = {a};
    u.x[1] = (int)((b - (double)e) * (double)(u.x[1] - 1072632447) + 1072632447.);
    u.x[0] = 0;

    // exponentiation by squaring with the exponent's integer part
    // double r = u.d makes everything much slower, not sure why
    // TODO: use popcount?
    double r = 1.;
    while (e) {
      if (e & 1) r *= a;
      a *= a;
      e >>= 1;
    }

    return r * u.d;
  }

  /*
   * Zeta() - Computes zeta function
   */
  static double Zeta(uint64_t last_n, double last_sum, uint64_t n, double theta) {
    if (last_n > n) {
      last_n = 0;
      last_sum = 0.;
    }

    while (last_n < n) {
      last_sum += 1. / PowApprox((double)last_n + 1., theta);
      last_n++;
    }

    return last_sum;
  }

  /*
   * FastRandD() - Fast randum number generator that returns double
   *
   * This is adapted from:
   *   https://github.com/efficient/msls-eval/blob/master/util.h
   */
  static double FastRandD(uint64_t *state) {
    *state = (*state * 0x5deece66dUL + 0xbUL) & ((1UL << 48) - 1);
    return (double)*state / (double)((1UL << 48) - 1);
  }

 public:

  /*
   * Constructor
   *
   * Note that since we copy this from C code, either memset() or the variable
   * n having the same name as a member is a problem brought about by the
   * transformation
   */
  Zipfian(uint64_t n, double theta, uint64_t rand_seed) {
    assert(n > 0);
    if (theta > 0.992 && theta < 1) {
      fprintf(stderr, "theta > 0.992 will be inaccurate due to approximation\n");
    } else if (theta >= 1. && theta < 40.) {
      fprintf(stderr, "theta in [1., 40.) is not supported\n");
      assert(false);
    }

    assert(theta == -1. || (theta >= 0. && theta < 1.) || theta >= 40.);
    assert(rand_seed < (1UL << 48));

    // This is ugly, but it is copied from C code, so let's preserve this
    memset(this, 0, sizeof(*this));

    this->n = n;
    this->theta = theta;

    if (theta == -1.) {
      rand_seed = rand_seed % n;
    } else if (theta > 0. && theta < 1.) {
      this->alpha = 1. / (1. - theta);
      this->thres = 1. + PowApprox(0.5, theta);
    } else {
      this->alpha = 0.;  // unused
      this->thres = 0.;  // unused
    }

    this->last_n = 0;
    this->zetan = 0.;
    this->rand_state = rand_seed;

    return;
  }

  /*
   * ChangeN() - Changes the parameter n after initialization
   *
   * This is adapted from zipf_change_n()
   */
  void ChangeN(uint64_t n) {
    this->n = n;

    return;
  }

  /*
   * Get() - Return the next number in the Zipfian distribution
   */
  uint64_t Get() {
    if (this->last_n != this->n) {
      if (this->theta > 0. && this->theta < 1.) {
        this->zetan = Zeta(this->last_n, this->zetan, this->n, this->theta);
        this->eta = (1. - PowApprox(2. / (double)this->n, 1. - this->theta)) /
                     (1. - Zeta(0, 0., 2, this->theta) / this->zetan);
      }
      this->last_n = this->n;
      this->dbl_n = (double)this->n;
    }

    if (this->theta == -1.) {
      uint64_t v = this->rand_state;
      if (++this->rand_state >= this->n) this->rand_state = 0;
      return v;
    } else if (this->theta == 0.) {
      double u = FastRandD(&this->rand_state);
      return (uint64_t)(this->dbl_n * u);
    } else if (this->theta >= 40.) {
      return 0UL;
    } else {
      // from J. Gray et al. Quickly generating billion-record synthetic
      // databases. In SIGMOD, 1994.

      // double u = erand48(this->rand_state);
      double u = FastRandD(&this->rand_state);
      double uz = u * this->zetan;

      if(uz < 1.) {
        return 0UL;
      } else if(uz < this->thres) {
        return 1UL;
      } else {
        return (uint64_t)(this->dbl_n *
                          PowApprox(this->eta * (u - 1.) + 1., this->alpha));
      }
    }

    // Should not reach here
    assert(false);
    return 0UL;
  }

};

#ifdef NO_USE_PAPI

/*
 * class CacheMeter - Placeholder for systems without PAPI
 */
class CacheMeter {
 public:
  CacheMeter() {};
  CacheMeter(bool, std::string) {};
  ~CacheMeter() {}
  void Start() {};
  void Stop() {};
  const std::string& GetCounterName(int i) const { return "" };
  size_t GetCounter(int i) const { return 0; };
};

#else

// This requires adding PAPI library during compilation
// The linking flag of PAPI is:
//   -lpapi
// To install PAPI under ubuntu please use the following command:
//   sudo apt-get install libpapi-dev
#include <papi.h>

/*
 * class CacheMeter - Measures cache usage using PAPI library
 *
 * This class is a high level encapsulation of the PAPI library designed for
 * more comprehensive profiling purposes, only using a small feaction of its
 * functionalities available. Also, the applicability of this library is highly
 * platform dependent, so please check whether the platform is supported before
 * using
 */
class CacheMeter {
 private:

  // This is a list of events that we care about
  std::map<string,int> eventMap = {
    { "REF_CYC", PAPI_REF_CYC },    // Cycles
    { "TOT_INS", PAPI_TOT_INS },    // Total instructions
    { "L1_TCM", PAPI_L1_TCM },      // L1 total cache misses
    { "L3_TCM", PAPI_L3_TCM },      // L3 total cache misses
    { "L3_LDM", PAPI_L3_LDM },      // Level 3 load misses
    { "L1_STM", PAPI_L1_STM },      // Level 1 store misses
    { "L2_STM", PAPI_L2_STM },      // Level 1 store misses
    { "PRF_DM", PAPI_PRF_DM },      // Data prefetch cache misses
    { "RES_STL", PAPI_RES_STL },    // Cycles stalled on any resource
    { "MEM_WCY", PAPI_MEM_WCY },    // Cycles Stalled Waiting for memory writes
    { "STL_ICY", PAPI_STL_ICY },    // Cycles with no instruction issue
    { "STL_CCY", PAPI_STL_CCY },    // Cycles with no instructions completed
    { "BR_CN", PAPI_BR_CN },        // Conditional branch instructions
    { "BR_MSP", PAPI_BR_MSP },      // Conditional branch instructions mispredicted
    { "BR_PRC", PAPI_BR_PRC },      // Conditional branch instructions correctly predicted
    { "BR_INS", PAPI_BR_INS },      // Branch instructions
    { "L3_DCA", PAPI_L3_DCA },      // Level 3 data cache accesses
    { "L3_TCA", PAPI_L3_TCA },      // Level 3 total cache accesses
    { "L3_TCR", PAPI_L3_TCR },      // Level 3 total cache reads
    { "L3_TCW", PAPI_L3_TCW },      // Level 3 total cache writes
  };

  // Use the length of the event_list to compute number of events we
  // are counting
  static constexpr int EVENT_COUNT = 4;

  // A list of results collected from the hardware performance counter
  long long counter_list[EVENT_COUNT];
  int event_list[EVENT_COUNT];
  int event_list_size;

  std::vector<std::string> events;

  /*
   * CheckEvent() - Checks whether the event exists in this platform
   *
   * This function wraps PAPI function in C++. Note that PAPI events are
   * declared using anonymous enum which is directly translated into int type
   */
  inline bool CheckEvent(int event) {
    int ret = PAPI_query_event(event);
    return ret == PAPI_OK;
  }

  /*
   * CheckAllEvents() - Checks all events that this object is going to use
   *
   * If the checking fails we just exit with error message indicating which one
   * failed
   */
  void CheckAllEvents(void) {
    assert(events.size() <= EVENT_COUNT);

    // If any of the rquired events do not exist we just exit
    event_list_size = 0;
    for (auto event : events) {
      assert(eventMap.find(event) != eventMap.end());
      if(CheckEvent(eventMap[event]) == false) {
        fprintf(stderr,
                "ERROR: PAPI event %s is not supported\n",
                event.c_str());
        exit(1);
      }
      event_list[event_list_size++] = eventMap[event];
    }
  }

 public:

  /*
   * CacheMeter() - Initialize PAPI and events
   *
   * This function starts counting if the argument passed is true. By default
   * it is false
   */
  CacheMeter(bool start=false, std::string eventList="") {
    int ret = PAPI_library_init(PAPI_VER_CURRENT);

    if (ret != PAPI_VER_CURRENT) {
      fprintf(stderr, "ERROR: PAPI library failed to initialize\n");
      exit(1);
    }

    // Initialize pthread support
    ret = PAPI_thread_init(pthread_self);
    if(ret != PAPI_OK) {
      fprintf(stderr, "ERROR: PAPI library failed to initialize for pthread\n");
      exit(1);
    }

    // Parse desired events
    std::replace(eventList.begin(), eventList.end(), ',', ' ');
    std::stringstream ss(eventList);
    std::istream_iterator<std::string> begin(ss);
    std::istream_iterator<std::string> end;
    events.assign(begin, end);

    // If this does not pass just exit
    CheckAllEvents();

    // If we want to start the counter immediately just test this flag
    if(start == true) {
      Start();
    }

    return;
  }

  /*
   * Destructor
   */
  ~CacheMeter() {
    PAPI_shutdown();

    return;
  }

  /*
   * Start() - Starts the counter until Stop() is called
   *
   * If counter could not be started we just fail
   */
  void Start() {
    int ret = PAPI_start_counters(event_list, event_list_size);
    // Start counters
    if (ret != PAPI_OK) {
      fprintf(stderr,
              "ERROR: Failed to start counters using"
              " PAPI_start_counters() (%d)\n",
              ret);
      exit(1);
    }

    return;
  }

  /*
   * Stop() - Stops all counters, and dump their values inside the local array
   *
   * This function will clear all counters after dumping them into the internal
   * array of this object
   */
  void Stop() {
    // Use counter list to hold counters
    if (PAPI_stop_counters(counter_list, event_list_size) != PAPI_OK) {
      fprintf(stderr,
              "ERROR: Failed to start counters using PAPI_stop_counters()\n");
      exit(1);
    }
  }

  const std::string& GetCounterName(int i) const {
    static const std::string empty = "";
    return (i < event_list_size) ? events[i] : empty;
  }

  size_t GetCounter(int i) const {
    return (i < event_list_size) ? counter_list[i] : 0;
  }
};

#endif

/*
 * class Permutation - Generates permutation of k numbers, ranging from
 *                     0 to k - 1
 *
 * This is usually used to randomize insert() to a data structure such that
 *   (1) Each Insert() call could hit the data structure
 *   (2) There is no extra overhead for failed insertion because all keys are
 *       unique
 */
template <typename IntType>
class Permutation {
 private:
  std::vector<IntType> data;

 public:

  /*
   * Generate() - Generates a permutation and store them inside data
   */
  void Generate(size_t count, IntType start=IntType{0}) {
    // Extend data vector to fill it with elements
    data.resize(count);

    // This function fills the vector with IntType ranging from
    // start to start + count - 1
    std::iota(data.begin(), data.end(), start);

    // Then swap all elements with a random position
    for(size_t i = count-1;i > 0;i--) {
      // The two arguments define a closed interval, NOT open interval
      Random<IntType> rand{0, static_cast<IntType>(i)};
      IntType random_key = rand();

      // Swap two numbers
      std::swap(data[i], data[random_key]);
    }

    return;
  }

  /*
   * Constructor
   */
  Permutation() {}

  /*
   * Constructor - Starts the generation process
   */
  Permutation(size_t count, IntType start=IntType{0}) {
    Generate(count, start);

    return;
  }

  /*
   * operator[] - Accesses random elements
   *
   * Note that return type is reference type, so element could be
   * modified using this method
   */
  inline IntType &operator[](size_t index) {
    return data[index];
  }

  inline const IntType &operator[](size_t index) const {
    return data[index];
  }
};

void PinToCore(size_t core_id);
