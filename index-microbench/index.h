
#define BTREE_SLOWER_LAYOUT

#include "ARTOLC/Tree.h"
#include "indexkey.h"
#include <iostream>
// #ifndef BTREE_SLOWER_LAYOUT
// #include "BTreeOLC/BTreeOLC.h"
// #else
// #include "BTreeOLC/BTreeOLC_child_layout.h"
// #endif
#include "BwTree/bwtree.h"
#include <byteswap.h>

// #include "btree-rtm/btree.h"
#include "masstree/mtIndexAPI.hh"

#include "./nohotspot-skiplist/background.h"
#include "./nohotspot-skiplist/intset.h"
#include "./nohotspot-skiplist/nohotspot_ops.h"

#include "hot/rowex/HOTRowex.hpp"
#include "hot/singlethreaded/HOTSingleThreaded.hpp"
#include "idx/contenthelpers/IdentityKeyExtractor.hpp"
#include "idx/contenthelpers/OptionalValue.hpp"


#include "btree_multimap.h"

#include "BTreeOLC.hpp"
#include "blindi_BTreeOLC_seqtree.hpp"
#include "blindi_BTreeOLC_subtrie.hpp"
#include "blindi_BTreeOLC_trie.hpp"

#include "blindi_btree_hybrid_nodes_multimap.h"
#include "blindi_btree_multimap_seqtree.h"
#include "blindi_btree_multimap_subtrie.h"
#include "blindi_btree_multimap_trie.h"

#ifndef _INDEX_H
#define _INDEX_H

using namespace wangziqi2013;
using namespace bwtree;

// struct UUID {

//   uint64_t val[2];

// public:
//   bool operator<(const UUID& rhs) const {
//     if (val[1] == rhs.val[1])
//       return val[0] < rhs.val[0];
//     else
//       return val[1] < rhs.val[1];
//   }

//   bool operator==(const UUID& other) const {
//     return val[0] == other.val[0] && val[1] == other.val[1];
//   }

//   bool operator>(const UUID& rhs) const {
//     return !(operator==(rhs) && operator<(rhs));
//   }

//     friend std::ostream& operator<<(std::ostream &os, const UUID &s);
//     friend std::istream& operator>>(std::istream &is, UUID &s);
// };

// std::istream& operator>>(std::istream &is, UUID &s) {
//     std::string str;
//     is >> str;
//     uint64_t v = 0;
//     for (int i = 0; i < 16; ++i)
//         v = 10*v+(str[str.size()-1-i]-'0');
//     s.val[1] = v;
//     v = 0;
//     for (int i = 16; i < 32 && str.size() >= i+1; ++i)
//         v = 10*v+(str[str.size()-1-i]-'0');
//     s.val[1] = v;
//     return is;
// }

// std::ostream& operator<<(std::ostream &os, const UUID &s) {
//     return os << s.val[0] << s.val[1];
// }

// std::istream& operator>>(std::istream &is, String &s) {
//     return is >> s.val[0] >> s.val[1];
// }

// namespace std {
// template<>
// struct hash<UUID> {
// public:
//     size_t operator()(const UUID &s) const
//     {
//         size_t h1 = std::hash<uint64_t>()(s.val[0]);
//         size_t h2 = std::hash<uint64_t>()(s.val[1]);
//         return h1 ^ h2;
//     }
// };
// }

template <typename KeyType, typename ValueType, class KeyComparator>
class Index {
  public:
    virtual bool insert(const KeyType &key, const ValueType &value,
                        threadinfo *ti) = 0;

    virtual ValueType find(const KeyType &key, std::vector<ValueType> *v,
                           threadinfo *ti) = 0;

    virtual uint64_t find_bwtree_fast(const KeyType &key,
                                      std::vector<ValueType> *v){};

    // Used for bwtree only
    virtual bool insert_bwtree_fast(const KeyType &key,
                                    const ValueType &value){};

    virtual bool upsert(const KeyType &key, const ValueType &value,
                        threadinfo *ti) = 0;

    virtual uint64_t scan(const KeyType &key, int range, threadinfo *ti) = 0;

    virtual int64_t getMemory() const = 0;

    // This initializes the thread pool
    virtual void UpdateThreadLocal(size_t thread_num) = 0;
    virtual void AssignGCID(size_t thread_id) = 0;
    virtual void UnregisterThread(size_t thread_id) = 0;

    // After insert phase perform this action
    // By default it is empty
    // This will be called in the main thread
    virtual void AfterLoadCallback() {}

    // This is called after threads finish but before the thread local are
    // destroied by the thread manager
    virtual void CollectStatisticalCounter(int) {}
    virtual size_t GetIndexSize() { return 0UL; }

    //
    virtual void print_stats() = 0;

	//
    virtual void print_tree() = 0;

    // Destructor must also be virtual
    virtual ~Index() {}
};

/////////////////////////////////////////////////////////////////////
// Skiplist
/////////////////////////////////////////////////////////////////////

extern thread_local long skiplist_steps;
extern std::atomic<long> skiplist_total_steps;

// template <typename KeyType, class KeyComparator>
// class BTreeRTMIndex : public Index<KeyType, uint64_t, KeyComparator> {
//   public:
//     ~BTreeRTMIndex() { bt_free(tree); }

//     void UpdateThreadLocal(size_t thread_num) {}
//     void AssignGCID(size_t thread_id) {}
//     void UnregisterThread(size_t thread_id) {}

//     bool insert(const KeyType &key, const uint64_t &value, threadinfo *ti) {
//         bt_insert(tree, (uint64_t)key, value);
//         return true;
//     }

//     uint64_t find(const KeyType &key, std::vector<uint64_t> *v,
//                   threadinfo *ti) {
//         uint64_t result;
//         int success;
//         result = bt_find(tree, key, &success);
//         v->clear();
//         v->push_back(result);
//         return 0;
//     }

//     bool upsert(const KeyType &key, const uint64_t &value, threadinfo *ti) {
//         bt_upsert(tree, (uint64_t)key, value);
//         return true;
//     }

//     void incKey(uint64_t &key) { key++; };
//     // void incKey(UUID& key) {
//     //   key.val[1]++;
//     //   if (key.val[1] == 0)
//     //     key.val[0]++;
//     // }
//     void incKey(GenericKey<31> &key) { key.data[strlen(key.data) - 1]++; };
//     void incKey(GenericKey<16> &key) { key.data[strlen(key.data) - 1]++; };

//     uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
//         fprintf(stderr, "\n\n>>> ERROR: in index.h <<<\n\n");
//         return 0;
//     }

//     int64_t getMemory() const { return 0; }

//     void merge() {}

//     BTreeRTMIndex(uint64_t kt) { tree = bt_init(bt_intcmp); }

//     void print_stats() {
//         std::cout   << "BTreeRTM stats:\n";
//         std::cout   << "* N/A" << std::endl;
//     }

// 	void print_tree()  { printf("\nTREE PRINTING REQUESTED: N/A\n"); }

//   private:
//     btree_t *tree;
// };

template <typename KeyType, class KeyComparator>
class SkipListIndex : public Index<KeyType, uint64_t, KeyComparator> {
  public:
    set_t *set;

  public:
    /*
     * Constructor - Allocate memory and initialize the skip list index
     */
    SkipListIndex(uint64_t key_type) {
        (void)key_type;
        skiplist_total_steps.store(0L);

        ptst_subsystem_init();
        gc_subsystem_init();
        set_subsystem_init();
        set = set_new(1);

        return;
    }

    /*
     * Destructor - We need to stop the background thread and also to
     *              free the index object
     */
    ~SkipListIndex() {
        // Stop the background thread
        bg_stop();
        gc_subsystem_destroy();
        // Delete index
        set_delete(set);
        return;
    }

