#pragma once

#include <atomic>
#include <cassert>
#include <cstring>
#include <immintrin.h>
#include <sched.h>

#include "blindi_seqtree.hpp"

#ifdef BTREEOLC_DEBUG
// #include <iostream>
// #include <cstdio>

// #define BTREEOLC_PRINTF(...)           {fprintf(stderr, __VA_ARGS__); fflush(stderr);}

// /// Print out debug information to std::cout if BTREE_DEBUG is defined.
// #define BTREEOLC_PRINT(x)          do { if (debug) (std::cout << x << std::endl); } while(0)

// /// Assertion only if BTREE_DEBUG is defined. This is not used in verify().
// #define BTREEOLC_ASSERT(x)         do { assert(x); } while(0)

// #else

// /// Print out debug information to std::cout if BTREE_DEBUG is defined.
// #define BTREEOLC_PRINTF(...)          do { } while(0)

// /// Assertion only if BTREE_DEBUG is defined. This is not used in verify().
// #define BTREEOLC_ASSERT(x)         do { } while(0)

#endif // BTREEOLC_DEBUG

namespace blindi {

// static const uint64_t pageSize = 4 * 1024;

// static const uint64_t LEAFSLOTS = 16;
// static const uint64_t LEAFSLOTS = (pageSize-sizeof(NodeBase))/(sizeof(Key)+sizeof(Payload));
// static const uint64_t INNERSLOTS = (pageSize - sizeof(NodeBase)) / (sizeof(Key) + sizeof(NodeBase *));
// static const uint16_t _Key_len = 8;

enum class PageType : uint8_t { BTreeInner = 1, BTreeLeaf = 2 };

struct OptLock {
    std::atomic<uint64_t> typeVersionLockObsolete{0b100};

    bool isLocked(uint64_t version) { return ((version & 0b10) == 0b10); }

    uint64_t readLockOrRestart(bool &needRestart) {
        uint64_t version;
        version = typeVersionLockObsolete.load();
        if (isLocked(version) || isObsolete(version)) {
            _mm_pause();
            needRestart = true;
        }
        return version;
    }

    void writeLockOrRestart(bool &needRestart) {
        uint64_t version;
        version = readLockOrRestart(needRestart);
        if (needRestart)
            return;

        upgradeToWriteLockOrRestart(version, needRestart);
        if (needRestart)
            return;
    }

    void upgradeToWriteLockOrRestart(uint64_t &version, bool &needRestart) {
        if (typeVersionLockObsolete.compare_exchange_strong(version,
                                                            version + 0b10)) {
            version = version + 0b10;
        } else {
            _mm_pause();
            needRestart = true;
        }
    }

    void writeUnlock() { typeVersionLockObsolete.fetch_add(0b10); }

    bool isObsolete(uint64_t version) { return (version & 1) == 1; }

    void checkOrRestart(uint64_t startRead, bool &needRestart) const {
        readUnlockOrRestart(startRead, needRestart);
    }

    void readUnlockOrRestart(uint64_t startRead, bool &needRestart) const {
        needRestart = (startRead != typeVersionLockObsolete.load());
    }

    void writeUnlockObsolete() { typeVersionLockObsolete.fetch_add(0b11); }
};

struct NodeBase : public OptLock {
    PageType type;

