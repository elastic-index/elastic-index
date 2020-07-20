#ifndef _SKIP_LIST_LAZY_H_
#define _SKIP_LIST_LAZY_H_

#include <algorithm>
#include <functional>
#include <memory>
#include <cstddef>
#include <cassert>
#include <array>
#include <mutex>
#include <atomic>

#include <iostream>
#include <limits>

template <typename _Key, typename _Data>
struct _NodeLazy {

    static const int32_t _MAX_LEVEL_NUM = 64;

    _Key key;
    _Data data;
    std::array<std::atomic<_NodeLazy<_Key, _Data>*>, _MAX_LEVEL_NUM> next;
    std::atomic<int32_t> level_num;      // the number of levels the node is on
    bool    marked;         // marked for removal
    bool    fully_linked;   // "logically in" the skip list
    std::recursive_mutex  lock;

    _NodeLazy() : key(), data(), level_num(0), marked(false), fully_linked(false) {
    }

    _NodeLazy(const _Key& _key, const _Data &_data)
    : key(_key), data(_data), level_num(0), marked(false), fully_linked(false) {
    }

    _NodeLazy(const _NodeLazy &) = delete;
    _NodeLazy &operator=(const _NodeLazy &) = delete;

};


template <typename _Key, typename _Data,
          typename _Compare = std::less<_Key> >
class skip_list_lazy {

typedef _NodeLazy<_Key, _Data> node_type;

private:
    node_type* _head;
    _Compare _cmp;

    static int32_t get_random_level() {
        for (int32_t lvl = 0; lvl < node_type::_MAX_LEVEL_NUM; ++lvl)
            if (rand() % 2)
                return lvl;
        return (rand() % node_type::_MAX_LEVEL_NUM);
    }

    skip_list_lazy(const skip_list_lazy&) = delete;
    skip_list_lazy& operator=(const skip_list_lazy&) = delete;
    skip_list_lazy& operator=(skip_list_lazy &&) = delete;

public:
    skip_list_lazy() {
        node_type* new_head = new node_type();
        new_head->level_num.store(1);
        for (auto &succ : new_head->next)
            succ = nullptr;
        _head = new_head;
    }


    ~skip_list_lazy() {
        // need to delete all that's on level 0
    }

    node_type* find(const _Key& key) const {
        node_type* pred = _head;
        int32_t levels = pred->level_num.load();
        for (int32_t i = levels-1; i >= 0; --i) {
            node_type* curr = pred->next[i].load();
            while (curr != nullptr && _cmp(curr->key, key)) {
                pred = curr;
                curr = pred->next[i].load();
            }
            if (curr != nullptr && !_cmp(curr->key, key) && !_cmp(key, curr->key))
                return curr;
        }
        return nullptr;
    }

    bool add(_Key key, _Data data) {
        std::array<node_type*, node_type::_MAX_LEVEL_NUM> preds, succs;
        while (true) {
            node_type* pred = _head;
            int32_t levels = _head->level_num.load();
            int32_t level_found = -1;
            for (int32_t i = levels-1; i >= 0; --i) {
                node_type* curr = pred->next[i].load();
                while (curr != nullptr && _cmp(curr->key, key)) {
                    pred = curr;
                    curr = pred->next[i].load();
                }
                if (level_found == -1 && curr != nullptr && !_cmp(curr->key, key) && !_cmp(key, curr->key))
                    level_found = i;
                preds[i] = pred;
                succs[i] = curr;
            }

            if (level_found != -1) {
                node_type* node = succs[level_found];
                if (!node->marked) {
                    while (!node->fully_linked) {}
                    return false;
                }
                continue;
            }

            int32_t gen_node_lvl = get_random_level();
            int32_t node_lvl;
            do {
                levels = _head->level_num.load();
                if (gen_node_lvl >= levels)
                    node_lvl = levels-1;
                else {
                    node_lvl = gen_node_lvl;
                    break;
                }
            } while (!_head->level_num.compare_exchange_strong(levels, levels+1));

            int32_t highest_locked = -1;
            bool valid = true;
            for (int32_t lvl = 0; valid && (lvl <= node_lvl); ++lvl) {
                preds[lvl]->lock.lock();
                highest_locked = lvl;
                node_type* old_succ = preds[lvl]->next[lvl].load();
                valid = !preds[lvl]->marked
                    && (succs[lvl] == nullptr || !(succs[lvl]->marked))
                    && (old_succ == succs[lvl]);
            }
            if (!valid) {
                for (int32_t lvl = 0; lvl <= highest_locked; ++lvl)
                    preds[lvl]->lock.unlock();
                continue;
            }

            node_type* new_node = new node_type(key, data);
            new_node->level_num.store(node_lvl+1);

            for (int32_t i = 0; i <= node_lvl; ++i) {
                new_node->next[i].store(succs[i]);
            }
            for (int32_t i = 0; i <= node_lvl; ++i) {
                preds[i]->next[i].store(new_node);
            }
            new_node->fully_linked = true;
            for (int32_t lvl = 0; lvl <= highest_locked; ++lvl) {
                preds[lvl]->lock.unlock();
            }
            return true;
        }
    }

    bool remove(const _Key& key) {
        std::array<std::atomic<node_type*>, node_type::_MAX_LEVEL_NUM> preds, succs;
        node_type* victim;
        bool is_marked = false;
        int32_t toplevel = -1;
        while (true) {
            int32_t levels = _head->_level_num.load();
            node_type* pred = _head;
            node_type* curr;

            int32_t level_found = -1;
            for (int32_t i = levels-1; i >= 0; --i) {
                curr = pred->next[i].load();
                while (curr != nullptr && _cmp(curr->key, key))
                    pred = curr;
                if (level_found == -1 && curr != nullptr && !_cmp(curr->key, key) && !_cmp(key, curr->key))
                    level_found = i;
                preds[i] = pred;
                succs[i] = curr;
            }

            if (level_found != -1)
                victim = succs[level_found]; // = curr;
            if (!is_marked &&
                  (level_found == -1 ||
                  (!victim->fully_linked
                  || victim->level_num.load() != level_found+1
                  || victim->marked)))
                return false;
            if (!is_marked) {
                victim->lock.lock();
                toplevel = victim->level_num.load();
                if (victim->marked) {
                    victim->lock.unlock();
                    return false;
                }
                victim->marked = true;
                is_marked = true;
            }

            int32_t highest_locked = -1;
            bool valid = true;
            for (int32_t lvl = 0; valid && (lvl < toplevel); ++lvl) {
                preds[lvl]->lock.lock();
                highest_locked = lvl;
                valid = !preds[lvl]->marked
                    && (preds[lvl]->next[lvl].load() == victim);
            }
            if (!valid) {
                for (int32_t lvl = 0; lvl <= highest_locked; ++lvl)
                    preds[lvl]->lock.unlock();
                continue;
            }
            for (int32_t lvl = toplevel-1; lvl >= 0; --lvl)
                preds[lvl]->next[lvl].store( victim->next[lvl].load() );
            delete victim;
            victim->lock.unlock();
            for (int32_t lvl = 0; lvl <= highest_locked; ++lvl)
                preds[lvl]->lock.unlock();
            // TODO: shrink the levels here
            return true;
        }
    }
};

#endif // _SKIP_LIST_LAZY_H_
