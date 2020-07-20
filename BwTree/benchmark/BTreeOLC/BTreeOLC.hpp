#pragma once

#include <atomic>
#include <cassert>
#include <cstring>
#include <immintrin.h>
#include <sched.h>

namespace btreeolc {

enum class PageType : uint8_t { BTreeInner = 1, BTreeLeaf = 2 };

static const uint64_t pageSize = 1024; // 4*1024;

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
    uint16_t count;

	virtual void print_node(std::ostream &os, unsigned int depth, bool recursive, const std::string &prefix) = 0;
};

struct BTreeLeafBase : public NodeBase {
    static const PageType typeMarker = PageType::BTreeLeaf;
};

template <typename Key, typename Payload, uint64_t _maxLeafExtries>
struct BTreeLeaf : public BTreeLeafBase {
    struct Entry {
        Key k;
        Payload p;
    };

    //  static const uint64_t
    //  maxEntries=(pageSize-sizeof(NodeBase))/(sizeof(Key)+sizeof(Payload));
    static const uint64_t maxEntries = _maxLeafExtries;

    Key keys[maxEntries];
    Payload payloads[maxEntries];

    BTreeLeaf() {
        count = 0;
        type = typeMarker;
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
			<< " uses " << count << " slots" << std::endl;

		for (unsigned int i = 0; i < depth; i++)
			os << "  ";

		for (unsigned int slot = 0; slot < count; ++slot) {

			print_key(os, (uint8_t*)&keys[slot]);
		}
		os << std::endl;
	}

    bool isFull() { return count == maxEntries; };

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

    void insert(Key k, Payload p) {
        if (count) {
            unsigned pos = lowerBound(k);
            if ((pos < count) && (keys[pos] == k)) {
                // Upsert
                payloads[pos] = p;
                return;
            }
            memmove(keys + pos + 1, keys + pos, sizeof(Key) * (count - pos));
            memmove(payloads + pos + 1, payloads + pos,
                    sizeof(Payload) * (count - pos));
            keys[pos] = k;
            payloads[pos] = p;
        } else {
            keys[0] = k;
            payloads[0] = p;
        }
        count++;
    }

    BTreeLeaf *split(Key &sep) {
        BTreeLeaf *newLeaf = new BTreeLeaf();
        newLeaf->count = count - (count / 2);
        count = count - newLeaf->count;
        memcpy(newLeaf->keys, keys + count, sizeof(Key) * newLeaf->count);
        memcpy(newLeaf->payloads, payloads + count,
               sizeof(Payload) * newLeaf->count);
        sep = keys[count - 1];
        return newLeaf;
    }
};

struct BTreeInnerBase : public NodeBase {
    static const PageType typeMarker = PageType::BTreeInner;
};

template <typename Key, uint64_t _maxInnerEntries>
struct BTreeInner : public BTreeInnerBase {
    static const uint64_t maxEntries = _maxInnerEntries;
    //  static const uint64_t
    //  maxEntries=(pageSize-sizeof(NodeBase))/(sizeof(Key)+sizeof(NodeBase*));
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

    void insert(Key k, NodeBase *child) {
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

template <class Key, class Value, int _InnerSlots, int _LeafSlots,
          typename _Compare = std::less<Key>>
struct BTree {
    std::atomic<NodeBase *> root;

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

    void insert(Key k, Value v) {
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
                else
                    makeRoot(sep, inner, newInner);
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
        if (leaf->count == leaf->maxEntries) {
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

    bool lookup(Key k, Value &result) {
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
        unsigned pos = leaf->lowerBound(k);
        bool success = false;
        if ((pos < leaf->count) && (leaf->keys[pos] == k)) {
            success = true;
            result = leaf->payloads[pos];
        }
        if (parent) {
            parent->readUnlockOrRestart(versionParent, needRestart);
            if (needRestart)
                goto restart;
        }
        node->readUnlockOrRestart(versionNode, needRestart);
        if (needRestart)
            goto restart;

        return success;
    }

    bool upsert(Key k, Value v) {
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
        unsigned pos = leaf->lowerBound(k);
        bool success = false;
        if ((pos < leaf->count) && (leaf->keys[pos] == k)) {
            success = true;
            leaf->payloads[pos] = v;
        }
        if (parent) {
            parent->readUnlockOrRestart(versionParent, needRestart);
            if (needRestart)
                goto restart;
        }
        node->readUnlockOrRestart(versionNode, needRestart);
        if (needRestart)
            goto restart;

        return success;
    }

    uint64_t scan(const Key &k, int range, Value *output) {
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
        unsigned pos = leaf->lowerBound(k);
        int count = 0;
        for (unsigned i = pos; i < leaf->count; i++) {
            if (count == range)
                break;
            output[count++] = leaf->payloads[i];
        }

        if (parent) {
            parent->readUnlockOrRestart(versionParent, needRestart);
            if (needRestart)
                goto restart;
        }
        node->readUnlockOrRestart(versionNode, needRestart);
        if (needRestart)
            goto restart;

        return count;
    }
};

} // namespace btreeolc
