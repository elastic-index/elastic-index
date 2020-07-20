#ifndef _SKIP_LIST_LOCK_FREE_H_
#define _SKIP_LIST_LOCK_FREE_H_

#include <algorithm>
#include <functional>
#include <memory>
#include <cstddef>
#include <cassert>
#include <vector>
#include <mutex>
#include <atomic>

#include <iostream>
#include <limits>

#define EPRINTF(...)    ; //fprintf(stderr, __VA_ARGS__)

// #include "marked_ptr.hpp"

template <typename _Key, typename _Data>
struct _NodeLF {

    static const int32_t _MAX_LEVEL_NUM = 64;

    _Key key;
    _Data data;
    std::array<std::atomic<_NodeLF<_Key, _Data>*>, _MAX_LEVEL_NUM> next;
    std::atomic<int32_t> level_num;      // the number of levels the node is on

    _NodeLF() : key(), data(), level_num(0) {
    }

    _NodeLF(const _Key& _key, const _Data &_data)
    : key(_key), data(_data), level_num(0) {
    }

    _NodeLF(const _NodeLF &) = delete;
    _NodeLF &operator=(const _NodeLF &) = delete;
};


template <typename _Key, typename _Data,
          typename _Compare = std::less<_Key> >
class skip_list_lock_free {

typedef _NodeLF<_Key, _Data> node_type;

private:

    std::atomic<node_type*> _head;
    _Compare _cmp;

    static const int32_t _MAX_LEVEL_NUM = 64; // maximal number of layers
    std::atomic<int32_t> _level_num; // current number of layers

public:

    skip_list_lock_free() {
        node_type* new_head = new node_type();
        new_head->level_num.store(_MAX_LEVEL_NUM);//1);
        for (auto &succ : new_head->next)
            succ.store(nullptr);
        // for (int32_t i = 0; i < new_head->next.size(); ++i)
        //     EPRINTF(">>> head->next[%d] is %p <<<\n", i, new_head->next[i].load());
        _head = new_head;
    }


    ~skip_list_lock_free() {
        // need to delete all that's on level 0
    }

private:

    static int32_t get_random_level() {
        for (int32_t lvl = 0; lvl < _MAX_LEVEL_NUM; ++lvl)
            if (rand() % 2)
                return lvl;
        return (rand() % _MAX_LEVEL_NUM);
    }

    node_type* locate(const _Key& key,
                std::array<node_type*, node_type::_MAX_LEVEL_NUM> &preds,
                std::array<node_type*, node_type::_MAX_LEVEL_NUM> &succs) {
        node_type* pred = nullptr;
        node_type* curr = nullptr;
        node_type* succ = nullptr;
        bool marked;
        // int CNT = 1;
    retry:
        while (true) {
            EPRINTF("* Trying to locate \"%s\" (attempt #%d)\n", key.c_str(), CNT++);
            pred = _head.load();
            int32_t levels = pred->level_num.load();
            // TODO: adjust the toplevel below
            for (int32_t lvl = levels-1; lvl >= 0; --lvl) {
                // EPRINTF("** Currently on level $%d\n", lvl);
                curr = get_pointer(pred->next[lvl]);
                // EPRINTF("*** pred = %p, curr = %p\n", pred, curr);
                while (curr != nullptr) {
                    succ = curr->next[lvl].load(); //
                    marked = get_mark(succ);
                    succ = get_pointer(succ);
                    while (marked) { // unlink marked nodes TODO fix memory leaks
                        node_type* tmp = get_pointer(curr);
                        bool snip = pred->next[lvl].compare_exchange_strong(tmp, get_pointer(succ));
                        if (!snip) // ???
                            goto retry;
                        curr = get_pointer(pred->next[lvl].load());
                        if (curr == nullptr)
                            break;
                        succ = curr->next[lvl].load();
                        marked = get_mark(succ);
                        succ = get_pointer(succ);
                    }
                    if (curr != nullptr && _cmp(curr->key, key)) {
                        pred = curr;
                        curr = succ;
                    } else
                        break;
                } 
                // EPRINTF("** Setting preds[%d] = %p, succs[%d] = %p\n", lvl, pred, lvl, curr);
                preds[lvl] = pred;
                succs[lvl] = curr;
            }
            return (curr != nullptr && curr->key == key)? curr : nullptr;
        }
        assert(false);
    }

public:

    node_type* find(const _Key& key) { // const {
        std::array<node_type*, node_type::_MAX_LEVEL_NUM> preds, succs;
        return locate(key, preds, succs);
    }

