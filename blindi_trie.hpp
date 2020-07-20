#ifndef _BLINDI_TRIE_H_
#define _BLINDI_TRIE_H_

// C++ program for BlindI node
#include<iostream>
#include <cassert>
#include <type_traits>
#include "bit_manipulation.hpp"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef _BLINDI_PARM_H_
#define _BLINDI_PARM_H_

enum SEARCH_TYPE {PREDECESSOR_SEARCH = 1, POINT_SEARCH = 0};
enum INSERT_RESULT {INSERT_SUCCESS = 0, INSERT_DUPLICATED_KEY = 1, INSERT_OVERFLOW = 2};
enum REMOVE_RESULT {REMOVE_SUCCESS = 0, REMOVE_NODE_EMPTY = 1, REMOVE_UNDERFLOW = 2, REMOVE_KEY_NOT_FOUND = 3};
#endif // _BLINDI_PARM_H_

// The Key valid_length is constant and up to 255 bits
#define PTR_AREA ( 1 << (sizeof(idx_t)*8 - 1))

// #define FINGERPRINT
// #define KEY_VAR_LEN

// #define ASSERT
//#define DEBUG_BLINDI
//#define DEBUG_FIRST_STAGE

#define BLINDI_MAX(a,b)          ((a) < (b) ? (b) : (a))

using namespace std;

//int first_stage_len = 0;
static int CompareStage (const uint8_t *key1, const uint8_t *key2, bool *eq, bool *big_small, uint16_t key_len_key1, uint16_t key_len_key2);

// A BlindI node
template <typename _Key, int NUM_SLOTS>
class TrieBlindiNode {
    static_assert((NUM_SLOTS % 2) == 0, "NUM_SLOTS is not even");
//    static_assert((NUM_SLOTS <= 128), "NUM_SLOTS > 128");

    typedef typename std::conditional<sizeof(_Key) <= 32,
    uint8_t, uint16_t>::type bitidx_t;

    typedef typename std::conditional<NUM_SLOTS <= 128,
    uint8_t, uint16_t>::type idx_t;

    bitidx_t blindiKeys[NUM_SLOTS - 1]; // An array of BlindIKeys -[ the lsb is the value 0/1  - just when we have vaiable key] [we don't change the position of the blindKeys - just the position of the left/right ptr]
    // The poister are for two places,
    // (1) "< PTR_AREA" to the blindKeys area,
    // (2) "> PTR_AREA" to the pointer area (key_ptr, key_len, key_len, fingerprint)
    idx_t blindiKeys_small_ptr[NUM_SLOTS - 1];
    idx_t blindiKeys_large_ptr[NUM_SLOTS - 1];
    idx_t blindiKeys_root; // the position of the root of the blindi_keys


    uint8_t *key_ptr[NUM_SLOTS]; // An array of pointers
#ifdef KEY_VAR_LEN
    uint16_t key_len[NUM_SLOTS]; // the length of the key
#endif
#ifdef FINGERPRINT
    uint8_t fingerprint[NUM_SLOTS]; // a hash to make sure this is the right key
#endif
    idx_t valid_len;// the current number of blindiKeys in the blindiNode

    /*
    The blindiKeys array are ordered and the left_ptr & right_ptr points to next addr when the value is <= NUM_SLOTS & to the pointers space when the msb= 1, we omit the msb to calc the key_ptr position
    */

public:
    TrieBlindiNode() {
        blindiKeys_root = PTR_AREA;
        valid_len = 0;
    }   // Constructor

    idx_t get_valid_len() const {
        return this->valid_len;
    }

    uint8_t *get_key_ptr(uint16_t pos) const {
        return this->key_ptr[pos];
    }

    uint16_t get_blindiKey(uint16_t pos) const {
        return this->blindiKeys[pos];
    }


    void first_insert(uint8_t *key_ptr, uint16_t key_len, uint8_t fingerprint = 0) {
        this->blindiKeys[0] = 0;
        this->key_ptr[0] = key_ptr;
        blindiKeys_small_ptr[0] = PTR_AREA;
        blindiKeys_large_ptr[0] = PTR_AREA;
#ifdef KEY_VAR_LEN
        this->key_len[0] = key_len;
#endif
#ifdef FINGERPRINT
        this->fingerprint[0] = fingerprint;
#endif
        this->valid_len = 1;
    }

public:
    void print_tree_traverse (uint16_t *tree_traverse, uint16_t *tree_traverse_len) const {
        for (int i= *tree_traverse_len -1; i>=0; i--) {
            printf ("taverse[%d]= %d small %d large %d \n", i, tree_traverse[i], blindiKeys_small_ptr[tree_traverse[i]], blindiKeys_large_ptr[tree_traverse[i]]);
        }
    }

// check smaller_than_node - check if the item is small than the small branch
    void check_smaller_than_node (uint16_t *tree_traverse, uint16_t len, bool *smaller_than_node, uint16_t key_len) const {
        if (len == 0) {
            *smaller_than_node = true;
            return;
        }
        while (len > 0) {
            if (blindiKeys_large_ptr[tree_traverse[len - 1]] == tree_traverse[len]) { // not from the left branch
                *smaller_than_node = false;
                return;
            }
            len = len - 1;
        }
        *smaller_than_node = true;
        return;
    }


    uint16_t bring_previous_item (uint16_t *tree_traverse, uint16_t *tree_traverse_len) const {
        if (*tree_traverse_len == 1) {
            return bring_largest_in_subtree(tree_traverse[*tree_traverse_len - 1], tree_traverse, tree_traverse_len);
        }
        // go up till we arrive to a position that the traverase was from the small side
        while (blindiKeys_large_ptr[tree_traverse[*tree_traverse_len - 2]] != tree_traverse[*tree_traverse_len - 1]) {
            *tree_traverse_len -= 1;
            if (blindiKeys_large_ptr[tree_traverse[*tree_traverse_len - 2]] == blindiKeys_root) {
                break;
            }
        }
        // go down to the small value and from there find the largest
        tree_traverse[*tree_traverse_len - 1] =  blindiKeys_small_ptr[tree_traverse[*tree_traverse_len - 2]];
        return bring_largest_in_subtree(tree_traverse[*tree_traverse_len - 1], tree_traverse, tree_traverse_len);
    }

    void  bring_descending_items(uint16_t *desc_positions) const {
        uint16_t tree_traverse[NUM_SLOTS] = {0};
        uint16_t tree_traverse_len = 1;
        tree_traverse[0] = blindiKeys_root;
        desc_positions[0] = bring_largest_in_subtree(tree_traverse[0], tree_traverse, &tree_traverse_len) - PTR_AREA;
//	print_tree_traverse(tree_traverse, &tree_traverse_len);
        for (int i=1; i < valid_len; i++) {
            desc_positions[i] = bring_previous_item(tree_traverse, &tree_traverse_len)- PTR_AREA;
//		print_tree_traverse(tree_traverse, &tree_traverse_len);
        }
    }