    bool insert(const KeyType &key, const uint64_t &value, threadinfo *ti) {
        sl_insert(&skiplist_steps, set, key,
                  const_cast<void *>(static_cast<const void *>(&value)));
        (void)ti;
        return true;
    }

    uint64_t find(const KeyType &key, std::vector<uint64_t> *v,
                  threadinfo *ti) {
        // Note that skiplist only supports membership check
        // This is fine, because it still traverses to the location that
        // the key is stored. We just call push_back() with an arbitraty
        // number to compensate for lacking a value
        sl_contains(&skiplist_steps, set, key);
        (void)v;
        (void)ti;
        v->clear();
        v->push_back(0);
        return 0UL;
    }

    bool upsert(const KeyType &key, const uint64_t &value, threadinfo *ti) {
        // Upsert is implemented as two operations. In practice if we change
        // the internals of the skiplist, we can make it one atomic step
        sl_delete(&skiplist_steps, set, key);
        sl_insert(&skiplist_steps, set, key,
                  const_cast<void *>(static_cast<const void *>(&value)));
        (void)ti;
        return true;
    }

    uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
        sl_scan(&skiplist_steps, set, key, range);
        (void)ti;
        #ifdef DEBUG_PRINT
        std::cout << "SkipList does not return the results of the scan" << std::endl;
        #endif
        return 0UL;
    }

    int64_t getMemory() const { return 0L; }

    // Returns the size of the skiplist
    size_t GetIndexSize() { return (size_t)set_size(set, 1); }

    // Not actually used
    void UpdateThreadLocal(size_t thread_num) { (void)thread_num; }

    // Before thread starts we set the steps to 0
    void AssignGCID(size_t thread_id) {
        (void)thread_id;
        skiplist_steps = 0L;
    }

    // Before thread exits we aggregate the steps into the global counter
    void UnregisterThread(size_t thread_id) {
        (void)thread_id;
        skiplist_total_steps.fetch_add(skiplist_steps);
        return;
    }

    void print_stats() {
        std::cout   << "Skiplist stats:\n";
        std::cout   << "* N/A" << std::endl;        
    }

	void print_tree()  { printf("\nTREE PRINTING REQUESTED: N/A\n"); }
};

/////////////////////////////////////////////////////////////////////
// ARTOLC
/////////////////////////////////////////////////////////////////////

template <typename KeyType, class KeyComparator>
class ArtOLCIndex : public Index<KeyType, uint64_t, KeyComparator> {
  public:
    ~ArtOLCIndex() { delete idx; }

    void UpdateThreadLocal(size_t thread_num) {}
    void AssignGCID(size_t thread_id) {}
    void UnregisterThread(size_t thread_id) {}

    void setKey(Key &k, uint64_t key) {
        k.setInt(key);
    }
    void setKey(Key &k, GenericKey<31> key) { k.set(key.data, 31); }
    void setKey(Key &k, GenericKey<16> key) { k.set(key.data, 16); }

    bool insert(const KeyType &key, const uint64_t &value, threadinfo *ti) {
        // std::cerr << "ARTOLC starting an insert" << std::endl;
        KeyType _key = key;
        uint64_t _val = value;
        auto t = idx->getThreadInfo();
        Key k;
        setKey(k, _key);
        idx->insert(k, _val, t);
        std::cerr << "ARTOLC finishing an insert" << std::endl;
        return true;
    }

    uint64_t find(const KeyType &key, std::vector<uint64_t> *v,
                  threadinfo *ti) {
        std::cerr << "ARTOLC starting a find" << std::endl;
        auto t = idx->getThreadInfo();
        KeyType _key = key;
        Key k;
        setKey(k, _key);
        uint64_t result = idx->lookup(k, t);
        v->clear();
        v->push_back(result);
        std::cerr << "ARTOLC finishing a find" << std::endl;
        return 0;
    }

    bool upsert(const KeyType &key, const uint64_t &value, threadinfo *ti) {
        auto t = idx->getThreadInfo();
        Key k;
        setKey(k, key);
        idx->insert(k, value, t);
    }

    uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
        auto t = idx->getThreadInfo();
        Key startKey;
        setKey(startKey, key);

        TID results[range];
        size_t resultCount;
        Key continueKey;
        idx->lookupRange(startKey, maxKey, continueKey, results, range,
                         resultCount, t);

        return resultCount;
    }

    int64_t getMemory() const { return 0; }

    void merge() {}

  ArtOLCIndex(uint64_t kt) {
    if (sizeof(KeyType)==8) {
      idx = new ART_OLC::Tree([](TID tid, Key &key) { key.setInt(*reinterpret_cast<uint64_t*>(tid)); });
      maxKey.setInt(~0ull);
    } else {
      idx = new ART_OLC::Tree([](TID tid, Key &key) { key.set(reinterpret_cast<char*>(tid),31); });
      uint8_t m[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		     0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		     0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		     0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
      maxKey.set((char*)m,31);
    }
  }

    void print_stats() {
        std::cout   << "ArtOLC stats:\n";
        std::cout   << "* N/A" << std::endl;    
    }


	void print_tree()  { printf("\nTREE PRINTING REQUESTED: N/A\n"); }

  private:
    Key maxKey;
    ART_OLC::Tree *idx;
};

template <typename KeyType, class KeyComparator,
          int InnerSlots, int LeafSlots>
class BTreeOLCIndex : public Index<KeyType, uint64_t, KeyComparator> {
  public:
    ~BTreeOLCIndex() {}

    void UpdateThreadLocal(size_t thread_num) {}
    void AssignGCID(size_t thread_id) {}
    void UnregisterThread(size_t thread_id) {}

    bool insert(const KeyType &key, const uint64_t &value, threadinfo *ti) {
        // std::cerr << "Size of the key is " << sizeof(key) << std:: endl;
        // std::cerr << "Size of the key type is " << sizeof(KeyType) << std:: endl;
        // std::cerr << "INSERT " << key << " " << value << std::endl;
        // uint64_t *ptr_key = reinterpret_cast<uint64_t*>(value);
        // uint64_t *ptr_val = reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(value)+sizeof(key));
        // std::cerr << "*value is " <<  (*ptr_key) << std::endl;
        // std::cerr << "value is  " <<  (*ptr_val) << std::endl;
        // getchar();
        idx.insert(key, value);
        return true;
    }

    uint64_t find(const KeyType &key, std::vector<uint64_t> *v,
                  threadinfo *ti) {
        uint64_t result;
        bool found = idx.lookup(key, result);
        v->clear();
        if (!found) {
            std::cerr << "Warning: in btreeolc::find(" << key << ")" << std::endl;
            // std::cout << "  * Nothing to update! the key is not in the index yet" << std::endl;
            getchar();
        }
        v->push_back(result);
        return result;
    }

    bool upsert(const KeyType &key, const uint64_t &value, threadinfo *ti) {
        bool f = idx.upsert(key, value);
        if (!f) {
            std::cerr << "Warning: in btreeolc::upsert(" << key << ", " << value << ")" << std::endl;
            getchar();
        }
        return f;
        // uint64_t result;
        // bool f = idx.lookup(key, result);
        // if (!f) {
        //     std::cerr << "Warning: in btreeolc::upsert(" << key << ", " << value << ")" << std::endl;
        //     // std::cout << "  * Nothing to update! the key is not in the index yet" << std::endl;
        //     getchar();
        // }
        // uint64_t *ptr_key = reinterpret_cast<uint64_t*>(result);
        // uint64_t *ptr_val = reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(result)+sizeof(key));
        // // std::cerr << "Size of the key is " << sizeof(key) << std:: endl;
        // // std::cerr << "Old value is " << (*ptr_val) << std:: endl;
        // *ptr_val = value;
        // // std::cerr << "New value is " << (*ptr_val) << std:: endl;
        // // getchar();
        // // idx.insert(key, value);
        // return true;
    }