    bool contains(const _Key& key) { // const {
        // EPRINTF("-> Starting contains(\"%s\")\n", key.c_str());
        node_type* pred = _head.load();
        node_type* curr = nullptr;
        node_type* succ = nullptr;
        bool marked = false;
        int32_t levels = pred->level_num.load();
        for (int32_t lvl = levels-1; lvl >= 0; --lvl) {
            curr = get_pointer(pred->next[lvl].load());
            while (curr != nullptr) {
                succ = curr->next[lvl].load();
                marked = get_mark(succ);
                succ = get_pointer(succ);
                while (marked) {
                    // curr = get_pointer(pred->next[lvl].load()); // from the H&S book -- but wrong?
                    curr = succ;
                    if (curr == nullptr)
                        break;
                    succ = curr->next[lvl].load();
                    marked = get_mark(succ);
                    succ = get_pointer(succ);
                }
                if (!_cmp(curr->key, key)) 
                    break;
                pred = curr;
                curr = succ; 
            }
        }
        return (curr != nullptr && curr->key == key);
    }

    node_type* get_pointer(node_type* ptr) {
        node_type* unmarked_ptr = reinterpret_cast<node_type*>(
            reinterpret_cast<std::uintptr_t>(ptr) & ~(uintptr_t)1
        );
        // EPRINTF("*** converting %p to %p\n", ptr, unmarked_ptr);
        return unmarked_ptr;
    }

    bool get_mark(node_type* ptr) {
        bool marked = reinterpret_cast<std::uintptr_t>(ptr) & (uintptr_t)1;
        // EPRINTF("*** converting %p to %u\n", ptr, marked);
        return marked;
    }

    node_type* set_mark(node_type* ptr) {
        node_type* marked_ptr = reinterpret_cast<node_type*>(
            reinterpret_cast<std::uintptr_t>(ptr) | (uintptr_t)1
        );
        // EPRINTF("*** converting %p to %p\n", ptr, marked_ptr);
        return marked_ptr;
    }


    bool add(_Key key, _Data data) {
        // TODO: figure out if a special tr
        std::array<node_type*, node_type::_MAX_LEVEL_NUM> preds, succs;
        int32_t node_lvl = get_random_level();
        // EPRINTF("* Generated a random level\n"); int x = 1;
        while (true) {
            // EPRINTF("* Insertion attempt #%d\n", x++);
            node_type* found = locate(key, preds, succs);
            // EPRINTF("* Search successfully finished.\n");
            if (found != nullptr)
                return false; // value is already in there
            // prepare a node
            node_type* new_node = new node_type(key, data);
            new_node->level_num.store(node_lvl+1);
            // add successors at each level
            for (int32_t lvl = 0; lvl <= node_lvl; ++lvl) {
                node_type* succ = get_pointer(succs[lvl]); // unmarked pointer
                new_node->next[lvl].store(succ);
            }
            assert(new_node->next[0] == get_pointer(succs[lvl]));
            node_type* succ = get_pointer(succs[0]);
            // EPRINTF("* pred[0] = %p, pred[0]->next[0] = %p, succs[0] = %p, succ == %p\n", preds[0], preds[0]->next[0].load(), succs[0], succ);
            if (!preds[0]->next[0].compare_exchange_strong(succ, get_pointer(new_node)))
                continue;
            // EPRINTF("* SECOND\n");
            // linking on higher levels
            for (int32_t lvl = 1; lvl <= node_lvl; ++lvl) {
                while (true) {
                    node_type* pred = preds[lvl];
                    node_type* succ = get_pointer(succs[lvl]);
                    if (pred->next[lvl].compare_exchange_strong(succ, get_pointer(new_node)))
                        break;
                    locate(key, preds, succs); // ???
                }
            }
            return true;
        }
    }

    bool remove(const _Key& key) {
        std::array<node_type*, node_type::_MAX_LEVEL_NUM> preds, succs;
        node_type* succ;
        bool marked;
        while (true) {
            node_type* found = locate(key, preds, succs);
            if (found == nullptr)
                return false;
            node_type* victim = succs[0];
            for (int lvl = victim->node_lvl; lvl > 0; --lvl) {
                succ = victim->next[lvl].load();
                marked = succ & 1;
                succ = succ & (~1);
                while (!marked) {
                    victim->next[lvl].store(succ | 1);
                    succ = victim->next[lvl].load();
                    marked = succ & 1;
                    succ = succ & (~1);
                }
            }
            succ = victim->next[0].load();
            marked = succ & (1);
            succ = succ & (~1);
            while (true) {
                bool marked_by_me =
                    victim->next[0].compare_exchange_strong(succ & (~1), succ | 1);
                succ = succs[0]->next[0].load();
                marked = succ & (1);
                succ = succ & (~1);
                if (marked_by_me) {
                    locate(key, preds, succs); // ??? erasing marked nodes?
                    return true;
                } else if (marked)
                return false;
            }
        }
    }
};

#endif // _SKIP_LIST_LOCK_FREE_H_