    uint16_t bring_next_item (uint16_t *tree_traverse, uint16_t *tree_traverse_len) const {
        if (*tree_traverse_len == 1) {
            return bring_smallest_in_subtree(tree_traverse[*tree_traverse_len - 1], tree_traverse, tree_traverse_len);
        }
        // go up till we arrive to a position that the traverase was from the large side
        while (blindiKeys_small_ptr[tree_traverse[*tree_traverse_len - 2]] != tree_traverse[*tree_traverse_len - 1]) {
            *tree_traverse_len -= 1;
            if (blindiKeys_small_ptr[tree_traverse[*tree_traverse_len - 2]] == blindiKeys_root) {
                break;
            }
        }
        // go down to the large value  and from there find the smallest
        tree_traverse[*tree_traverse_len - 1] =  blindiKeys_large_ptr[tree_traverse[*tree_traverse_len - 2]];
        return bring_smallest_in_subtree(tree_traverse[*tree_traverse_len - 1], tree_traverse, tree_traverse_len);
    }

    void bring_sorted_items(uint16_t *sorted_positions) const {
        uint16_t tree_traverse[NUM_SLOTS] = {0};
        uint16_t tree_traverse_len = 1;
        tree_traverse[0] = blindiKeys_root;
        sorted_positions[0] = bring_smallest_in_subtree(tree_traverse[0], tree_traverse, &tree_traverse_len) - PTR_AREA;
        for (int i=1; i < valid_len; i++) {
            sorted_positions[i] = bring_next_item(tree_traverse, &tree_traverse_len)- PTR_AREA;
        }
    }



    uint16_t bring_largest_in_subtree (uint16_t pos, uint16_t *tree_traverse, uint16_t *tree_traverse_len) const {
        if (pos >= PTR_AREA) {
            return pos;
        }
        while (blindiKeys_large_ptr[pos] < PTR_AREA) {
            pos = blindiKeys_large_ptr[pos];
            tree_traverse[*tree_traverse_len] = pos;
            *tree_traverse_len +=1;
        }
        pos = blindiKeys_large_ptr[pos];
        tree_traverse[*tree_traverse_len] = pos;
        *tree_traverse_len +=1;
        return pos;
    }

    uint16_t bring_smallest_in_subtree (uint16_t pos ,uint16_t *tree_traverse, uint16_t *tree_traverse_len) const {
        if (pos >= PTR_AREA) {
            return pos;
        }
        while (blindiKeys_small_ptr[pos] < PTR_AREA) {
            pos = blindiKeys_small_ptr[pos];
            tree_traverse[*tree_traverse_len] = pos;
            *tree_traverse_len +=1;
        }
        pos = blindiKeys_small_ptr[pos];
        tree_traverse[*tree_traverse_len] = pos;
        *tree_traverse_len +=1;
        return 	pos;
    }

    uint16_t get_max_pos_node () const {
        if (valid_len == 1) {
            return 0;
        }
        uint16_t pos = blindiKeys_root;
        while (blindiKeys_large_ptr[pos] < PTR_AREA) {
            pos = blindiKeys_large_ptr[pos];
        }
        return blindiKeys_large_ptr[pos] - PTR_AREA;
    }

    uint16_t get_min_pos_node () const {
        if (valid_len == 1) {
            return 0;
        }
        uint16_t pos = blindiKeys_root;
        while (blindiKeys_small_ptr[pos] < PTR_AREA) {
            pos = blindiKeys_small_ptr[pos];
        }
        pos = blindiKeys_small_ptr[pos] - PTR_AREA;
        return pos;
    }

    uint16_t get_min_pos_node (uint16_t *tree_traverse, uint16_t *tree_traverse_len) {
        *tree_traverse_len = 1;
        if (valid_len == 1) {
            tree_traverse[0] = PTR_AREA;
            return 0;
        }
        uint16_t pos = blindiKeys_root;
        tree_traverse[0] = pos;
        while (blindiKeys_small_ptr[pos] < PTR_AREA) {
            pos = blindiKeys_small_ptr[pos];
            *tree_traverse_len += 1;
            tree_traverse[*tree_traverse_len - 1] = pos;
        }
        *tree_traverse_len += 1;
        tree_traverse[*tree_traverse_len - 1] = blindiKeys_small_ptr[pos];
        pos = blindiKeys_small_ptr[pos] - PTR_AREA;
        return pos;
    }

    uint16_t get_mid_pos_node () const {
        if (valid_len == 1) {
            return 0;
        }
        uint16_t tree_traverse[valid_len - 1];
        uint16_t tree_traverse_len = 1;
        tree_traverse[0] = blindiKeys_root;
        bring_smallest_in_subtree(blindiKeys_root, tree_traverse, &tree_traverse_len);
        for (int i = 1; i < (valid_len / 2); i++) {
            bring_next_item(tree_traverse, &tree_traverse_len);
        }
        return tree_traverse[tree_traverse_len - 1] - PTR_AREA;
    }


// return place in the PTR_AREA
    uint16_t bring_item_edge_subtree (bool large_small, bool *smaller_than_node, uint16_t *tree_traverse, uint16_t *tree_traverse_len, uint16_t key_len = 0) const {
        if (large_small) { // bring the largest item in the subtree
            *tree_traverse_len += 1;
            return 	bring_largest_in_subtree(tree_traverse[*tree_traverse_len - 1], tree_traverse, tree_traverse_len);
        } else { // bring previous item (if smaller_than_node bring the smallest in the subtree)
            if (*smaller_than_node) {
                *tree_traverse_len += 1;
                return 	bring_smallest_in_subtree(tree_traverse[*tree_traverse_len - 1], tree_traverse, tree_traverse_len);
            }
            // go up till we arrive from the large value

            while (blindiKeys_large_ptr[tree_traverse[*tree_traverse_len - 1]] != tree_traverse[*tree_traverse_len]) {
                *tree_traverse_len -= 1;
            }
            // go to the small value  and from there find the largest
            tree_traverse[*tree_traverse_len] =  blindiKeys_small_ptr[tree_traverse[*tree_traverse_len - 1]];
            *tree_traverse_len += 1;
            return bring_largest_in_subtree(tree_traverse[*tree_traverse_len - 1], tree_traverse, tree_traverse_len);
        }
    }

#ifdef KEY_VAR_LEN
    uint16_t get_key_len (uint16_t pos) const {
        return this->key_len[pos]; // the length of the key
    }
#endif