    void incKey(uint64_t &key) { key++; };
    // void incKey(UUID& key) {
    //   key.val[1]++;
    //   if (key.val[1] == 0)
    //     key.val[0]++;
    // }
    void incKey(GenericKey<31> &key) { key.data[strlen(key.data) - 1]++; };
    void incKey(GenericKey<16> &key) { key.data[strlen(key.data) - 1]++; };

    uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
        // uint64_t results[range];
        // uint64_t count = idx.scan(key, range, results);
        // if (count==0)
        //    return 0;

        // while (count < range) {
        //   // KeyType nextKey = *reinterpret_cast<KeyType*>(results[count-1]);
        //   // KeyType nextKey = keys[results[count-1]];
        //   incKey(nextKey); // hack: this only works for fixed-size keys

        //   uint64_t nextCount = idx.scan(nextKey, range - count, results +
        //   count); if (nextCount==0)
        //     break; // no more entries
        //   count += nextCount;
        // }
        // return count;
    }

    int64_t getMemory() const { return 0; }

    void merge() {}

    BTreeOLCIndex(uint64_t kt) {}

    void print_stats() {
        #ifdef BTREEOLC_DEBUG
        auto stats = idx.get_stats();
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

	void print_tree()  { printf("\nTREE PRINTING REQUESTED: N/A\n"); }

  private:
    btreeolc::BTree<KeyType, uint64_t, InnerSlots, LeafSlots, KeyComparator> idx;
};

template <typename KeyType, typename ValueType, class KeyComparator,
          int InnerSlots, int LeafSlots>
class BTreeOLCSeqtreeIndex : public Index<KeyType, ValueType, KeyComparator> {
  public:
    ~BTreeOLCSeqtreeIndex() {}

    void UpdateThreadLocal(size_t thread_num) {}
    void AssignGCID(size_t thread_id) {}
    void UnregisterThread(size_t thread_id) {}

    bool insert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        ValueType result;
        idx.insert(key, value);
        return true;
    }

    ValueType find(const KeyType &key, std::vector<ValueType> *v,
                   threadinfo *ti) {
        ValueType result;
        bool flag = idx.lookup(key, result);
        v->clear();
        if (!flag) {
            std::cerr << "Warning: in btreeolc_seqtree::find(" << key << ")" << std::endl;
            // std::cout << "  * The key has not been found contrary to how YCSB workloads are" << std::endl;
            getchar();
        }

        // std::cerr << sizeof(key) << " vs " << sizeof(KeyType) << std::endl;
        // KeyType *ptr_key = reinterpret_cast<KeyType*>(result);
        // ValueType *ptr_val = reinterpret_cast<ValueType*>(reinterpret_cast<uint8_t*>(result)+sizeof(key));
        v->push_back(result);//*ptr_val);
        return result;
    }

    bool upsert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        bool f = idx.upsert(key, value);
        if (!f) {
            std::cerr << "Warning: in btreeolc::upsert(" << key << ", " << value << ")" << std::endl;
            getchar();
        }
        return f;
        // // std::cerr << "!!!" << std::endl;
        // ValueType result;
        // bool flag = idx.lookup(key, result);
        // if (!flag) {
        //     std::cerr << "Warning: in btreeolc_seqtree::upsert(" << key << ", " << value << ")" << std::endl;
        //     // std::cout << "  * Nothing to update! the key is not in the index yet" << std::endl;
        //     getchar();
        // }
        // // uint64_t *ptr_key = reinterpret_cast<uint64_t*>(result);
        // uint64_t *ptr_val = reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(result)+sizeof(key));
        // // std::cerr << "Size of the key is " << sizeof(key) << std:: endl;
        // // std::cerr << "Old value is " << (*ptr_val) << std:: endl;
        // // getchar();
        // *ptr_val = value;
        // // idx.upsert(key, value);
        // return true;
    }

    void incKey(uint64_t &key) { key++; };
    // void incKey(UUID& key) {
    //   key.val[1]++;
    //   if (key.val[1] == 0)
    //     key.val[0]++;
    // }
    void incKey(GenericKey<31> &key) { key.data[strlen(key.data) - 1]++; };
    void incKey(GenericKey<16> &key) { key.data[strlen(key.data) - 1]++; };

    uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
        // ValueType results[range];
        // uint64_t count = idx.scan(key, range, results);
        // if (count == 0)
        //     return 0;

        // while (count < range) {
        //     KeyType nextKey = *reinterpret_cast<KeyType *>(results[count - 1]);
        //     incKey(nextKey); // hack: this only works for fixed-size keys

        //     uint64_t nextCount =
        //         idx.scan(nextKey, range - count, results + count);
        //     if (nextCount == 0)
        //         break; // no more entries
        //     count += nextCount;
        // }
        // test_scan(key, range, value_list);
        // return count;
    }

    int64_t getMemory() const { return 0; }

    void merge() {}

    BTreeOLCSeqtreeIndex(uint64_t kt) {}

    void print_stats() {
        #ifdef BTREEOLC_DEBUG
        auto stats = idx.get_stats();
        uint64_t leaf_size = stats.leaf_size;
        uint64_t leaf_num = stats.leaf_num;
        uint64_t leaf_slots = stats.leaf_slots;
        uint64_t inner_size = stats.inner_size;
        uint64_t inner_num = stats.inner_num;
        uint64_t inner_slots = stats.inner_slots;
        uint64_t th_size = (leaf_size*leaf_num+inner_size*inner_num);
        std::cout   << "BTreeOLC stats:\n";
        std::cout   << "* leaf size:    " << leaf_size << std::endl 	
                    << "* leaf num:     " << leaf_num << std::endl
                    << "* leaf slots:   " << leaf_slots << std::endl
                    << "* inner size:   " << inner_size << std::endl
                    << "* inner num:    " << inner_num << std::endl
                    << "* inner slots:  " << inner_slots << std::endl
                    << "* theoretical index size: " << th_size << std::endl;
        #else
        printf("\nINDEX STATS REQUESTED: N/A\n");
        #endif
    }

	void print_tree()  {
        printf("\nTREE PRINTING BEGINS\n");
        ofstream fout("btreeolc_seqtree.out");
        idx.print_tree(fout);
        printf("\nTREE PRINTING ENDS\n");
    }

  private:
    blindi::BTree<KeyType, ValueType, InnerSlots, LeafSlots, KeyComparator> idx;
};

template <typename KeyType, typename ValueType, class KeyComparator,
          int InnerSlots, int LeafSlots>