	virtual void print_node(std::ostream &os, unsigned int depth, bool recursive, const std::string &prefix) = 0;

};

struct BTreeLeafBase : public NodeBase {
    static const PageType typeMarker = PageType::BTreeLeaf;
};

template <typename Key, typename Payload, uint64_t _maxLeafExtries> struct BTreeLeaf : public BTreeLeafBase {

    //  struct Entry {
    //     Key k;
    //     Payload p;
    //  };

    //  static const uint64_t
    //  maxEntries=(pageSize-sizeof(NodeBase))/(sizeof(Key)+sizeof(Payload));

    //  Key keys[maxEntries];
    //  Payload payloads[maxEntries];

    typedef GenericBlindiSeqTreeNode<SeqTreeBlindiNode<Key, _maxLeafExtries>,
                                      Key, _maxLeafExtries> blindi_node_t;
    blindi_node_t blindi_node;

    BTreeLeaf() {
        type = typeMarker;
        // count = 0;
    }

    ~BTreeLeaf() {
    }

    void print_key(std::ostream &os, uint8_t* key) {
//        os << *((Key*)(key)) << " ";
	// os << " ( ";
	// 	for (int j = (sizeof(Key) - 1); j>= 0; j--) {
	// 		std::cout << (int)key[j] << " ";
	// 	}
    //         os << ") ";
    }

	virtual void print_node(std::ostream &os,
							unsigned int depth, bool recursive, const std::string &prefix) {
		os << std::endl << prefix << ": ";
		for (unsigned int i = 0; i < depth; i++)
			os << "  ";

		blindi_node.get_valid_len();
		os	<< "node " << this
			// << " level " << node->level
			<< " uses " << blindi_node.get_valid_len() << " slots"
			<< " has a valid length " << ((int)blindi_node.get_valid_len())
			<< std::endl;

		for (unsigned int i = 0; i < depth; i++)
			os << "  ";

		for (unsigned int slot = 0; slot < blindi_node.get_valid_len(); ++slot) {
			print_key(os, blindi_node.node.get_key_ptr(slot));
		}
		os << std::endl;
	}

    bool isFull() {
        return blindi_node.get_valid_len() == _maxLeafExtries;
    }

    // unsigned lowerBound(Key k) {
    //   unsigned lower=0;
    //   unsigned upper=count;
    //   do {
    //     unsigned mid=((upper-lower)/2)+lower;
    //     if (k<keys[mid]) {
    //       upper=mid;
    //     } else if (k>keys[mid]) {
    //       lower=mid+1;
    //     } else {z
    //       return mid;
    //     }
    //   } while (lower<upper);
    //   return lower;
    // }

    //  unsigned lowerBoundBF(Key k) {
    //     auto base=keys;
    //     unsigned n=count;
    //     while (n>1) {
    //        const unsigned half=n/2;
    //        base=(base[half]<k)?(base+half):base;
    //        n-=half;
    //     }
    //     return (*base<k)+base-keys;
    //  }

    // returns values of an enum type: INSERT_SUCCESS, INSERT_DUPLICATED_KEY or INSERT_OVERFLOW
    int insert(const Key &k, const Payload &p) {
        // BTREEOLC_PRINTF("      BTreeLeaf::insert starts with (%p, %p)", &k, &p);
        // if (isFull()) {
        //     BTREEOLC_PRINTF("  ... INSERT_RESULT = OVERFLOW (anticipated)\n");
        //     throw 1;
        // }
		return blindi_node.Insert2BlindiNodeWithKey((uint8_t *)&k, sizeof(Key));
        // std:cerr << "INSERTED NODE WITH THE KEY " << k << std::endl;
		// this->print_node(cerr, 0, false, "insertee");
	}

    // returns values of an enum type: INSERT_SUCCESS, INSERT_DUPLICATED_KEY or INSERT_OVERFLOW
    int upsert(const Key &k, const Payload &p) {
		return blindi_node.Upsert2BlindiNodeWithKey((uint8_t *)&k, sizeof(Key));
	}

    BTreeLeaf *split(Key &sep) {
        BTreeLeaf *newLeaf = new BTreeLeaf();
        blindi_node_t blindi_node_copy;
        this->blindi_node.SplitBlindiNode(&blindi_node_copy,
                                            &(newLeaf->blindi_node)
                                        );
        memcpy(&blindi_node, &blindi_node_copy, sizeof(blindi_node_copy));
        sep = blindi_node.get_max_key_in_node();
        return newLeaf;
    }
};

struct BTreeInnerBase : public NodeBase {
    static const PageType typeMarker = PageType::BTreeInner;
    uint16_t count;
};

template <typename Key, uint64_t _maxEntries> struct BTreeInner : public BTreeInnerBase {
    // static const uint64_t maxEntries = _maxEntries;

    NodeBase *children[_maxEntries];
    Key keys[_maxEntries];

    BTreeInner() {
        count = 0;
        type = typeMarker;
    }

    ~BTreeInner() {
    }

	virtual void print_node(std::ostream &os,
							unsigned int depth, bool recursive, const std::string &prefix) {
		os << std::endl << prefix << ":" ;

		for (unsigned int i = 0; i < depth; i++)
			os << "  ";

		os	<< "node " << this
			// << " level " << node->level
			<< " uses " << count << " slots"
			<< std::endl;

		for (unsigned int i = 0; i < depth; i++)
			os << "  ";

		for (unsigned short slot = 0; slot < count; ++slot) {
			os << "(" << children[slot] << ") "
				<< " ( ";
			for (int j = (sizeof(Key) - 1); j >= 0; j--) {
				uint8_t *t = (uint8_t *)&keys[slot];
				os << (int)t[j] << " ";
			}
			os << " ) ";
		}
		// os << "(" << children[count] << ")"
		// 	<< std::endl;

		if (recursive) {
			for (unsigned short slot = 0; slot < count; ++slot) {
				children[slot]->print_node(os, depth + 1, recursive, "");
			}
		}
	}

    bool isFull() { return count == (_maxEntries - 1); };

    unsigned lowerBoundBF(Key k) {
        auto base = keys;
        unsigned n = count;
        while (n > 1) {
            const unsigned half = n / 2;
            base = (base[half] < k) ? (base + half) : base;
            n -= half;
        }
        return (*base < k) + base - keys;
    }

    unsigned lowerBound(Key k) {
        unsigned lower = 0;
        unsigned upper = count;
        do {
            unsigned mid = ((upper - lower) / 2) + lower;
            if (k < keys[mid]) {
                upper = mid;
            } else if (keys[mid] < k) {
                lower = mid + 1;
            } else {
                return mid;
            }
        } while (lower < upper);
        return lower;
    }

    BTreeInner *split(Key &sep) {
        BTreeInner *newInner = new BTreeInner();
        newInner->count = count - (count / 2);
        count = count - newInner->count - 1;
        sep = keys[count];
        memcpy(newInner->keys, keys + count + 1,
               sizeof(Key) * (newInner->count + 1));
        memcpy(newInner->children, children + count + 1,
               sizeof(NodeBase *) * (newInner->count + 1));
        return newInner;
    }

    void insert(const Key &k, NodeBase *child) {
        assert(count < _maxEntries - 1);
        unsigned pos = lowerBound(k);
        memmove(keys + pos + 1, keys + pos, sizeof(Key) * (count - pos + 1));
        memmove(children + pos + 1, children + pos,
                sizeof(NodeBase *) * (count - pos + 1));
        keys[pos] = k;
        children[pos] = child;
        std::swap(children[pos], children[pos + 1]);
        count++;
    }
};

template <typename Key,
          typename Value,
          int _InnerSlots,
          int _LeafSlots,
          typename _Compare = std::less<Key>,
          std::uint16_t _Key_len = sizeof(Key)>
struct BTree {
  std::atomic<NodeBase *> root;