    /* The size of the blinfikeys array is valid_lenght -1 while the size of teh value is valid_lengthh
     Example for value valid_length 6 keys
     BlindiKeys (5 elements) 	  0             1              2  	     3         4
     Values  (6 elements)        0          1            2               3            4          5
    */
    // A function to search first stage on this key
    // return the position
    void SearchFirstStage (const uint8_t *key_ptr, uint16_t key_len,  uint16_t *tree_traverse, uint16_t *tree_traverse_len) {
        uint16_t blindiKey_pos = blindiKeys_root;
        tree_traverse[0] = blindiKeys_root;
#ifdef KEY_VAR_LEN
        uint16_t var_len_possible_hit = -1;
#endif
        *tree_traverse_len = 1;
        int i = 0;
        while (blindiKey_pos < PTR_AREA) { // if blindiKey_pos > PTR_AREA -> we reach to the poninter area
            uint16_t idx = blindiKeys[blindiKey_pos];
#ifdef KEY_VAR_LEN
            uint16_t small_ptr =  this->blindiKeys_small_ptr[blindiKey_pos];
            if (small_ptr >= PTR_AREA && (get_key_len(small_ptr - PTR_AREA) == (idx >> 3))) {// the blindiKey is because of length and not branching
                if (key_len <= get_key_len(small_ptr - PTR_AREA)) { // posibility to hit
                    if (var_len_possible_hit == -1) {
                        var_len_possible_hit = *tree_traverse_len;
                    }
                }
                blindiKey_pos = this->blindiKeys_large_ptr[blindiKey_pos];
                i = i+1;
                tree_traverse[i] = blindiKey_pos;
                *tree_traverse_len = i+1;
                continue;
            }
#endif
            if (isNthBitSetInString(key_ptr, idx, key_len)) {// check if the bit is set  - the blindiKey is because of branch
#ifdef KEY_VAR_LEN
                if (var_len_possible_hit != -1 && (key_len * 8 > idx)) { // we are not under the same sub string
//				printf("zeros key_len blindiKey_pos ");
                    var_len_possible_hit = -1;
                }
#endif
                blindiKey_pos = this->blindiKeys_large_ptr[blindiKey_pos];
            } else {
                blindiKey_pos = this->blindiKeys_small_ptr[blindiKey_pos];
            }
            i = i+1;
            tree_traverse[i] = blindiKey_pos;
            *tree_traverse_len = i+1;
        }
#ifdef KEY_VAR_LEN
        if (var_len_possible_hit != -1) { // the same substring ->  change the tree traverse to possible hit
            tree_traverse[var_len_possible_hit] = this->blindiKeys_small_ptr[tree_traverse[var_len_possible_hit -1]];
            *tree_traverse_len = var_len_possible_hit + 1;
        }

#endif
//	    printf("posssible  hit %d \n", var_len_possible_hit);
#ifdef ASSERT
        assert (tree_traverse[*tree_traverse_len - 1] >= PTR_AREA);
#endif
    }


    // bring pointer to the key
    uint8_t *bring_key(uint16_t pos) {
        return  key_ptr[pos]; // bring the key from the DB
    }

#ifdef FINGERPRINT
    // bring pointer to the key
    uint8_t bring_fingerprint(uint16_t pos) {
        return  fingerprint[pos]; // bring the key from the DB
    }
#endif



    // A function to search second stage on this key
    void SearchSecondStage (int diff_pos, bool large_small, uint16_t* tree_traverse, uint16_t *tree_traverse_len, bool len_diff = 0) {
        // The tree_traverse[tree_traverse_len -1] points to PTR_AREA (Except:in KEY_VAR_LEN)
        auto start_traverse_back = *tree_traverse_len - 2;
        *tree_traverse_len -= 1;
        int i;

#ifdef KEY_VAR_LEN
        diff_pos -= len_diff & !large_small; // -1 in case the diff from len & small
#endif
        for (i = start_traverse_back; i>=0;  i--) {
            auto blindiKey = this->blindiKeys[tree_traverse[i]];
//		printf("blindiKey %d diff_pos %d large_small %d len_diff %d \n", blindiKey, diff_pos, large_small, len_diff);
            if (blindiKey <= diff_pos) {
                *tree_traverse_len = i+1;
                return;
            }
        }
        *tree_traverse_len = 0;
    }
    // A function to insert a BlindiKey in position in non-full BlindiNode
    // the position - is the position in the blindiKeys

    void Insert2BlindiNodeInPosition(uint8_t *insert_key, uint16_t key_len, int *diff_pos, bool *large_small, bool *smaller_than_node, uint16_t *tree_traverse, uint16_t *tree_traverse_len, uint8_t fingerprint = 0) {
        // insert all the items in the new plac//e
        this->key_ptr[valid_len] = insert_key;
#ifdef KEY_VAR_LEN
        this->key_len[valid_len] = key_len;
#endif
#ifdef FINGERPRINT
        this->fingerprint[valid_len] = fingerprint;
#endif
        this->blindiKeys[valid_len - 1] = *diff_pos;
// connect the tree
        // connect the node to the tree
        int connect_new_blindiKeys2_node;
        bool update_blindi_root = false;
        uint16_t position;
        if (*tree_traverse_len == 0) {
            update_blindi_root = true;
        } else {
            position = tree_traverse[*tree_traverse_len - 1]; // position in the blindKeys
        }
        if (this->valid_len == 1) {
            update_blindi_root = true;
            this->blindiKeys_small_ptr[0] = PTR_AREA;
            this->blindiKeys_large_ptr[0] = PTR_AREA;
            if (*large_small) {
                this->blindiKeys_large_ptr[0] = PTR_AREA + 1;

            } else {
                this->blindiKeys_small_ptr[0] = PTR_AREA + 1;
            }
            connect_new_blindiKeys2_node = PTR_AREA;
            this->blindiKeys_root = 0;
            this->valid_len = 2;
            return;
        }
        if (!update_blindi_root) {
            if (this->blindiKeys_large_ptr[position] == tree_traverse[*tree_traverse_len]) { // we arrived from the large path
                connect_new_blindiKeys2_node = this->blindiKeys_large_ptr[position];
                this->blindiKeys_large_ptr[position] = valid_len - 1;
            } else {
                connect_new_blindiKeys2_node = this->blindiKeys_small_ptr[position];
                this->blindiKeys_small_ptr[position] = valid_len - 1;
            }
        } else { // need to update the root
            this->blindiKeys_root = valid_len - 1;
            connect_new_blindiKeys2_node =  tree_traverse[0];
        }

        // the new blindiKey node
        if (*large_small) {
            this->blindiKeys_small_ptr[valid_len - 1] = connect_new_blindiKeys2_node;
            this->blindiKeys_large_ptr[valid_len - 1] = PTR_AREA + valid_len;
        } else {
            this->blindiKeys_small_ptr[valid_len - 1] = PTR_AREA + valid_len;
            this->blindiKeys_large_ptr[valid_len - 1] = connect_new_blindiKeys2_node;
        }
        // update the valid_len
        this->valid_len += 1;
    }