class BTreeOLCTrieIndex
    : public Index<KeyType, ValueType, KeyComparator> {
  public:
    ~BTreeOLCTrieIndex() {}

    void UpdateThreadLocal(size_t thread_num) {}
    void AssignGCID(size_t thread_id) {}
    void UnregisterThread(size_t thread_id) {}

    bool insert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        idx.insert(key, value);
        #ifdef BTREEOLC_DEBUG
        static auto xxx = 0;
        if (!((++xxx) % 10000000))
            print_stats();
        // print_tree();
        #endif          
        return true;
    }

    ValueType find(const KeyType &key, std::vector<ValueType> *v,
                   threadinfo *ti) {
        ValueType result;
        bool red = idx.lookup(key, result);
        v->clear();
        v->push_back(result);
        return result;
    }

    bool upsert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        idx.upsert(key, value);
        return true;
    }

    void incKey(uint64_t &key) { key++; };
    // void incKey(UUID& key) {
    //   key.val[1]++;
    //   if (key.val[1] == 0)
    //     key.val[0]++;
    // }
    void incKey(GenericKey<31> &key) { key.data[strlen(key.data) - 1]++; };
    void incKey(GenericKey<16> &key) { key.data[strlen(key.data) - 1]++; };

    uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
        // ValueType results[range];
        // uint64_t count = idx.scan(key, range, results);
        // if (count == 0)
        //     return 0;

        // while (count < range) {
        //     KeyType nextKey = *reinterpret_cast<KeyType *>(results[count - 1]);
        //     incKey(nextKey); // hack: this only works for fixed-size keys

        //     uint64_t nextCount =
        //         idx.scan(nextKey, range - count, results + count);
        //     if (nextCount == 0)
        //         break; // no more entries
        //     count += nextCount;
        // }
        // test_scan(key, range, value_list);
        // return count;
    }

    int64_t getMemory() const { return 0; }

    void merge() {}

    BTreeOLCTrieIndex(uint64_t kt) {}


    void print_stats() {
        std::cout   << "BTreeOLC Trie stats:\n";
        std::cout   << "* N/A" << std::endl;
    }
	
	void print_tree()  { printf("\nTREE PRINTING REQUESTED: N/A\n"); }

  private:
    blindi_trie::BTree<KeyType, ValueType, InnerSlots, LeafSlots, KeyComparator>
        idx;
};

template <typename KeyType, typename ValueType, class KeyComparator,
          int InnerSlots, int LeafSlots>
class BTreeOLCSubtrieIndex
    : public Index<KeyType, ValueType, KeyComparator> {
  public:
    ~BTreeOLCSubtrieIndex() {}

    void UpdateThreadLocal(size_t thread_num) {}
    void AssignGCID(size_t thread_id) {}
    void UnregisterThread(size_t thread_id) {}

    bool insert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        idx.insert(key, value);
        #ifdef BTREEOLC_DEBUG
        static auto xxx = 0;
        if (!((++xxx) % 10000000))
            print_stats();
        // print_tree();
        #endif          
        return true;
    }

    ValueType find(const KeyType &key, std::vector<ValueType> *v,
                   threadinfo *ti) {
        ValueType result;
        bool red = idx.lookup(key, result);
        v->clear();
        v->push_back(result);
        return result;
    }

    bool upsert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        idx.upsert(key, value);
        return true;
    }

    void incKey(uint64_t &key) { key++; }
    // void incKey(UUID& key) {
    //   key.val[1]++;
    //   if (key.val[1] == 0)
    //     key.val[0]++;
    // }
    void incKey(GenericKey<31> &key) { key.data[strlen(key.data) - 1]++; };
    void incKey(GenericKey<16> &key) { key.data[strlen(key.data) - 1]++; };

    uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
        // ValueType results[range];
        // uint64_t count = idx.scan(key, range, results);
        // if (count == 0)
        //     return 0;

        // while (count < range) {
        //     KeyType nextKey = *reinterpret_cast<KeyType *>(results[count - 1]);
        //     incKey(nextKey); // hack: this only works for fixed-size keys

        //     uint64_t nextCount =
        //         idx.scan(nextKey, range - count, results + count);
        //     if (nextCount == 0)
        //         break; // no more entries
        //     count += nextCount;
        // }
        // test_scan(key, range, value_list);
        // return count;
    }

    int64_t getMemory() const { return 0; }

    void merge() {}

    BTreeOLCSubtrieIndex(uint64_t kt) {}

    void print_stats() {
        std::cout   << "BTreeOLC Subtrie stats:\n";
        std::cout   << "* N/A" << std::endl;
    }
	
	
	void print_tree()  { printf("\nTREE PRINTING REQUESTED: N/A\n"); }

  private:
    blindi_subtrie::BTree<KeyType, ValueType, InnerSlots, LeafSlots,
                          KeyComparator> idx;
};


template <typename KeyType, typename ValueType, class KeyComparator,
          int InnerSlots, int LeafSlots>
class HotIndex : public Index<KeyType, ValueType, KeyComparator> {
  public:
    template <typename Val> struct KeyExtractor {

        inline KeyType operator()(Val const &ptr) const {
            return *((KeyType *)ptr);
        }
    };

    ~HotIndex() {}

    void UpdateThreadLocal(size_t thread_num) {}
    void AssignGCID(size_t thread_id) {}
    void UnregisterThread(size_t thread_id) {}

    bool insert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        // printf("Inserting (%d, %d) the address %p\n", key, value, (&key));
        return hot.insert((void *)&key);
    }

    ValueType find(const KeyType &key, std::vector<ValueType> *v,
                   threadinfo *ti) {
        auto res = hot.lookup(key);
        v->clear();
        if (res.mIsValid) {
            KeyType *k = (KeyType *)res.mValue;
            // ValueType *val = (ValueType *)(k + 1);
            // v->push_back(*val);
            v->push_back(reinterpret_cast<uint64_t>(k));
        }
    }

    bool upsert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        auto res = hot.upsert((void *)&key);
        return res.mIsValid; 
    }

    void incKey(uint64_t &key) { key++; };
    void incKey(GenericKey<31> &key) { key.data[strlen(key.data) - 1]++; };
    void incKey(GenericKey<16> &key) { key.data[strlen(key.data) - 1]++; };

    uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
        // std::vector<ValueType> value_list;
        // auto it = hot.upper_bound(key);
        // for (int i = 0; i < 1; i++) {
        //     if (it == hot.end())
        //         break;
        //     value_list.push_back(it->second);
        //     it++;
        // }
        // return value_list.size();
        auto it = hot.lower_bound(key);
        std::vector<pair<KeyType, KeyType*> > key_list;
        // std::cerr << "in scan: ";
		for (int i = 0; i < range; ++i) {
            if (it == hot.end())
                break;
            auto tmp = (KeyType*)*it;
            key_list.push_back(make_pair(*tmp, tmp)); // This is incorrect
            // std::cerr << " " << (*tmp);
            ++it;
        }
        #ifdef DEBUG_PRINT
        std::cout << "results of scan called with: key=" << key << " range=" << range << std::endl;
        std::cout << "  res.size()=" << key_list.size() << std::endl;
        for (auto i = 0; i < key_list.size(); ++i) {
            std::cout << "  res[" << i << "]=(" << key_list[i].first << ", " << key_list[i].second << ")" << std::endl;
        }
        getchar();
        #endif
        return key_list.size();
    }

    int64_t getMemory() const { return 0; }

    void merge() {}

    HotIndex(uint64_t kt) {}

    void print_stats() {
        std::cout   << "HOT (single-threaded) stats:\n";
        std::cout   << "* N/A" << std::endl;
    }

	void print_tree()  { printf("\nTREE PRINTING REQUESTED: N/A\n"); }

  private:
    hot::singlethreaded::HOTSingleThreaded<void *, KeyExtractor> hot;
};

template <typename KeyType, typename ValueType, class KeyComparator,
          int InnerSlots, int LeafSlots>
class ConcurrentHotIndex : public Index<KeyType, ValueType, KeyComparator> {

  public:
    template <typename Val> struct KeyExtractor {

        inline KeyType operator()(Val const &ptr) const {
            return *((KeyType *)ptr);
        }
    };

public:
    ~ConcurrentHotIndex() {}

    void UpdateThreadLocal(size_t thread_num) {}
    void AssignGCID(size_t thread_id) {}
    void UnregisterThread(size_t thread_id) {}

