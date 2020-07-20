#ifndef _SKIP_LIST_SINGLE_LOCK_H_
#define _SKIP_LIST_SINGLE_LOCK_H_

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
struct _NodeSL {

    static const int32_t _MAX_LEVEL_NUM = 64;

    _Key key;
    _Data data;
    std::array<std::atomic<_NodeSL<_Key, _Data>*>, _MAX_LEVEL_NUM> next;

    _NodeSL() : key(), data() {
    }

    _NodeSL(const _Key& _key, const _Data &_data)
    : key(_key), data(_data) {
    }

    _NodeSL(const _NodeSL &) = delete;
    _NodeSL &operator=(const _NodeSL &) = delete;

};


template <typename _Key, typename _Data,
          typename _Compare = std::less<_Key> >
class skip_list_single_lock {

typedef _NodeSL<_Key, _Data> node_type;

private:
    std::atomic<node_type*> _head;
    _Compare _cmp;

    std::atomic<int32_t> _level_num;      // the number of levels the node is on
    std::mutex _global_lock;

    static int32_t get_random_level() {
        for (int32_t lvl = 0; lvl < node_type::_MAX_LEVEL_NUM; ++lvl)
            if (rand() % 2)
                return lvl;
        return (rand() % node_type::_MAX_LEVEL_NUM);
    }

public:
    skip_list_single_lock() {
        node_type* new_head = new node_type();
        _level_num.store(1);
        for (auto &succ : new_head->next)
            succ = nullptr;
        _head = new_head;
    }


    ~skip_list_single_lock() {
        _global_lock.lock();
        node_type* node = _head.load();
        while (node != nullptr) {
            node_type* temp = node->next[0];
            delete node;
            node = temp;
        }
        _global_lock.unlock();
    }

    node_type* find(const _Key& key) const {
        node_type* pred = _head;
        int32_t levels = _level_num.load();
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
        _global_lock.lock();

        std::array<node_type*, node_type::_MAX_LEVEL_NUM> preds;
        node_type* pred = _head;
        int32_t levels = _level_num.load();
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
        }

        if (level_found != -1) {
            _global_lock.unlock();
            return false;
        }

        int32_t node_lvl = get_random_level();
        if (node_lvl >= levels) {
            node_lvl = levels-1;
            _level_num.store(levels+1);
        }

        node_type* new_node = new node_type(key, data);

        for (int32_t i = 0; i < node_lvl+1; ++i)
            new_node->next[i].store(preds[i]->next[i].load());
        for (int32_t i = 0; i < node_lvl+1; ++i)
            preds[i]->next[i].store(new_node);
        _global_lock.unlock();
        return true;
    }

    bool remove(const _Key& key) {
        _global_lock.lock();

        std::array<node_type*, node_type::_MAX_LEVEL_NUM> preds;
        node_type* pred = _head;
        node_type* curr;
        int32_t levels = _level_num.load();
        int32_t level_found = -1;
        for (int32_t i = levels-1; i >= 0; --i) {
            curr = pred->next[i].load();
            while (curr != nullptr && _cmp(curr->key, key)) {
                pred = curr;
                curr = pred->next[i].load();
            }
            if (level_found == -1 && !_cmp(curr->key, key) && !_cmp(key, curr->key))
                level_found = i;
            preds[i] = pred;
        }

        if (level_found != -1) {
            _global_lock.unlock();
            return false; // else curr is the victim
        }

        for (int32_t i = 0; i <= level_found; ++i) {
            assert(preds[i]->next[i].load() == curr);
            preds[i]->next[i].store(curr->next[i].load());
        }
        delete curr;
        // lock to decrease the max level number
        // update _level_num;
        int32_t decr = 0;
        for (int32_t i = _level_num-2; i >= 0 && _head->next[i].load() == nullptr; --i)
            ++decr;
        _level_num.store(levels-decr);
        //
        _global_lock.unlock();
        return true;
    }
};

#endif // _SKIP_LIST_SINGLE_LOCK_H_