  typedef GenericBlindiSeqTreeNode<SeqTreeBlindiNode<Key, _LeafSlots>, Key, _LeafSlots> blindi_node_t;

    struct btreeolc_stats {
        uint64_t inner_size;
        uint64_t leaf_size;
        uint64_t inner_num;
        uint64_t leaf_num;
        uint64_t leaf_used;
        uint64_t inner_used;
        uint64_t leaf_slots;
        uint64_t inner_slots;

        btreeolc_stats(): inner_size(0), leaf_size(0), inner_num(0), leaf_num(0), leaf_used(0), inner_used(0), leaf_slots(_LeafSlots), inner_slots(_InnerSlots) {}

        void print() {
            std::cout   << "* leaf size:    " << leaf_size << std::endl 	
                        << "* leaf num:     " << leaf_num << std::endl
                        << "* leaf slots:   " << leaf_slots << std::endl
                        << "* inner size:   " << inner_size << std::endl
                        << "* inner num:    " << inner_num << std::endl
                        << "* inner slots:  " << inner_slots << std::endl
                        << "* theoretical index size: " << (leaf_size*leaf_num+inner_size*inner_num) << std::endl;
        }
    } stats;

    btreeolc_stats get_stats() {
        return stats;
    }

    BTree() {
        root = new BTreeLeaf<Key, Value, _LeafSlots>();
        #ifdef BTREEOLC_DEBUG
        BTreeLeaf<Key, Value, _LeafSlots> *new_leaf = new BTreeLeaf<Key, Value, _LeafSlots>();
        BTreeInner<Key, _InnerSlots> *new_inner = new BTreeInner<Key, _InnerSlots>();
        stats.inner_size = sizeof(*new_inner);
        stats.leaf_size = sizeof(*new_leaf);
        stats.leaf_num += 1;
        delete new_leaf;
        delete new_inner;
        #endif
    }

    void makeRoot(Key k, NodeBase *leftChild, NodeBase *rightChild) {
        auto inner = new BTreeInner<Key, _InnerSlots>();
        #ifdef BTREEOLC_DEBUG
        stats.inner_num += 1;
        #endif
        inner->count = 1;
        inner->keys[0] = k;
        inner->children[0] = leftChild;
        inner->children[1] = rightChild;
        root = inner;

    }