    bool insert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        // printf("Inserting (%d, %d) the address %p\n", key, value, (&key));
        return hot.insert((void *)&key);
    }

    ValueType find(const KeyType &key, std::vector<ValueType> *v,
                   threadinfo *ti) {
        auto res = hot.lookup(key);
        v->clear();
        if (res.mIsValid) {
            KeyType *k = (KeyType *)res.mValue;
            // ValueType *val = (ValueType *)(k); // This is incorrect
            // v->push_back(*val);
            v->push_back(reinterpret_cast<uint64_t>(k));
        }
    }

    bool upsert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        return hot.insert((void *)&key);
    }

    void incKey(uint64_t &key) { key++; };
    void incKey(GenericKey<31> &key) { key.data[strlen(key.data) - 1]++; };
    void incKey(GenericKey<16> &key) { key.data[strlen(key.data) - 1]++; };

    uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
        // std::cout << "scan " << key << " " << range << std::endl;
        auto it = hot.lower_bound(key);
        std::vector<KeyType> key_list;
		for (int i = 0; i < range; ++i) {
            if (it == hot.end())
                break;
            auto tmp = (KeyType*)*it;
            key_list.push_back(*tmp); // This is incorrect
            ++it;
        }
        // test_scan(key, range, key_list);
        return key_list.size();
    }

    int64_t getMemory() const { return 0; }

    void merge() {}

    ConcurrentHotIndex(uint64_t kt) {}

    void print_stats() {
        std::cout   << "HOT (multi-threaded) stats:\n";
        std::cout   << "* N/A" << std::endl;
    }

	void print_tree()  { printf("\nTREE PRINTING REQUESTED: N/A\n"); }

  private:
    // using NoThreadInfo = idx::benchmark::NoThreadInfo;
	// using IndexType = hot::rowex::HOTRowex<void *, KeyExtractor>;//hot::rowex::HOTRowex<uint64_t, idx::contenthelpers::IdentityKeyExtractor>;
    hot::rowex::HOTRowex<void *, KeyExtractor> hot;
};

template <typename KeyType, typename ValueType, typename KeyComparator,
          int InnerSlots, int LeafSlots>
class STXIndex : public Index<KeyType, ValueType, KeyComparator> {
  public:
    ~STXIndex() {}

    void UpdateThreadLocal(size_t thread_num) {}
    void AssignGCID(size_t thread_id) {}
    void UnregisterThread(size_t thread_id) {}

    bool insert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        idx.insert(key, value);
        // #ifdef DEBUG_PRINT
        // static auto xxx = 0;
        // if (!((++xxx) % 10000000))
        //     print_stats();
        // // print_tree();
        // #endif
        // auto it = idx.find(key);
        // auto result = it->second;
        return true;
    }

    ValueType find(const KeyType &key, std::vector<ValueType> *v,
                   threadinfo *ti) {
        auto it = idx.find(key);
        v->clear();
        auto result = it->second;

        if (it == idx.end())  {
            std::cout << "READ: key not found" << std::endl;
            getchar();
        }
        // uint64_t *ptr_val = reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(result)+sizeof(key));
        // v->clear();
        // v->push_back(*ptr_val);
        v->push_back(result);
        return result;
    }

    bool upsert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        // idx.insert(key, value);
        // if (key == 2933389304617401955)
        //     printf("\n\nUpserting (%llu, %llu) the address %p\n", key, value, (&key));        
        auto it = idx.find(key);
        if (it == idx.end()) {
            std::cout << "\n\nError: updating a record that has not been inserted yet" << std::endl;
            return true;
        }
        // uint64_t *ptr_key = reinterpret_cast<uint64_t*>(result);
        // uint64_t *ptr_val = reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(result)+sizeof(key));
        // it.data() = reinterpret_cast<uint64_t*>(&key);
        it.data() = value;
        return false;
    }

    void incKey(uint64_t &key) { key++; };
    // void incKey(UUID& key) {
    //   key.val[1]++;
    //   if (key.val[1] == 0)
    //     key.val[0]++;
    // }
    void incKey(GenericKey<31> &key) { key.data[strlen(key.data) - 1]++; };
    void incKey(GenericKey<16> &key) { key.data[strlen(key.data) - 1]++; };

    uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
        std::vector<pair<KeyType, ValueType> > value_list;
        auto it = idx.find(key);
        for (int i = 0; i < range; i++) {
            if (it == idx.end()) {
                #ifdef DEBUG_PRINT
                std::cout << "WARNING: scan reached the end of the index prematurely" << std::endl;
                #endif
                break;
            }
            value_list.push_back(make_pair(it->first, it->second));
            it++;
        }
        #ifdef DEBUG_PRINT
        std::cout << "results of scan called with: key=" << key << " range=" << range << std::endl;
        std::cout << "  res.size()=" << value_list.size() << std::endl;
        for (auto i = 0; i < value_list.size(); ++i) {
            std::cout << "  res[" << i << "]=(" << value_list[i].first << ", " << value_list[i].second << ")" << std::endl;
            if (value_list[i].first != *((KeyType*)value_list[i].second))
                std::cout << "Error: scan returned an incorrect pair" << std::endl;
        }
        getchar();
        #endif        
    }

    int64_t getMemory() const { return 0; }

    void merge() {}

    STXIndex(uint64_t kt) {}

    void print_stats() {
        auto stats = idx.get_stats();
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


	void print_tree()  {
		std::cout << "\nBEGIN PRINTING A TREE:" << std::endl;
        #ifdef BTREE_DEBUG
		idx.print(cout);
        #endif
		std::cout << "END OF PRINTING" << std::endl;
		getchar();
    }

  private:
    stx::btree_multimap<KeyType, ValueType, InnerSlots, LeafSlots,
                        KeyComparator>
        idx;
};

template <typename KeyType, typename ValueType, typename KeyComparator,
          int InnerSlots, int LeafSlots>
class STXHybridIndex : public Index<KeyType, ValueType, KeyComparator> {
  public:
    ~STXHybridIndex() {}

    void UpdateThreadLocal(size_t thread_num) {}
    void AssignGCID(size_t thread_id) {}
    void UnregisterThread(size_t thread_id) {}

    bool insert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        idx.insert(key, value);
        // #ifdef DEBUG_PRINT
        // static auto xxx = 0;
        // if (!((++xxx) % 10000000))
        //     print_stats();
        // // print_tree();
        // #endif
        // auto it = idx.find(key);
        // auto result = it->second;
        return true;
    }

    ValueType find(const KeyType &key, std::vector<ValueType> *v,
                   threadinfo *ti) {
        // std::cout << "Calling find with "/<< key << std::endl;
        auto it = idx.find(key);
        auto result = it->second;
        if (it == idx.end()) {
            std::cout << "Error in find: key " << key << " not found" << std::endl;
            getchar();
            return result;
        }
        v->clear();
        v->push_back(result);
        return result;
    }

    bool upsert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        auto it = idx.find(key);
        if (it == idx.end()) {
            std::cout << "\n\nError in upsert: updating a record that has not been inserted yet" << std::endl;
            getchar();
            return true;
        }
        it.data() = value;
        return false;
    }

    void incKey(uint64_t &key) { key++; };
    void incKey(GenericKey<31> &key) { key.data[strlen(key.data) - 1]++; };
    void incKey(GenericKey<16> &key) { key.data[strlen(key.data) - 1]++; };

    uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
        std::vector<pair<KeyType, ValueType> > value_list;
        auto it = idx.find(key);
        for (int i = 0; i < range; i++) {
            if (it == idx.end()) {
                #ifdef DEBUG_PRINT
                std::cout << "WARNING: scan reached the end of the index prematurely" << std::endl;
                #endif
                break;
            }
            value_list.push_back(make_pair(it->first, it->second));
            it++;
        }
        #ifdef DEBUG_PRINT
        std::cout << "results of scan called with: key=" << key << " range=" << range << std::endl;
        std::cout << "  res.size()=" << value_list.size() << std::endl;
        for (auto i = 0; i < value_list.size(); ++i) {
            std::cout << "  res[" << i << "]=(" << value_list[i].first << ", " << value_list[i].second << ")" << std::endl;
            if (value_list[i].first != *((KeyType*)value_list[i].second))
                std::cout << "Error: scan returned an incorrect pair" << std::endl;
        }
        getchar();
        #endif
    }

    int64_t getMemory() const { return 0; }

    void merge() {}

    STXHybridIndex(uint64_t kt) {}

    void print_stats() {
        auto stats = idx.get_stats();
        // auto stats = tree.get_stats();
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


	void print_tree()  {
		std::cout << "\nBEGIN PRINTING A TREE:" << std::endl;
        #ifdef BTREE_DEBUG
		idx.print(cout);
        #endif
		std::cout << "END OF PRINTING" << std::endl;
		getchar();
    }

  private:
    stx::blindi_btree_hybrid_nodes_multimap<KeyType, ValueType, SeqTreeBlindiNode, InnerSlots, LeafSlots, KeyComparator> idx;
};