    // input tree_traverse, tree_traverse_len -> fill the new_node with the content of the tree_traverse
    void fill_node_array (uint16_t root, uint16_t *small_ptr_new_node, uint16_t *large_ptr_new_node, TrieBlindiNode *old_node, TrieBlindiNode *new_node) {
        uint16_t new_pos_array[NUM_SLOTS - 1] = {0}; // the position of the old blindiKey in the new_node
        uint16_t nu_in_pos[NUM_SLOTS - 1] = {0}; // how many times we updated in position
        uint16_t blindiKeys_cnt = 0;
        uint16_t key_ptr_cnt = 0;
        uint16_t tree_traverse[NUM_SLOTS];
        uint16_t tree_traverse_len = 1;
        uint16_t key_ptr_pos;

        new_node->blindiKeys_root = blindiKeys_cnt;
        new_node->blindiKeys[0] = old_node->blindiKeys[root];
        new_pos_array[root] = 0;
        blindiKeys_cnt += 1;
        tree_traverse[0] = root;
        bool update_key_ptr = 0;

        while (key_ptr_cnt < (NUM_SLOTS / 2)) {
            update_key_ptr = 0;
            if (nu_in_pos[tree_traverse[tree_traverse_len - 1]] == 0) { // first time we face this blindiKey - small path
                nu_in_pos[tree_traverse[tree_traverse_len - 1]] = 1;
                if (small_ptr_new_node[tree_traverse[tree_traverse_len - 1]] >= PTR_AREA) {
                    update_key_ptr = 1;
                    key_ptr_pos = small_ptr_new_node[tree_traverse[tree_traverse_len - 1]] - PTR_AREA;
                    new_node->blindiKeys_small_ptr[new_pos_array[tree_traverse[tree_traverse_len - 1]]] = key_ptr_cnt + PTR_AREA;
                } else {
                    tree_traverse[tree_traverse_len] =  small_ptr_new_node[tree_traverse[tree_traverse_len - 1]];
                    tree_traverse_len += 1;
                    new_node->blindiKeys[blindiKeys_cnt] = old_node->blindiKeys[tree_traverse[tree_traverse_len -1]];
                    new_pos_array[tree_traverse[tree_traverse_len - 1]] = blindiKeys_cnt;
                    new_node->blindiKeys_small_ptr[new_pos_array[tree_traverse[tree_traverse_len - 2]]] = blindiKeys_cnt;
                    blindiKeys_cnt += 1;
                }
            } else if (nu_in_pos[tree_traverse[tree_traverse_len - 1]] == 1) { // second time we face this blindiKey - small path
                nu_in_pos[tree_traverse[tree_traverse_len - 1]] = 2;
                if (large_ptr_new_node[tree_traverse[tree_traverse_len - 1]] >= PTR_AREA) {
                    update_key_ptr = 1;
                    key_ptr_pos = large_ptr_new_node[tree_traverse[tree_traverse_len - 1]] - PTR_AREA;
                    new_node->blindiKeys_large_ptr[new_pos_array[tree_traverse[tree_traverse_len - 1]]] = key_ptr_cnt + PTR_AREA;
                } else {
                    tree_traverse[tree_traverse_len] =  large_ptr_new_node[tree_traverse[tree_traverse_len - 1]];
                    tree_traverse_len += 1;
                    new_node->blindiKeys[blindiKeys_cnt] = old_node->blindiKeys[tree_traverse[tree_traverse_len -1]];
                    new_pos_array[tree_traverse[tree_traverse_len - 1]] = blindiKeys_cnt;
                    new_node->blindiKeys_large_ptr[new_pos_array[tree_traverse[tree_traverse_len - 2]]] = blindiKeys_cnt;
                    blindiKeys_cnt += 1;
                }
            } else if (nu_in_pos[tree_traverse[tree_traverse_len - 1]] == 2) { // second time we face this blindiKey - small path
                tree_traverse[tree_traverse_len] =  large_ptr_new_node[tree_traverse[tree_traverse_len - 1]];
                tree_traverse_len -= 1;
            }
            if (update_key_ptr) {
                // update key_ptr (key_len, key_len, fingerprint) // tree_traverse[tree_traverse_len -2]
                new_node->key_ptr[key_ptr_cnt] = old_node->key_ptr[key_ptr_pos];
#ifdef KEY_VAR_LEN
                new_node->key_len[key_ptr_cnt] = old_node->key_len[key_ptr_pos];
#endif
#ifdef FINGERPRINT
                new_node->fingerprint[key_ptr_cnt] = old_node->fingerprint[key_ptr_pos];
#endif
                key_ptr_cnt += 1;
            }
        }
    }

    // split
    // create two sub trees positions - tmp_node1 - tmp_node2
    //  1. (0 - NUM_SLOTS /2 -1) -> copy it to new_node1
    //  2. (NUM_SLOTS/2 : NUM_SLOTS -1) -> copy it to new_node2

    void SplitBlindiNode(TrieBlindiNode *node_small, TrieBlindiNode *node_large) {
        // Find the the key_traverse of key #NUM_SLOTS /2 (the first key of node 2)
        uint16_t _tree_traverse_node_small[NUM_SLOTS] = {0};
        uint16_t _tree_traverse_node_large[NUM_SLOTS] = {0};
        uint16_t _tree_traverse_len_node_small = 1;
        uint16_t _tree_traverse_len_node_large = 1;
        _tree_traverse_node_small[0] = this->blindiKeys_root;
        _tree_traverse_node_large[0] = this->blindiKeys_root;

        // create two tmp tmp_blindiKeys (tmp_blindiKeys for node_small and tmp_blindiKeys2 for node_large
        uint16_t _blindiKeys_small_ptr_node_small[NUM_SLOTS -1], _blindiKeys_small_ptr_node_large[NUM_SLOTS -1];
        uint16_t _blindiKeys_large_ptr_node_small[NUM_SLOTS -1], _blindiKeys_large_ptr_node_large[NUM_SLOTS -1];
        uint16_t _blindiKeys_root_node_small = this->blindiKeys_root;
        uint16_t _blindiKeys_root_node_large = this->blindiKeys_root;
        for (int i=0; i< NUM_SLOTS - 1; i++) {
            _blindiKeys_large_ptr_node_small[i] = this->blindiKeys_large_ptr[i];
            _blindiKeys_large_ptr_node_large[i] = this->blindiKeys_large_ptr[i];
            _blindiKeys_small_ptr_node_small[i] = this->blindiKeys_small_ptr[i];
            _blindiKeys_small_ptr_node_large[i] = this->blindiKeys_small_ptr[i];
        }

        // find key NUM_SLOTS /2 - 1   - will be the last on node 1
        this->bring_smallest_in_subtree(_tree_traverse_node_small[0], _tree_traverse_node_small, &_tree_traverse_len_node_small);
        for (int i=1; i < NUM_SLOTS / 2; i++) {
            this->bring_next_item(_tree_traverse_node_small, &_tree_traverse_len_node_small);
        }
        // find key NUM_SLOTS /2  - will be the first on node 2
        for (int i=0; i < _tree_traverse_len_node_small; i++) {
            _tree_traverse_node_large[i] = _tree_traverse_node_small[i];
        }
        _tree_traverse_len_node_large = _tree_traverse_len_node_small;
        this->bring_next_item(_tree_traverse_node_large, &_tree_traverse_len_node_large);

        // create the tree_node_small - connect NUM_SLOTS/2 - 1 to the tree and remove all the higher keys (define this node as the largest path of node_small tree)
        bool root_stored = false;
        uint16_t pos_save = _blindiKeys_root_node_small;
        for (int i=0; i < _tree_traverse_len_node_small - 1; i++) {
            if (this->blindiKeys_large_ptr[_tree_traverse_node_small[i]] ==  _tree_traverse_node_small[i+1]) {
                if (!root_stored) {
                    root_stored = true;
                    _blindiKeys_root_node_small = _tree_traverse_node_small[i];
                    pos_save = _tree_traverse_node_small[i];
                } else {
                    _blindiKeys_large_ptr_node_small[pos_save] = _tree_traverse_node_small[i];
                    pos_save = _tree_traverse_node_small[i];
                }
            }

        }
        _blindiKeys_large_ptr_node_small[pos_save] = _tree_traverse_node_small[_tree_traverse_len_node_small - 1];

        // create the tree_node_large - connect NUM_SLOTS/2 - 1 to the tree and remove all the higher keys (define this node as the smallest path of node_large tree)
        root_stored = false;
        pos_save = _blindiKeys_root_node_large;
        for (int i=0; i < _tree_traverse_len_node_large - 1; i++) {
            if (this->blindiKeys_small_ptr[_tree_traverse_node_large[i]] ==  _tree_traverse_node_large[i+1]) {
                if (!root_stored) {
                    root_stored = true;
                    _blindiKeys_root_node_large = _tree_traverse_node_large[i];
                    pos_save = _tree_traverse_node_large[i];
                } else {
                    _blindiKeys_small_ptr_node_large[pos_save] = _tree_traverse_node_large[i];
                    pos_save = _tree_traverse_node_large[i];
                }
            }

        }
        _blindiKeys_small_ptr_node_large[pos_save] = _tree_traverse_node_large[_tree_traverse_len_node_large - 1];

        // fill new_node 1 (copy the values from _node_small_tree)
        fill_node_array(_blindiKeys_root_node_small, _blindiKeys_small_ptr_node_small, _blindiKeys_large_ptr_node_small, this, node_small);
        fill_node_array(_blindiKeys_root_node_large, _blindiKeys_small_ptr_node_large, _blindiKeys_large_ptr_node_large, this, node_large);
        node_small->valid_len = NUM_SLOTS / 2;
        node_large->valid_len = NUM_SLOTS / 2;
    }