    void yield(int count) {
        if (count > 3)
            sched_yield();
        else
            _mm_pause();
    }

    void print_tree(std::ostream &os) {
        NodeBase *node = root;
        node->print_node(os, 0, true, "the whole tree");
    }

    void insert(const Key &k, const Value &v) {
        // BTREEOLC_PRINTF("    BTree::insert started: &key=%p and &value=%p\n", &k, &v);
        int restartCount = 0;
    restart:
        if (restartCount++)
            yield(restartCount);
        bool needRestart = false;

        // Current node
        NodeBase *node = root;
        uint64_t versionNode = node->readLockOrRestart(needRestart);
        if (needRestart || (node != root))
            goto restart;

        // Parent of current node
        BTreeInner<Key, _InnerSlots> *parent = nullptr;
        uint64_t versionParent;

        while (node->type == PageType::BTreeInner) {
            auto inner = static_cast<BTreeInner<Key, _InnerSlots> *>(node);

            // Split eagerly if full
            if (inner->isFull()) {
                // Lock
                if (parent) {
                    parent->upgradeToWriteLockOrRestart(versionParent,
                                                        needRestart);
                    if (needRestart)
                        goto restart;
                }
                node->upgradeToWriteLockOrRestart(versionNode, needRestart);
                if (needRestart) {
                    if (parent)
                        parent->writeUnlock();
                    goto restart;
                }
                if (!parent && (node != root)) { // there's a new parent
                    node->writeUnlock();
                    goto restart;
                }
                // Split
                Key sep;
                BTreeInner<Key, _InnerSlots> *newInner = inner->split(sep);
                #ifdef BTREEOLC_DEBUG
                stats.inner_num += 1;
                #endif
                if (parent)
                    parent->insert(sep, newInner);
                else {
                    makeRoot(sep, inner, newInner);
                }
                // Unlock and restart
                node->writeUnlock();
                if (parent)
                    parent->writeUnlock();
                goto restart;
            }

            if (parent) {
                parent->readUnlockOrRestart(versionParent, needRestart);
                if (needRestart)
                    goto restart;
            }

            parent = inner;
            versionParent = versionNode;

            node = inner->children[inner->lowerBound(k)];
            inner->checkOrRestart(versionNode, needRestart);
            if (needRestart)
                goto restart;
            versionNode = node->readLockOrRestart(needRestart);
            if (needRestart)
                goto restart;
        }

        auto leaf = static_cast<BTreeLeaf<Key, Value, _LeafSlots> *>(node);

        // Split leaf if full
        if (leaf->isFull()) { //(leaf->count==leaf->maxEntries) {
            // fprintf(stderr, "... SPLITTING IS NEEDED ... "); fflush(stderr);
            // Lock
            // #ifdef BTREEOLC_DEBUG
            // std::cerr << std::endl << "Splitting is needed when called insert(" << k << "," << v << ")" << std::endl;
            // stats.print();
            // if (parent)
            //     std::cerr << "Parent size: " << parent->count << std::endl;
            // else
            //     std::cerr << "No parent" << std::endl;
            // getchar();
            // NodeBase *root_node = root;
            // root_node->print_node(std::cerr, 0, true, "debugging insert");
            // #endif
            if (parent) {
                parent->upgradeToWriteLockOrRestart(versionParent, needRestart);
                if (needRestart)
                    goto restart;
            }
            node->upgradeToWriteLockOrRestart(versionNode, needRestart);
            if (needRestart) {
                if (parent)
                    parent->writeUnlock();
                goto restart;
            }
            if (!parent && (node != root)) { // there's a new parent
                node->writeUnlock();
                goto restart;
            }
            // Split
            Key sep;
            BTreeLeaf<Key, Value, _LeafSlots> *newLeaf = leaf->split(sep);
            #ifdef BTREEOLC_DEBUG
            stats.leaf_num += 1;
            #endif
            if (parent)
                parent->insert(sep, newLeaf);
            else
                makeRoot(sep, leaf, newLeaf);
            // Unlock and restart
            node->writeUnlock();
            if (parent)
                parent->writeUnlock();
            // #ifdef BTREEOLC_DEBUG
            // if (parent)
            //     std::cerr << "Parent size: " << parent->count << std::endl;
            // else
            //     std::cerr << "No parent" << std::endl;
            // stats.print();
            // leaf->print_node(std::cerr, 0, false, "leaf");
            // newLeaf->print_node(std::cerr, 0, false, "new_leaf");
            // root_node = root;
            // root_node->print_node(std::cerr, 0, false, "root");                
            // getchar();
            // #endif
            goto restart;
        } else {
            // only lock leaf node
            node->upgradeToWriteLockOrRestart(versionNode, needRestart);
            if (needRestart)
                goto restart;
            if (parent) {
                parent->readUnlockOrRestart(versionParent, needRestart);
                if (needRestart) {
                    node->writeUnlock();
                    goto restart;
                }
            }
            leaf->insert(k, v);
            node->writeUnlock();
            return; // success
        }
    }

