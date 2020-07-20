#ifndef _SKIP_LIST_SEQ_H_
#define _SKIP_LIST_SEQ_H_

#include <algorithm>
#include <functional>
#include <memory>
#include <cstddef>
#include <cassert>
#include <vector>

#include <iostream>

template <typename _Key, typename _Data>
struct _NodeSeq {
    _Key key;
    _Data data;
    std::vector<_NodeSeq<_Key, _Data>*>  next;

    _NodeSeq() : key(), data() {
    }

    _NodeSeq(const _Key& _key, const _Data &_data) : key(_key), data(_data) {
    }

    // _Node(const _Node&) = delete;
    // _Node& operator=(const _Node&) = delete;
};

template <typename _Key, typename _Data,
          typename _Compare = std::less<_Key> >
class skip_list_seq {

typedef _NodeSeq<_Key, _Data> node_type;

private:
    node_type* _head;
    _Compare _cmp;

    static const int32_t _MAX_LEVEL_NUM = 64; // maximal number of layers
    int32_t _level_num; // current number of layers

    static int32_t get_random_level() {
        for (int32_t lvl = 0; lvl < _MAX_LEVEL_NUM; ++lvl)
            if (rand() % 2)
                return lvl;
        return (rand() % _MAX_LEVEL_NUM);
    }

public:
    skip_list_seq() : _head() {
        _level_num = 1;

        node_type* new_head = new node_type();
        new_head->next.resize(_MAX_LEVEL_NUM);
        new_head->next[0] = nullptr;//new_tail;

        _head = new_head;
    }


    ~skip_list_seq() {
        node_type* node = _head;
        while (node != nullptr) {
            node_type* temp = node->next[0];
            delete node;
            node = temp;
        }
    }

    node_type* find(const _Key& key) const {
        node_type* node = _head;
        int32_t level_num = _level_num;
        for (int32_t i = level_num-1; i >= 0; --i) {
            while (node->next[i] != nullptr && _cmp(node->next[i]->key, key))
                node = node->next[i];
            if (node->next[i] != nullptr && !_cmp(node->next[i]->key, key)
                            && !_cmp(key, node->next[i]->key))
                return node->next[i];
        }
        return nullptr;
    }

    bool add(_Key key, _Data data) {
        node_type* pred = _head;
        node_type* curr;
        int level_num = _level_num;
        std::vector<node_type*> preds(level_num);

        int32_t level_found = -1;
        for (int32_t i = level_num-1; i >= 0; --i) {
            curr = pred->next[i];
            while (curr != nullptr && _cmp(curr->key, key)) {
                pred = curr;
                curr = pred->next[i];
            }
            if (level_found == -1 && curr != nullptr && curr->key == key)
                level_found = i;
            preds[i] = pred;
        }

        if (level_found != -1)
            return false;

        int32_t node_lvl = get_random_level();
        if (node_lvl >= level_num) {
            node_lvl = level_num-1;
            _level_num += 1;
        }

        node_type* new_node = new node_type(key, data);
        new_node->next.resize(node_lvl+1);

        for (size_t i = 0; i < new_node->next.size(); ++i) {
            new_node->next[i] = preds[i]->next[i];
            preds[i]->next[i] = new_node;
        }
        return true;
    }

    bool remove(const _Key& key) {
        node_type* node = _head;
        node_type* head = _head;
        int32_t level_num = _level_num;

        node_type* pred = head;
        node_type* curr;
        std::vector<node_type*> preds(level_num);

        int32_t level_found = -1;
        for (int32_t i = level_num-1; i >= 0; --i) {
            curr = pred->next[i];
            while (curr != nullptr && _cmp(curr->key, key)) {
                pred = curr;
                curr = pred->next[i];
            }
            if (level_found == -1 && curr != nullptr && curr->key == key)
                level_found = i;
            preds[i] = pred;
        }

        if (level_found != -1) {
            return false; // else curr is the victim
        }

        for (size_t i = 0; i <= level_found; ++i) {
            assert(preds[i]->next[i] == curr);
            preds[i]->next[i] = curr->next[i];
        }
        delete curr;
        // lock to decrease the max level number
        // update _level_num;
        int32_t decr = 0;
        for (int32_t i = level_num-2; i >= 0 && head->next[i] == nullptr; --i)
            _level_num--;
        _level_num -= decr;
        //
        return true;
    }
};

#endif // _SKIP_LIST_SEQ_H_