    // Remove shift key_ptr and shift BlindiKeys, & valid_len -= 1
    uint16_t RemoveFromBlindiNodeInPosition(uint16_t *tree_traverse, uint16_t *tree_traverse_len) {
        //  save the key_ptr poisition in tree_traverse[tree_traverse_len - 1]
        uint16_t key_ptr_remove_position = tree_traverse[*tree_traverse_len - 1];

        //  remove the blindiKeys in tree_traverse[tree_traverse_len - 2]
        uint16_t blindiKey_remove_position = tree_traverse[*tree_traverse_len - 2];
        if (this->valid_len == 1) {
            this->valid_len = 0;
            return REMOVE_NODE_EMPTY;
        }
        if (this->blindiKeys_root ==  tree_traverse[*tree_traverse_len - 2]) {
            if (this->blindiKeys_small_ptr[tree_traverse[*tree_traverse_len - 2]] == tree_traverse[*tree_traverse_len - 1]) {
                this->blindiKeys_root = this->blindiKeys_large_ptr[tree_traverse[*tree_traverse_len - 2]];
            } else {
                this->blindiKeys_root = this->blindiKeys_small_ptr[tree_traverse[*tree_traverse_len - 2]];
            }
        } else {

            //  connect the blindiKeys in tree_traverse[tree_traverse_len - 3] to the other child of  tree_traverse[tree_traverse_len - 2]
            if (this->blindiKeys_small_ptr[tree_traverse[*tree_traverse_len - 3]] == tree_traverse[*tree_traverse_len - 2]) { // arrive from the small size to the blindkey that remove
                if (this->blindiKeys_small_ptr[tree_traverse[*tree_traverse_len - 2]] == tree_traverse[*tree_traverse_len - 1]) {
                    this->blindiKeys_small_ptr[tree_traverse[*tree_traverse_len - 3]] = this->blindiKeys_large_ptr[tree_traverse[*tree_traverse_len - 2]];
                } else {
                    this->blindiKeys_small_ptr[tree_traverse[*tree_traverse_len - 3]] = this->blindiKeys_small_ptr[tree_traverse[*tree_traverse_len - 2]];
                }

            } else { // arrive from the small size to the blindkey that remove
                if (this->blindiKeys_small_ptr[tree_traverse[*tree_traverse_len - 2]] == tree_traverse[*tree_traverse_len - 1]) {
                    this->blindiKeys_large_ptr[tree_traverse[*tree_traverse_len - 3]] = this->blindiKeys_large_ptr[tree_traverse[*tree_traverse_len - 2]];
                } else {
                    this->blindiKeys_large_ptr[tree_traverse[*tree_traverse_len - 3]] = this->blindiKeys_small_ptr[tree_traverse[*tree_traverse_len - 2]];
                }
            }
        }
        this->valid_len -= 1;

        // move the last key_ptr to th empty place
        if ( key_ptr_remove_position < (this->valid_len + PTR_AREA)) { // if the key_ptr that had been removed is not the last item
            this->key_ptr[key_ptr_remove_position - PTR_AREA] = this->key_ptr[this->valid_len];
#ifdef KEY_VAR_LEN
            this->key_len[key_ptr_remove_position - PTR_AREA] =  this->key_len[this->valid_len] ;
#endif
#ifdef FINGERPRINT
            this->fingerprint[key_ptr_remove_position - PTR_AREA] = this->fingerprint[this->valid_len];
#endif
        }


        // move the last blindiKey to th empty place
        if ( blindiKey_remove_position < (valid_len - 1)) { // if the blindiKey that had been removed is not the last item
            this->blindiKeys[blindiKey_remove_position] = this->blindiKeys[this->valid_len - 1];
            this->blindiKeys_small_ptr[blindiKey_remove_position] = this->blindiKeys_small_ptr[this->valid_len - 1];
            this->blindiKeys_large_ptr[blindiKey_remove_position] = this->blindiKeys_large_ptr[this->valid_len - 1];
        }

        // change the pointer in  the blindiKeys from valid_len to the removed place
        if ( key_ptr_remove_position < (this->valid_len + PTR_AREA)) { // if the key_ptr that had been removed is not the last item
            for (int i=valid_len - 2; i>=0; i--) {
                if (this->blindiKeys_large_ptr[i] == (this->valid_len + PTR_AREA)) {
                    this->blindiKeys_large_ptr[i] = key_ptr_remove_position;
                    break;
                }
                if (this->blindiKeys_small_ptr[i] == (this->valid_len + PTR_AREA)) {
                    this->blindiKeys_small_ptr[i] = key_ptr_remove_position;
                    break;
                }
            }
        }

        // change the pointer in  the blindiKeys from valid_len to the removed place
        if ( blindiKey_remove_position < (valid_len - 1)) { // if the blindiKey that had been removed is not the last item
            for (int i=valid_len - 2; i>=0; i--) {
                if (this->blindiKeys_large_ptr[i] == (this->valid_len - 1)) {
                    this->blindiKeys_large_ptr[i] = blindiKey_remove_position;
                    break;
                }
                if (this->blindiKeys_small_ptr[i] == (this->valid_len - 1)) {
                    this->blindiKeys_small_ptr[i] = blindiKey_remove_position;
                    break;
                }
            }
            if (this->blindiKeys_root == valid_len - 1) {
                this->blindiKeys_root = blindiKey_remove_position;
            }
        }
        if (valid_len == 1) {
            this->blindiKeys_root = PTR_AREA;
        }
        return REMOVE_SUCCESS; // TBD + check the case of vlaid_len == 2
    }