    bool upsert(const Key &k, const Value &v) {
        // BTREEOLC_PRINTF("    BTree::lookup started: &key=%p\n", &k);
        // std::cerr << "... Starting a lookup" << std::endl;
        int restartCount = 0;
    restart:
        if (restartCount++)
            yield(restartCount);
        bool needRestart = false;

        NodeBase *node = root;
        uint64_t versionNode = node->readLockOrRestart(needRestart);
        if (needRestart || (node != root))
            goto restart;

        // Parent of current node
        BTreeInner<Key, _InnerSlots> *parent = nullptr;
        uint64_t versionParent;

        while (node->type == PageType::BTreeInner) {
            auto inner = static_cast<BTreeInner<Key, _InnerSlots> *>(node);

            if (parent) {
                parent->readUnlockOrRestart(versionParent, needRestart);
                if (needRestart)
                    goto restart;
            }

            parent = inner;
            versionParent = versionNode;

            node = inner->children[inner->lowerBound(k)];
            inner->checkOrRestart(versionNode, needRestart);
            if (needRestart)
                goto restart;
            versionNode = node->readLockOrRestart(needRestart);
            if (needRestart)
                goto restart;
        }

        BTreeLeaf<Key, Value, _LeafSlots> *leaf =
            static_cast<BTreeLeaf<Key, Value, _LeafSlots> *>(node);
        bool hit = false, smaller_than_node = false;
        uint16_t tree_traverse[_LeafSlots];
        uint16_t tree_traverse_len;

        int slot = leaf->blindi_node.SearchBlindiNode(
            (uint8_t *)&k, sizeof(k), SEARCH_TYPE::POINT_SEARCH, &hit,
            &smaller_than_node, tree_traverse, &tree_traverse_len);

        if (hit) {
            auto ptr = leaf->blindi_node.get_ptr2key_ptr();
            ptr[slot] = reinterpret_cast<uint8_t*>(v);
        }
        // result = reinterpret_cast<uint64_t>(leaf->blindi_node.get_key_ptr(slot)); 
        return hit;
    }

    bool lookup(const Key &k, Value &result) {
        // BTREEOLC_PRINTF("    BTree::lookup started: &key=%p\n", &k);
        // std::cerr << "... Starting a lookup" << std::endl;
        int restartCount = 0;
    restart:
        if (restartCount++)
            yield(restartCount);
        bool needRestart = false;

        NodeBase *node = root;
        uint64_t versionNode = node->readLockOrRestart(needRestart);
        if (needRestart || (node != root))
            goto restart;

        // Parent of current node
        BTreeInner<Key, _InnerSlots> *parent = nullptr;
        uint64_t versionParent;

        while (node->type == PageType::BTreeInner) {
            auto inner = static_cast<BTreeInner<Key, _InnerSlots> *>(node);

            if (parent) {
                parent->readUnlockOrRestart(versionParent, needRestart);
                if (needRestart)
                    goto restart;
            }

            parent = inner;
            versionParent = versionNode;

            node = inner->children[inner->lowerBound(k)];
            inner->checkOrRestart(versionNode, needRestart);
            if (needRestart)
                goto restart;
            versionNode = node->readLockOrRestart(needRestart);
            if (needRestart)
                goto restart;
        }

        BTreeLeaf<Key, Value, _LeafSlots> *leaf =
            static_cast<BTreeLeaf<Key, Value, _LeafSlots> *>(node);
        // // THIS IS THE OLD CODE
        // unsigned pos = leaf->lowerBound(k);
        // bool success;
        // if ((pos < leaf->count) && (leaf->keys[pos] == k)) {
        //   success = true;
        //   result = leaf->payloads[pos];
        // }
        // if (parent) {
        //   parent->readUnlockOrRestart(versionParent, needRestart);
        //   if (needRestart)
        //     goto restart;
        // }
        // node->readUnlockOrRestart(versionNode, needRestart);
        // if (needRestart)
        //   goto restart;
        // return success;
        /// BELOW IS WHAT SHOULD REPLACE IT
        bool hit = false, smaller_than_node = false;
        uint16_t tree_traverse[_LeafSlots];
        uint16_t tree_traverse_len;

        int slot = leaf->blindi_node.SearchBlindiNode(
            (uint8_t *)&k, sizeof(k), SEARCH_TYPE::POINT_SEARCH, &hit,
            &smaller_than_node, tree_traverse, &tree_traverse_len);
        result = reinterpret_cast<uint64_t>(leaf->blindi_node.get_key_ptr(slot)); 
        return hit;
    }