template <typename KeyType, typename ValueType, class KeyComparator,
          int InnerSlots, int LeafSlots>
class STXSeqtreeIndex : public Index<KeyType, ValueType, KeyComparator> {
  public:
    ~STXSeqtreeIndex() {}

    void UpdateThreadLocal(size_t thread_num) {}
    void AssignGCID(size_t thread_id) {}
    void UnregisterThread(size_t thread_id) {}

    bool insert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        idx.insert(key, value);
        // #ifdef DEBUG_PRINT
        // static auto xxx = 0;
        // if (!((++xxx) % 10000000))
        //     print_stats();
        // // print_tree();
        // #endif
        return true;
    }

    ValueType find(const KeyType &key, std::vector<ValueType> *v,
                   threadinfo *ti) {
        auto it = idx.find(key);
        v->clear();

        if (it == idx.end())  {
            std::cout << "READ: key not found" << std::endl;
            getchar();
        }
        auto result = it->second;
        v->clear();
        v->push_back(result);        
        // std::cerr << "Iterator stuff: " << it.key() << " " << it.data() << std::endl;
        // uint64_t *ptr_key = reinterpret_cast<uint64_t*>(result);
        // uint64_t *ptr_val = reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(result)+sizeof(key));
        // std::cerr << "The txn.key is *(" << (&key) <<  "){ = "<< key << std::endl;
        // std::cerr << "FIND returned " << it->first << " " << it->second << std::endl;
        // std::cerr << "    which can be interpreted as: key_ptr being " << ptr_key << ", value_ptr being " << ptr_val << std::endl;
        // std::cerr << "    and the actual key found is " <}< (*ptr_key) << std::endl;
        // getchar();
     
        return result;
    }

    bool upsert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        // static int n = 0;
        // if (n % 100000 == 0)
            // std::cerr << "\nStarting an upsert of (" << key << ", " << value << ")..." << std::endl;
        // idx.upsert(key, value);
        // if (n % 100000 == 0)
            // std::cerr << "... DONE" << std::endl;
        // ++n;
		// print_stats();
        // return true;
        auto it = idx.find(key);
        // std::cerr << "\nUPDATE: key " << key  << " to " << value << std::endl;
        if (it == idx.end()) {
            std::cout << "Error: updating a record that has not been inserted yet" << std::endl;
            return true;
        }
        // std::cerr << "Found " << it->first << " " << it->second << std::endl;        
        // std::cerr << "      " << it.key() << " " << it.data() << std::endl;        
        it.data() = value;
        // std::cerr << "Updated to " << it->second << std::endl;        
        // getchar();
        return false;        
    }

    void incKey(uint64_t &key) { key++; };
    // void incKey(UUID& key) {
    //   key.val[1]++;
    //   if (key.val[1] == 0)
    //     key.val[0]++;
    // }
    void incKey(GenericKey<31> &key) { key.data[strlen(key.data) - 1]++; };
    void incKey(GenericKey<16> &key) { key.data[strlen(key.data) - 1]++; };

    uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
        std::vector<pair<KeyType, ValueType> > value_list;
        auto it = idx.lower_bound(key);
        for (int i = 0; i < range; i++) {
            if (it == idx.end()) {
                #ifdef DEBUG_PRINT
                std::cout << "WARNING: scan reached the end of the index prematurely" << std::endl;
                #endif
                break;
            }
            value_list.push_back(make_pair(it->first, it->second));
            it++;
        }
        #ifdef DEBUG_PRINT
        std::cout << "results of scan called with: key=" << key << " range=" << range << std::endl;
        std::cout << "  res.size()=" << value_list.size() << std::endl;
        for (auto i = 0; i < value_list.size(); ++i) {
            std::cout << "  res[" << i << "]=(" << value_list[i].first << ", " << value_list[i].second << ")" << std::endl;
            if (value_list[i].first != *((KeyType*)value_list[i].second))
                std::cout << "Error: scan returned an incorrect pair" << std::endl;
        }
        getchar();
        #endif
    }

    int64_t getMemory() const { return 0; }

    void merge() {}

    STXSeqtreeIndex(uint64_t kt) {}

    void print_stats() {
        auto stats = idx.get_stats();
        std::cout   << "STX Seqtree stats:\n";
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


	void print_tree()  {
		std::cout << "\nBEGIN PRINTING A TREE:" << std::endl;
        #ifdef BTREE_DEBUG
		idx.print(cout);
        #endif
		// for (auto it = idx.begin(); it != idx.end(); ++it)
		// 	std::cout << it->first << std::endl;
		std::cout << "END OF PRINTING" << std::endl;
		getchar();
    }

  private:
    stx::blindi_btree_multimap_seqtree<KeyType, ValueType, SeqTreeBlindiNode,
                                       InnerSlots, LeafSlots, KeyComparator>
        idx;
};

template <typename KeyType, typename ValueType, class KeyComparator,
          int InnerSlots, int LeafSlots>
class STXTrieIndex : public Index<KeyType, ValueType, KeyComparator> {
  public:
    ~STXTrieIndex() {}

    void UpdateThreadLocal(size_t thread_num) {}
    void AssignGCID(size_t thread_id) {}
    void UnregisterThread(size_t thread_id) {}