    // The merge is from the large node to small_node (this)
    uint16_t MergeBlindiNodes(TrieBlindiNode *node_large) {
        // copy the key_ptr from large node to small_node (this) -> create two trees pointing to small node
        uint16_t small_node_len = this->get_valid_len();
        uint16_t large_node_len = node_large->get_valid_len();
        for (int i=0; i < large_node_len; i++) {
            this->key_ptr[small_node_len + i] = node_large->key_ptr[i];
#ifdef KEY_VAR_LEN
            this->key_len[small_node_len + i] =  node_large->key_len[i] ;
#endif
#ifdef FINGERPRINT
            this->fingerprint[small_node_len + i] = node_large->fingerprint[i];
#endif
        }

        // copy the blindiKeys from large node to small_node (this) -> create two trees pointing to small node
        for (int i=0; i < large_node_len - 1; i++) {
            this->blindiKeys[small_node_len + i] = node_large->blindiKeys[i];
            this->blindiKeys_large_ptr[small_node_len + i] = node_large->blindiKeys_large_ptr[i] + small_node_len;
            this->blindiKeys_small_ptr[small_node_len + i] = node_large->blindiKeys_small_ptr[i] + small_node_len;
        }

        uint16_t tree_traverse_small_node[NUM_SLOTS] = {0};
        tree_traverse_small_node[0] = this->blindiKeys_root;
        uint16_t tree_traverse_len_small_node = 1;
        uint16_t tree_traverse_large_node[NUM_SLOTS] = {0};
        tree_traverse_large_node[0] = node_large->blindiKeys_root + small_node_len;
        uint16_t tree_traverse_len_large_node = 1;

        uint16_t last_small_node_pos = bring_largest_in_subtree(this->blindiKeys_root, tree_traverse_small_node, &tree_traverse_len_small_node);
        uint16_t first_large_node_pos = bring_smallest_in_subtree(node_large->blindiKeys_root + small_node_len, tree_traverse_large_node, &tree_traverse_len_large_node);
        bool _hit, _large_small;
        uint16_t _diff_pos;

        uint8_t *last_small_key = this->bring_key(last_small_node_pos - PTR_AREA);  // bring the key from the DB
        uint8_t *first_large_key = this->bring_key(first_large_node_pos - PTR_AREA);  // bring the key from the DB

#ifdef KEY_VAR_LEN
        uint16_t last_small_node_key_len = this->get_key_len(last_small_node_pos - PTR_AREA);
        uint16_t first_large_node_key_len = this->get_key_len(first_large_node_pos - PTR_AREA);
        _diff_pos = CompareStage (last_small_key, first_large_key, &_hit, &_large_small, last_small_node_key_len, first_large_node_key_len);
#else
        _diff_pos = CompareStage (last_small_key, first_large_key, &_hit, &_large_small, sizeof(_Key), sizeof(_Key));
#endif


        this->blindiKeys[small_node_len - 1] = _diff_pos;
        this->valid_len = small_node_len + large_node_len;

        // connect the small tree and the large tree
        // calculate the root
        uint16_t i_small = 0, i_large = 0;
        bool end_node_small = small_node_len == 1; // if the node is with one element There is not blindiKeys (end_node_small =1)
        bool end_node_large = large_node_len == 1; // if the node is with one element There is not blindiKeys (end_node_large =1)
        uint16_t cur_blindikey_small = this->blindiKeys[tree_traverse_small_node[i_small]];
        uint16_t cur_blindikey_large = this->blindiKeys[tree_traverse_large_node[i_large]];
        uint16_t next_blindikey_small = cur_blindikey_small, next_blindikey_large = cur_blindikey_large;

        // calc the root
        if ((end_node_small || _diff_pos < cur_blindikey_small) && (end_node_large ||  _diff_pos < cur_blindikey_large)) { // the root is the new blindiKeys
            this->blindiKeys_root = small_node_len - 1;
        } else {
            if (end_node_large || (!end_node_small && (cur_blindikey_small < cur_blindikey_large))) { // the root is the previous small_root
                this->blindiKeys_root = tree_traverse_small_node[0];
            } else { // the root is the previous large_root
                this->blindiKeys_root = tree_traverse_large_node[0];
            }
        }

        // till the new_blindiKey is smaller than the two trees
#ifdef DEBUG_BLINDI
        printf ("small_node\n");
        for (int i = 0; i< large_node_len; i++ ) {
            printf ("blindiKeys[%d] = %d	", i, blindiKeys[i]);
        }
        print_tree_traverse(tree_traverse_small_node, &tree_traverse_len_small_node);
        printf ("large_node\n");
        for (int i = 0; i< large_node_len; i++ ) {
            printf ("blindiKeys[%d] = %d	", i, node_large->blindiKeys[i]);
        }
        print_tree_traverse(tree_traverse_large_node, &tree_traverse_len_large_node);
        printf ("diff_pos = %d\n", _diff_pos);
#endif
        while (!(end_node_small || _diff_pos < cur_blindikey_small) ||  !(end_node_large || _diff_pos < cur_blindikey_large)) {
            if (end_node_large || (!end_node_small && cur_blindikey_small < cur_blindikey_large)  ) { // connect the small tree
                // 3 options or large or _diff_pos or small (stay the same)
                i_small += 1;
                if (tree_traverse_small_node[i_small] >= PTR_AREA) {
                    end_node_small = 1;
                } else {
                    next_blindikey_small = this->blindiKeys[tree_traverse_small_node[i_small]];
                }
                if ((end_node_small || _diff_pos < next_blindikey_small) &&  (end_node_large || _diff_pos < cur_blindikey_large)) { // new_blindiKey is the smallest
                    this->blindiKeys_large_ptr[tree_traverse_small_node[i_small - 1]] = small_node_len - 1;
                    break;
                }
                if (end_node_small || (!end_node_large && (next_blindikey_small > cur_blindikey_large))) { // connect the large part
                    this->blindiKeys_large_ptr[tree_traverse_small_node[i_small - 1]] = tree_traverse_large_node[i_large];

                } // else do nothing the small part is already connected
                cur_blindikey_small = next_blindikey_small;

            } else { // connect the right tree
                // 3 options or small or _diff_pos or large (stay the same)
                i_large += 1;
                if (tree_traverse_large_node[i_large] >= PTR_AREA) {
                    end_node_large = 1;
                } else {
                    next_blindikey_large = this->blindiKeys[tree_traverse_large_node[i_large]];
                }
                if ((end_node_small || _diff_pos < cur_blindikey_small) &&  (end_node_large || _diff_pos < next_blindikey_large)) { // new_blindiKey is the smallest
                    this->blindiKeys_small_ptr[tree_traverse_large_node[i_large - 1]] = small_node_len - 1;
                    break;
                }
                if (end_node_large || (!end_node_small && (next_blindikey_large > cur_blindikey_small))) { // connect the large part
                    this->blindiKeys_small_ptr[tree_traverse_large_node[i_large - 1]] = tree_traverse_small_node[i_small];

                } // else do nothing the large part is already connected
                cur_blindikey_large = next_blindikey_large;
            }
        }

        this->blindiKeys_small_ptr [small_node_len - 1] = tree_traverse_small_node[i_small];
        this->blindiKeys_large_ptr[small_node_len - 1] = tree_traverse_large_node[i_large];
        return 0;
    }