    uint64_t scan(Key k, int range, Value *output) {
        std::cout << "SCAN: Not Implemented Yet" <<std::endl;
        exit(1);
        return 0;
    //   int restartCount = 0;
    // restart:
    //   if (restartCount++) yield(restartCount);
    //   bool needRestart = false;

    //   NodeBase *node = root;
    //   uint64_t versionNode = node->readLockOrRestart(needRestart);
    //   if (needRestart || (node != root)) goto restart;

    //   // Parent of current node
    //   BTreeInner<Key> *parent = nullptr;
    //   uint64_t versionParent;

    //   while (node->type == PageType::BTreeInner) {
    //     auto inner = static_cast<BTreeInner<Key> *>(node);

    //     if (parent) {
    //       parent->readUnlockOrRestart(versionParent, needRestart);
    //       if (needRestart) goto restart;
    //     }

    //     parent = inner;
    //     versionParent = versionNode;

    //     node = inner->children[inner->lowerBound(k)];
    //     inner->checkOrRestart(versionNode, needRestart);
    //     if (needRestart) goto restart;
    //     versionNode = node->readLockOrRestart(needRestart);
    //     if (needRestart) goto restart;
    //   }

    //   BTreeLeaf<Key, Value> *leaf = static_cast<BTreeLeaf<Key, Value>
    //   *>(node); unsigned pos = leaf->lowerBound(k); int count = 0; for
    //   (unsigned i = pos; i < leaf->count; i++) {
    //     if (count == range) break;
    //     output[count++] = leaf->payloads[i];
    //   }

    //   if (parent) {
    //     parent->readUnlockOrRestart(versionParent, needRestart);
    //     if (needRestart) goto restart;
    //   }
    //   node->readUnlockOrRestart(versionNode, needRestart);
    //   if (needRestart) goto restart;

    //   return count;
    }
};

// /// Recursively descend down the tree and print out nodes.
// template <class Key, class Value, uint64_t _InnerSlots, uint64_t LeafSlots>
// void print_node(const NodeBase *node, std::ostream &os,
// 						unsigned int depth, bool recursive) {
// 	for (unsigned int i = 0; i < depth; i++)
// 		os << "  ";


// 	if (node->type == PageType::BTreeLeaf) {
// 		auto *leafnode = static_cast<const BTreeLeaf<Key, Value, _LeafSlots> *>(node);

//         os	<< "node " << node
//             // << " level " << node->level
//             << " slotuse " << leafnode->blindi_node.get_valid_len()
//             << std::endl;

// 		for (unsigned int i = 0; i < depth; i++)
// 			os << "  ";

// 		for (unsigned int slot = 0; slot < leafnode->slotuse; ++slot) {
// 			print_key(os, leafnode->blindi_seqtree.node.get_key_ptr(slot));
// 		}
// 		os << std::endl;

// 	} else {
// 		auto *innernode = static_cast<const BTreeInner<Key, _InnerSlots> *>(node);

//         os	<< "node " << node
//             // << " level " << node->level
//             << " slotuse " << innernode->count
//             << std::endl;

// 		for (unsigned int i = 0; i < depth; i++)
// 			os << "  ";

// 		for (unsigned short slot = 0; slot < innernode->slotuse; ++slot) {
// 			os << "(" << innernode->childid[slot] << ") "
// 				<< " ( ";
// 			for (int j = (sizeof(Key) - 1); j >= 0; j--) {
// 				uint8_t *t = (uint8_t *)&innernode->slotkey[slot];
// 				os << (int)t[j] << " ";
// 			}
// 			os << " ) ";
// 		}
// 		os << "(" << innernode->childid[innernode->slotuse] << ")"
// 			<< std::endl;

// 		if (recursive) {
// 			for (unsigned short slot = 0; slot < innernode->slotuse + 1;
// 					++slot) {
// 				print_node(os, innernode->childid[slot], depth + 1,
// 							recursive);
// 			}
// 		}
// 	}
// }

} // namespace blindi
