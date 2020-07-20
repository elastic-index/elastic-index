#pragma once

#include <atomic>
#include <cassert>
#include <cstring>
#include <immintrin.h>
#include <sched.h>

#include "blindi_subtrie.hpp"

namespace blindi_subtrie {

    
enum class PageType : uint8_t { BTreeInner = 1, BTreeLeaf = 2 };

static const uint64_t pageSize = 4 * 1024;

static const uint64_t LEAFSLOTS = 16;
// static const uint64_t LEAFSLOTS = (pageSize-sizeof(NodeBase))/(sizeof(Key)+sizeof(Payload));
// static const uint64_t INNERSLOTS = (pageSize - sizeof(NodeBase)) / (sizeof(Key) + sizeof(NodeBase *));
// static const uint16_t _Key_len = 8;

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

template <typename Key, typename Payload, uint64_t _maxLeafExtries>
struct BTreeLeaf : public BTreeLeafBase {

    //  struct Entry {
    //     Key k;
    //     Payload p;
    //  };

    //  static const uint64_t
    //  maxEntries=(pageSize-sizeof(NodeBase))/(sizeof(Key)+sizeof(Payload));

    //  Key keys[maxEntries];
    //  Payload payloads[maxEntries];

    typedef GenericBlindiSubTrieNode<SubTrieBlindiNode<Key, _maxLeafExtries>,
                                      Key, _maxLeafExtries> blindi_node_t;
    blindi_node_t blindi_node;

    BTreeLeaf() {
        type = typeMarker;
        // count = 0; 
    }

    ~BTreeLeaf() {
    }

    void print_key(std::ostream &os, uint8_t* key) {
	os << " ( ";
		for (int j = (sizeof(Key) - 1); j>= 0; j--) {
			std::cout << (int)key[j] << " ";
		}
            os << ") ";
    }

	virtual void print_node(std::ostream &os,
							unsigned int depth, bool recursive, const std::string &prefix) {
		os << std::endl << prefix << ": ";
		for (unsigned int i = 0; i < depth; i++)
			os << "  ";

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
    //     } else {
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
        // Payload not used???
        // std:cerr << "INSERTED NODE WITH THE KEY " << k << std::endl;
		// this->print_node(cerr, 0, false, "insertee");
	}

    // returns values of an enum type: INSERT_SUCCESS, INSERT_DUPLICATED_KEY or INSERT_OVERFLOW
    int upsert(const Key &k, const Payload &p) {
		return blindi_node.Upsert2BlindiNodeWithKey((uint8_t *)&k, sizeof(Key));
	}

    BTreeLeaf *split(Key &sep) {
        // fprintf(stderr, "... SPLITTING IN PROGRESS ..."); fflush(stderr);
        BTreeLeaf *newLeaf = new BTreeLeaf();
        // newLeaf->count = count - (count / 2);
        // count = count - newLeaf->count;
        blindi_node_t blindi_node_copy;
        this->blindi_node.SplitBlindiNode(&(newLeaf->blindi_node),
                                          &blindi_node_copy);
        // this->blindi_node = blindi_node_copy;
        memcpy(&blindi_node, &blindi_node_copy, sizeof(blindi_node_copy));
        // fprintf(stderr, "... MEMCPIED ...");
        // fflush(stderr);
        // memcpy(newLeaf->keys, keys+count, sizeof(Key)*newLeaf->count);
        // memcpy(newLeaf->payloads, payloads+count,
        // sizeof(Payload)*newLeaf->count); sep = keys[count-1];
		// newLeaf->print_node(cerr, 0, false, "new leaf");
		// this->print_node(cerr, 0, false, "this leaf");
        sep = newLeaf->blindi_node.get_max_key_in_node();
        // fprintf(stderr, "... SPLIT\n"); fflush(stderr);
        return newLeaf;
    }
};

struct BTreeInnerBase : public NodeBase {
    static const PageType typeMarker = PageType::BTreeInner;
    uint16_t count;
};

template <typename Key, uint64_t _maxEntries> struct BTreeInner : public BTreeInnerBase {
    static const uint64_t maxEntries = _maxEntries;

    NodeBase *children[maxEntries];
    Key keys[maxEntries];

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

    bool isFull() { return count == (maxEntries - 1); };

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
        assert(count < maxEntries - 1);
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

    typedef GenericBlindiSubTrieNode<SubTrieBlindiNode<Key, _LeafSlots>,
                                      Key, _LeafSlots> blindi_node_t;

    BTree() { root = new BTreeLeaf<Key, Value, _LeafSlots>(); }

    void makeRoot(Key k, NodeBase *leftChild, NodeBase *rightChild) {
        auto inner = new BTreeInner<Key, _InnerSlots>();
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

    void insert(const Key &k, const Value &v) {
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
            // leaf->insert(k, v);
            if (parent)
                parent->insert(sep, newLeaf);
            else {
                // fprintf(stderr, "HERE HERE!!!\n"); fflush(stderr);
                makeRoot(sep, leaf, newLeaf);
            }
            // Unlock and restart
            node->writeUnlock();
            if (parent)
                parent->writeUnlock();
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

    void upsert(const Key &k, const Value &v) {
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
            // leaf->insert(k, v);
            if (parent)
                parent->insert(sep, newLeaf);
            else {
                // fprintf(stderr, "HERE HERE!!!\n"); fflush(stderr);
                makeRoot(sep, leaf, newLeaf);
            }
            // Unlock and restart
            node->writeUnlock();
            if (parent)
                parent->writeUnlock();
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
            leaf->upsert(k, v);
            node->writeUnlock();
            return; // success
        }
    }

    bool lookup(const Key &k, Value &result) {
        // fprintf(stderr, "LOOKING UP ... "); fflush(stderr);
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
        throw "Not Implemented Yet";
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
// template <class Key, class Value>
// void print_node(const NodeBase *node, std::ostream &os,
// 						unsigned int depth, bool recursive) {
// 	for (unsigned int i = 0; i < depth; i++)
// 		os << "  ";

// 	os	<< "node " << node
// 		// << " level " << node->level
// 		<< " slotuse " << node->count
// 		<< std::endl;

// 	if (node->type == PageType::BTreeLeaf) {
// 		auto *leafnode = static_cast<const BTreeLeaf<Key, Value> *>(node);

// 		for (unsigned int i = 0; i < depth; i++)
// 			os << "  ";

// 		for (unsigned int slot = 0; slot < leafnode->slotuse; ++slot) {
// 			print_key(os, leafnode->blindi_seqtree.node.get_key_ptr(slot));
// 		}
// 		os << std::endl;

// 	} else {
// 		auto *innernode = static_cast<const BTreeInner<Key> *>(node);

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