    int CopyNode(TrieBlindiNode *node_large) {
         // copy the key_ptr from large node to small_node (this) -> create two trees pointing to small node
        int large_node_len = node_large->get_valid_len();
	this->blindiKeys_root = node_large->blindiKeys_root;
        for (int i=0; i < large_node_len - 1; i++) {
            this->key_ptr[i] = node_large->key_ptr[i];
            this->blindiKeys[i] = node_large->blindiKeys[i];
            this->blindiKeys_large_ptr[i] = node_large->blindiKeys_large_ptr[i];
            this->blindiKeys_small_ptr[i] = node_large->blindiKeys_small_ptr[i];
#ifdef KEY_VAR_LEN
            this->key_len[i] =  node_large->key_len[i] ;
#endif
#ifdef FINGERPRINT
            this->fingerprint[i] = node_large->fingerprint[i];
#endif
        }

        this->key_ptr[large_node_len -1] = node_large->key_ptr[large_node_len -1];
#ifdef KEY_VAR_LEN
        this->key_len[large_node_len -1] =  node_large->key_len[large_node_len -1] ;
#endif
#ifdef FINGERPRINT
        this->fingerprint[large_node_len -1 ] = node_large->fingerprint[large_node_len -1];
#endif

        // calc new len
        this->valid_len = large_node_len;
	return 0;
    }
};

template<class NODE, typename _Key, int NUM_SLOTS>
class GenericBlindiTrieNode {
public:
    NODE node;

    GenericBlindiTrieNode() : node() { }

    typedef typename std::conditional<NUM_SLOTS <= 128,
    uint8_t, uint16_t>::type idx_t;

    uint16_t get_valid_len() const {
        return node.get_valid_len();
    }

    uint8_t *get_key_ptr(uint16_t pos) const {
        return node.get_key_ptr(pos);
    }

    _Key get_max_key_in_node() const {
        return (*((_Key*) node.get_key_ptr(node.get_max_pos_node())));
    }

    _Key get_min_key_in_node() const {
        return (*((_Key*) node.get_key_ptr(node.get_min_pos_node())));
    }

    _Key get_mid_key_in_node() const {
        return (*((_Key*) node.get_key_ptr(node.get_mid_pos_node())));
    }

    void bring_sorted_items(uint16_t *sorted_positions) {
        node.bring_sorted_items(sorted_positions);
    }


    uint16_t get_next_item (uint16_t *tree_traverse, uint16_t *tree_traverse_len) const {
        return	node.bring_next_item(tree_traverse, tree_traverse_len) - PTR_AREA;
    }

    uint16_t get_previous_item (uint16_t *tree_traverse, uint16_t *tree_traverse_len) const {
        return	node.bring_previous_item(tree_traverse, tree_traverse_len) - PTR_AREA;
    }


    // Predecessor search -> return the position of the largest key not greater than x.
    // get the node, the key and a pointer to hit, return a position in the node. If hit: the was exact as the key in the DB. If miss: the key does not appear in the DB
    // We return the value of diff_pos and large_small on demand (if they are not NULL)
    // Tree  search
    uint16_t SearchBlindiNode(const uint8_t *search_key, uint16_t key_len, bool search_type, bool *hit, bool *smaller_than_node, uint16_t *tree_traverse, uint16_t *tree_traverse_len, uint8_t fingerprint = 0, int *diff_pos = NULL, bool *large_small =  NULL) {
        bool _large_small = false;
        int _diff_pos = 0;
        *smaller_than_node = false;
        node.SearchFirstStage(search_key, key_len, tree_traverse, tree_traverse_len);
//	printf("tree_traverse[*tree_traverse_len - 1] %d , tree_traverse_len %d\n", tree_traverse[*tree_traverse_len - 1], *tree_traverse_len);
        uint16_t first_stage_pos = tree_traverse[*tree_traverse_len - 1];
#ifdef KEY_VAR_LEN
        bool len_diff = 0;
#endif
#ifdef DEBUG_FIRST_STAGE
        first_stage_len = *tree_traverse_len - 1; // thr place tree_traverse[tree_traverse_len -1] is the pos of the key_ptr -> this is why we sub 1
#endif
#ifdef DEBUG_BLINDI
        printf("AFTER first stage\n");
        node.print_tree_traverse(tree_traverse, tree_traverse_len);
        printf("XXXXXXX\n");
#endif

#ifdef FINGERPRINT
        if (search_type == POINT_SEARCH && fingerprint !=  node.bring_fingerprint(first_stage_pos - PTR_AREA)) { // We return a value if it is a Point search or if we had a hit
            *smaller_than_node = false;
            *hit = false;
            return first_stage_pos - PTR_AREA;
        }
#endif

        uint8_t *db_key = node.bring_key(first_stage_pos - PTR_AREA);  // bring the key from the DB

#ifdef KEY_VAR_LEN
        uint16_t node_key_len = node.get_key_len(first_stage_pos - PTR_AREA);
        _diff_pos = CompareStage (search_key, db_key, hit, &_large_small, key_len, node_key_len);
//	printf ("hit %d eq %d\n", *hit, _large_small);
        if (key_len !=  node_key_len && *hit) {
            *hit = false;
            _large_small = (node_key_len < key_len);
            if (_large_small) {

                _diff_pos = node_key_len * 8;
            } else {
                _diff_pos = key_len * 8;
            }
            len_diff = 1;
        }
//	printf ("len_diff %d hit %d eq %d\n", len_diff, *hit, _large_small);
#else
        _diff_pos = CompareStage (search_key, db_key, hit, &_large_small, sizeof(_Key), sizeof(_Key));
//	printf("CompareStage %d\n", _diff_pos);
#endif
        if (search_type == POINT_SEARCH || *hit) { // We return a value if it is a Point search or if we had a hit
            *smaller_than_node = false;
            return first_stage_pos - PTR_AREA;
        }
//	node.print_tree_traverse(tree_traverse, tree_traverse_len);
#ifdef KEY_VAR_LEN
        node.SearchSecondStage(_diff_pos, _large_small, tree_traverse, tree_traverse_len, len_diff);
#else
        node.SearchSecondStage(_diff_pos, _large_small, tree_traverse, tree_traverse_len);
#endif

        if ((diff_pos != NULL) && (large_small != NULL)) { // this search is for insert - don't need to bring the real position
            *diff_pos = _diff_pos;
            *large_small = _large_small;
            return 0;
        }
        if (_large_small == false) { // might be smaller than node if on the small_branch in the tree
            node.check_smaller_than_node(tree_traverse, *tree_traverse_len, smaller_than_node, key_len);
        }

#ifdef DEBUG_BLINDI
        printf ("YYY large_small %d tree_traverse_len %d, tree_traverse[0] %d  tree_traverse[1] %d \n", _large_small, *tree_traverse_len, tree_traverse[0], tree_traverse[1]);
#endif
        return node.bring_item_edge_subtree(_large_small, smaller_than_node, tree_traverse, tree_traverse_len, key_len) - PTR_AREA;
    }