    bool insert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        // printf("Inserting (%d, %d) the address %p\n", key, value, (&key));
        // std::cout << "inserting " << key << std::endl;
        idx.insert(key, value);
        #ifdef DEBUG_PRINT
        static auto xxx = 0;
        if (!((++xxx) % 10000000))
            print_stats();
        // print_tree();
        #endif
        return true;
    }

    ValueType find(const KeyType &key, std::vector<ValueType> *v,
                   threadinfo *ti) {
        auto it = idx.find(key);
        v->clear();
        if (it != idx.end())
            v->push_back(it->second);
        // return it->second;
    }

    bool upsert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        idx.upsert(key, value);
        // auto it = idx.find(key);
        // if (it == idx.end()) {
        //     std::cout << "Error: updating a record that has not been inserted yet" << std::endl;
        //     return true;
        // }
        // it.data() = value;
        // return false;        
        return true;
    }

    void incKey(uint64_t &key) { key++; };
    // void incKey(UUID& key) {
    //   key.val[1]++;
    //   if (key.val[1] == 0)
    //     key.val[0]++;
    // }
    void incKey(GenericKey<31> &key) { key.data[strlen(key.data) - 1]++; };
    void incKey(GenericKey<16> &key) { key.data[strlen(key.data) - 1]++; };

    uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
        std::vector<pair<KeyType, ValueType> > value_list;
        auto it = idx.lower_bound(key);
        for (int i = 0; i < range; i++) {
            if (it == idx.end()) {
                #ifdef DEBUG_PRINT
                std::cout << "WARNING: scan reached the end of the index prematurely" << std::endl;
                #endif
                break;
            }
            value_list.push_back(make_pair(it->first, it->second));
            it++;
        }
        #ifdef DEBUG_PRINT
        std::cout << "results of scan called with: key=" << key << " range=" << range << std::endl;
        std::cout << "  res.size()=" << value_list.size() << std::endl;
        for (auto i = 0; i < value_list.size(); ++i) {
            std::cout << "  res[" << i << "]=(" << value_list[i].first << ", " << value_list[i].second << ")" << std::endl;
            if (value_list[i].first != *((KeyType*)value_list[i].second))
                std::cout << "Error: scan returned an incorrect pair" << std::endl;
        }
        getchar();
        #endif
        return value_list.size();
    }

    int64_t getMemory() const { return 0; }

    void merge() {}

    STXTrieIndex(uint64_t kt) {}

    void print_stats() {
        auto stats = idx.get_stats();
        std::cout << "\nitemcount: " << stats.itemcount << " leaves: " << stats.leaves
                  << " innernodes: " << stats.innernodes << " leafslots: " << stats.leafslots
                  << " innerslots: " << stats.innerslots
                  << " leaf size: " << stats.leafsize << " node size: " << stats.nodesize
                  << " index size (theoretical): " << (stats.leaves * stats.leafsize) + (stats.innernodes * stats.nodesize)
                  << " indexcapacity: " << stats.indexcapacity
                   << "\n";
	}


	void print_tree()  {
		std::cout << "\nBEGIN PRINTING A TREE:" << std::endl;
        // #ifdef BTREE_DEBUG
		// // idx.print(cout);
        // #endif
		// for (auto it = idx.begin(); it != idx.end(); ++it)
		// 	std::cout << it->first << std::endl;
		std::cout << "END OF PRINTING" << std::endl;
		getchar();
    }

  private:
    stx::blindi_btree_multimap_trie<KeyType, ValueType, TrieBlindiNode,
                                    InnerSlots, LeafSlots, KeyComparator>
        idx;
};

template <typename KeyType, typename ValueType, class KeyComparator,
          int InnerSlots, int LeafSlots>
class STXSubtrieIndex : public Index<KeyType, ValueType, KeyComparator> {
  public:
    ~STXSubtrieIndex() {}

    void UpdateThreadLocal(size_t thread_num) {}
    void AssignGCID(size_t thread_id) {}
    void UnregisterThread(size_t thread_id) {}


    bool insert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        idx.insert(key, value);
        return true;
    }

    ValueType find(const KeyType &key, std::vector<ValueType> *v,
                   threadinfo *ti) {
        auto it = idx.find(key);
        v->clear();

        if (it == idx.end())  {
            std::cerr << "READ: key not found" << std::endl;
            getchar();
        }
        auto result = it->second;
        // uint64_t *ptr_key = reinterpret_cast<uint64_t*>(result);
        // uint64_t *ptr_val = reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(result)+sizeof(key));
        // v->clear();
        v->push_back(result);        
        return result;
    }

    bool upsert(const KeyType &key, const ValueType &value, threadinfo *ti) {
        auto it = idx.find(key);
        if (it == idx.end()) {
            std::cout << "Error: updating a record that has not been inserted yet" << std::endl;
            return true;
        }
        it.data() = value;
        return false;        
    }

    void incKey(uint64_t &key) { key++; };
    // void incKey(UUID& key) {
    //   key.val[1]++;
    //   if (key.val[1] == 0)
    //     key.val[0]++;
    // }
    void incKey(GenericKey<31> &key) { key.data[strlen(key.data) - 1]++; };
    void incKey(GenericKey<16> &key) { key.data[strlen(key.data) - 1]++; };

    uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
        std::vector<pair<KeyType, ValueType> > value_list;
        auto it = idx.lower_bound(key);
        for (int i = 0; i < range; i++) {
            if (it == idx.end()) {
                #ifdef DEBUG_PRINT
                std::cout << "WARNING: scan reached the end of the index prematurely" << std::endl;
                #endif
                break;
            }
            value_list.push_back(make_pair(it->first, it->second));
            it++;
        }
        #ifdef DEBUG_PRINT
        std::cout << "results of scan called with: key=" << key << " range=" << range << std::endl;
        std::cout << "  res.size()=" << value_list.size() << std::endl;
        for (auto i = 0; i < value_list.size(); ++i) {
            std::cout << "  res[" << i << "]=(" << value_list[i].first << ", " << value_list[i].second << ")" << std::endl;
            if (value_list[i].first != *((KeyType*)value_list[i].second))
                std::cout << "Error: scan returned an incorrect pair" << std::endl;
        }
        getchar();
        #endif
        return value_list.size();
    }

    int64_t getMemory() const { return 0; }

    void merge() {}

    STXSubtrieIndex(uint64_t kt) {}

    void print_stats() {
        auto stats = idx.get_stats();
        std::cout << "\nitemcount: " << stats.itemcount << " leaves: " << stats.leaves
                  << " innernodes: " << stats.innernodes << " leafslots: " << stats.leafslots
                  << " innerslots: " << stats.innerslots
                  << " leaf size: " << stats.leafsize << " node size: " << stats.nodesize
                  << " index size (theoretical): " << (stats.leaves * stats.leafsize) + (stats.innernodes * stats.nodesize)
                  << " indexcapacity: " << stats.indexcapacity
                   << "\n";
	}


	void print_tree()  {
		std::cout << "\nBEGIN PRINTING A TREE:" << std::endl;
        #ifdef BTREE_DEBUG
		idx.print(cout);
        #endif
		// for (auto it = idx.begin(); it != idx.end(); ++it)
		// 	std::cout << it->first << std::endl;
		std::cout << "END OF PRINTING" << std::endl;
		getchar();
    }

  private:
    stx::blindi_btree_multimap_subtrie<KeyType, ValueType, SubTrieBlindiNode,
                                       InnerSlots, LeafSlots, KeyComparator>
        idx;
};


template <typename KeyType, typename KeyComparator,
          typename KeyEqualityChecker = std::equal_to<KeyType>,
          typename KeyHashFunc = std::hash<KeyType>>
class BwTreeIndex : public Index<KeyType, uint64_t, KeyComparator> {
  public:
    using index_type = BwTree<KeyType, uint64_t, KeyComparator,
                              KeyEqualityChecker, KeyHashFunc>;
    using BaseNode = typename index_type::BaseNode;

    BwTreeIndex(uint64_t kt) {
        index_p = new index_type{};
        assert(index_p != nullptr);
        (void)kt;

        // Print the size of preallocated storage
        fprintf(stderr, "Inner prealloc size = %lu; Leaf prealloc size = %lu\n",
                index_type::INNER_PREALLOCATION_SIZE,
                index_type::LEAF_PREALLOCATION_SIZE);

        return;
    }

    ~BwTreeIndex() {
        delete index_p;
        return;
    }

#ifdef BWTREE_COLLECT_STATISTICS
    void CollectStatisticalCounter(int thread_num) {
        static constexpr int counter_count =
            BwTreeBase::GCMetaData::CounterType::COUNTER_COUNT;
        int counters[counter_count];

        // Aggregate on the array of counters
        memset(counters, 0x00, sizeof(counters));

        for (int i = 0; i < thread_num; i++) {
            for (int j = 0; j < counter_count; j++) {
                counters[j] += index_p->GetGCMetaData(i)->counters[j];
            }
        }

        fprintf(stderr, "Statistical counters:\n");
        for (int j = 0; j < counter_count; j++) {
            fprintf(stderr, "    counter %s = %d\n",
                    BwTreeBase::GCMetaData::COUNTER_NAME_LIST[j], counters[j]);
        }

        return;
    }
#endif