    // SearchBlindiNode
    // Insert2BlindiNodeInPosition
    // return success or duplicated key or overflow
    // INSERT TREE
    uint16_t Insert2BlindiNodeWithKey(uint8_t *insert_key, uint16_t key_len = sizeof(_Key), uint8_t fingerprint = 0) {
        bool hit = 0;
        int diff_pos = 0;
        bool large_small = 0;
        uint16_t tree_traverse[NUM_SLOTS] = {0};
        uint16_t tree_traverse_len;
        bool smaller_than_node = false;


        if (node.get_valid_len() == NUM_SLOTS) {
            return INSERT_OVERFLOW;
        }

        if (node.get_valid_len() != 0) { // if the node is not empty
            SearchBlindiNode(insert_key, key_len, PREDECESSOR_SEARCH, &hit, &smaller_than_node, &tree_traverse[0], &tree_traverse_len, fingerprint, &diff_pos, &large_small);
        } else {
            node.first_insert(insert_key, key_len, fingerprint);
            return INSERT_SUCCESS;
        }

        if (hit) {  // it was exeact we have duplicated key
            printf("Error: Duplicated key ???\n");
            return INSERT_DUPLICATED_KEY;
        }
        node.Insert2BlindiNodeInPosition(insert_key, key_len, &diff_pos, &large_small, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        return INSERT_SUCCESS;
    }


    // UPSERT either inserts a new key into the blindi node, or overwrites the key pointer
    // return success or duplicated key or overflow
    // UPSERT TREE
    uint16_t Upsert2BlindiNodeWithKey(uint8_t *insert_key, uint16_t key_len = sizeof(_Key), uint8_t fingerprint = 0) {
        bool hit = 0;
        int diff_pos = 0;
        bool large_small = 0;
        uint16_t tree_traverse[NUM_SLOTS] = {0};
        uint16_t tree_traverse_len;
        bool smaller_than_node = false;

        if (node.get_valid_len() == 0) { // if the node is not empty
            node.first_insert(insert_key, key_len, fingerprint);
            return INSERT_SUCCESS;
        }

        SearchBlindiNode(insert_key, key_len, PREDECESSOR_SEARCH, &hit, &smaller_than_node, &tree_traverse[0], &tree_traverse_len, fingerprint, &diff_pos, &large_small);

        // fprintf(stderr, "Upsert: search found %d, traversal history is ", pos);
        // for (int ii = 0; ii < tree_traverse_len; ++ii)
        //     fprintf(stderr, "%d ", tree_traverse[ii]);
        // fprintf(stderr, "\n");

        if (node.get_valid_len() == NUM_SLOTS) {
            return INSERT_OVERFLOW;
        }

        node.Insert2BlindiNodeInPosition(insert_key, key_len, &diff_pos, &large_small, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        return (hit)? INSERT_DUPLICATED_KEY : INSERT_SUCCESS;
    }

    void SplitBlindiNode(GenericBlindiTrieNode *node_small, GenericBlindiTrieNode *node_large) {
        node.SplitBlindiNode(&node_small->node, &node_large->node);
    }


    uint16_t MergeBlindiNodes(GenericBlindiTrieNode *large_node) {
	int small_node_len = this->get_valid_len();
        int large_node_len = large_node->get_valid_len();
        if (node.get_valid_len() == 0) {
		this->node.CopyNode(&large_node->node);
		return 0;
	}	 
	if (large_node->node.get_valid_len() == 0){
		return 0;
	} 
        return this->node.MergeBlindiNodes(&large_node->node);
    }

    uint16_t RemoveFromBlindiNodeWithKey(uint8_t *remove_key, uint16_t key_len = sizeof(_Key), uint8_t fingerprint = 0) {
        bool hit = 0;
        uint16_t tree_traverse[NUM_SLOTS] = {0};
        uint16_t tree_traverse_len;
        bool smaller_than_node;


        SearchBlindiNode(remove_key, key_len, POINT_SEARCH, &hit, &smaller_than_node, &tree_traverse[0], &tree_traverse_len, fingerprint);
        if (hit == 0) {
            return REMOVE_KEY_NOT_FOUND;
        }
        uint16_t remove_result =  node.RemoveFromBlindiNodeInPosition(&tree_traverse[0], &tree_traverse_len);
        uint16_t valid_len = node.get_valid_len();
        if ((valid_len < NUM_SLOTS / 2)) {
            if (remove_result == REMOVE_SUCCESS) {
                return REMOVE_UNDERFLOW;
            }
        }
        return remove_result;
    }

};
// compare the bring key with search key- find the first bit fdiifreance between searched key and the key we bring from DB.
/*
#ifndef _BLINDI_COMPARE_H_
#define _BLINDI_COMPARE_H_
static int CompareStage (const uint8_t *key1, const uint8_t *key2, bool *eq, bool *big_small, uint16_t key_len_key1, uint16_t key_len_key2)
{
    *eq = false;
#ifdef KEY_VAR_LEN
    uint16_t key_len_byte = min(key_len_key1, key_len_key2);
#else
    uint16_t key_len_byte = key_len_key1;
#endif
    for (int i = 1; i <= key_len_byte ; i++) {
        int mask_byte =  *(key1 + key_len_key1 - i) ^ *(key2 + key_len_key2 - i);
        if (mask_byte != 0) {
            if (*(key1 + key_len_key1 - i) > *(key2 + key_len_key2 - i)) {
                *big_small = true;
            } else {
                *big_small = false;
            }
            return 8*(i - 1) + firstMsbLookUpTable(mask_byte);
        }
    }
    *eq = true;
    return 0;
}
#endif // _BLINDI_COMPARE_H_
*/
#endif // _BLINDI_TRIE_H_