    void AfterLoadCallback() {
        int inner_depth_total = 0, leaf_depth_total = 0, inner_node_total = 0,
            leaf_node_total = 0;
        int inner_size_total = 0, leaf_size_total = 0;
        size_t inner_alloc_total = 0, inner_used_total = 0;
        size_t leaf_alloc_total = 0, leaf_used_total = 0;

        uint64_t index_root_id = index_p->root_id.load();
        fprintf(stderr, "BwTree - Start consolidating delta chains...\n");
        int ret = index_p->DebugConsolidateAllRecursive(
            index_root_id, &inner_depth_total, &leaf_depth_total,
            &inner_node_total, &leaf_node_total, &inner_size_total,
            &leaf_size_total, &inner_alloc_total, &inner_used_total,
            &leaf_alloc_total, &leaf_used_total);
        fprintf(stderr, "BwTree - Finished consolidating %d delta chains\n",
                ret);

        fprintf(stderr, "    Inner Avg. Depth: %f (%d / %d)\n",
                (double)inner_depth_total / (double)inner_node_total,
                inner_depth_total, inner_node_total);
        fprintf(stderr, "    Inner Avg. Size: %f (%d / %d)\n",
                (double)inner_size_total / (double)inner_node_total,
                inner_size_total, inner_node_total);
        fprintf(stderr, "    Leaf Avg. Depth: %f (%d / %d)\n",
                (double)leaf_depth_total / (double)leaf_node_total,
                leaf_depth_total, leaf_node_total);
        fprintf(stderr, "    Leaf Avg. Size: %f (%d / %d)\n",
                (double)leaf_size_total / (double)leaf_node_total,
                leaf_size_total, leaf_node_total);

        fprintf(stderr, "Inner Alloc. Util: %f (%lu / %lu)\n",
                (double)inner_used_total / (double)inner_alloc_total,
                inner_used_total, inner_alloc_total);

        fprintf(stderr, "Leaf Alloc. Util: %f (%lu / %lu)\n",
                (double)leaf_used_total / (double)leaf_alloc_total,
                leaf_used_total, leaf_alloc_total);

        // Only do thid after the consolidation, because the mapping will change
        // during the consolidation

#ifndef BWTREE_USE_MAPPING_TABLE
        fprintf(stderr, "Replacing all NodeIDs to BaseNode *\n");
        BaseNode *node_p =
            (BaseNode *)index_p->GetNode(index_p->root_id.load());
        index_p->root_id = reinterpret_cast<NodeID>(node_p);
        index_p->DebugReplaceNodeIDRecursive(node_p);
#endif
        return;
    }

    void UpdateThreadLocal(size_t thread_num) {
        index_p->UpdateThreadLocal(thread_num);
    }

    void AssignGCID(size_t thread_id) { index_p->AssignGCID(thread_id); }

    void UnregisterThread(size_t thread_id) {
        index_p->UnregisterThread(thread_id);
    }

    bool insert(const KeyType &key, const uint64_t &value, threadinfo *) {
        return index_p->Insert(key, value);
    }

    uint64_t find(const KeyType &key, std::vector<uint64_t> *v, threadinfo *) {
        index_p->GetValue(key, *v);

        return 0UL;
    }

#ifndef BWTREE_USE_MAPPING_TABLE
    uint64_t find_bwtree_fast(KeyType key, std::vector<uint64_t> *v) {
        index_p->GetValueNoMappingTable(key, *v);

        return 0UL;
    }
#endif

#ifndef BWTREE_USE_DELTA_UPDATE
    bool insert_bwtree_fast(KeyType key, uint64_t value) {
        index_p->InsertInPlace(key, value);
        return true;
    }
#endif

    bool upsert(const KeyType &key, const uint64_t &value, threadinfo *) {
        // index_p->Delete(key, value);
        // index_p->Insert(key, value);

        index_p->Upsert(key, value);

        return true;
    }

    uint64_t scan(const KeyType &key, int range, threadinfo *) {
        auto it = index_p->Begin(key);

        if (it.IsEnd() == true) {
            std::cout << "Iterator reaches the end\n";
            return 0UL;
        }

        uint64_t sum = 0;
        for (int i = 0; i < range; i++) {
            if (it.IsEnd() == true) {
                return sum;
            }

            sum += it->second;
            it++;
        }

        return sum;
    }

    int64_t getMemory() const { return 0L; }

    void print_stats() { printf("\nINDEX STATS REQUESTED: N/A\n"); }

	void print_tree()  { printf("\nTREE PRINTING REQUESTED: N/A\n"); }

  private:
    BwTree<KeyType, uint64_t, KeyComparator, KeyEqualityChecker, KeyHashFunc>
        *index_p;
};

template <typename KeyType, class KeyComparator>
class MassTreeIndex : public Index<KeyType, uint64_t, KeyComparator> {
  public:
    typedef mt_index<Masstree::default_table> MapType;

    ~MassTreeIndex() { delete idx; }

    inline void swap_endian(uint64_t &i) {
        // Note that masstree internally treat input as big-endian
        // integer values, so we need to swap here
        // This should be just one instruction
        i = __bswap_64(i);
    }

    // inline void swap_endian(UUID &i) {
    //   // Note that masstree internally treat input as big-endian
    //   // integer values, so we need to swap here
    //   // This should be just one instruction
    //   auto x = __bswap_64(i.val[1]);
    //   i.val[1] = __bswap_64(i.val[0]);
    //   i.val[0] = x;
    // }

    inline void swap_endian(GenericKey<31> &) { return; }

    inline void swap_endian(GenericKey<16> &) { return; }

    void UpdateThreadLocal(size_t thread_num) {}
    void AssignGCID(size_t thread_id) {}
    void UnregisterThread(size_t thread_id) {}

    bool insert(const KeyType &key, const uint64_t &value, threadinfo *ti) {
        KeyType key_ = key;
        swap_endian(key_);
        idx->put((const char *)&key_, sizeof(KeyType), (const char *)&value, 8,
                 ti);

        return true;
    }

    uint64_t find(const KeyType &key, std::vector<uint64_t> *v,
                  threadinfo *ti) {
        Str val;
        KeyType key_ = key;
        swap_endian(key_);
        idx->get((const char *)&key_, sizeof(KeyType), val, ti);

        v->clear();
        if (val.s)
            v->push_back(*(uint64_t *)val.s);

        return 0;
    }

    bool upsert(const KeyType &key, const uint64_t &value, threadinfo *ti) {
        KeyType key_ = key;
        swap_endian(key_);
        idx->put((const char *)&key_, sizeof(KeyType), (const char *)&value, 8,
                 ti);
        return true;
    }

    // uint64_t scan(KeyType key, int range, threadinfo *ti) {
    //   Str val;

    //   swap_endian(key);
    //   int key_len = sizeof(KeyType);

    //   for (int i = 0; i < range; i++) {
    //     idx->dynamic_get_next(val, (char *)&key, &key_len, ti);
    //   }

    //   return 0UL;
    // }

    uint64_t scan(const KeyType &key, int range, threadinfo *ti) {
        Str results[range];

        KeyType key_ = key;
        swap_endian(key_);
        int key_len = sizeof(KeyType);

        int resultCount =
            idx->get_next_n(results, (char *)&key_, &key_len, range, ti);
        // printf("scan: requested: %d, actual: %d\n", range, resultCount);
        return resultCount;
    }

    int64_t getMemory() const { return 0; }

    MassTreeIndex(uint64_t kt) {
        idx = new MapType{};

        threadinfo *main_ti = threadinfo::make(threadinfo::TI_MAIN, -1);
        idx->setup(main_ti);

        return;
    }

    void print_stats() { printf("\nINDEX STATS REQUESTED: N/A\n"); }

	void print_tree()  { printf("\nTREE PRINTING REQUESTED: N/A\n"); }

    MapType *idx;
};


#endif
