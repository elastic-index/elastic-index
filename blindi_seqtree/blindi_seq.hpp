// C++ program for BlindI node
#include<iostream>
#include <bit_manipulation.hpp>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>



enum SEARCH_TYPE {PREDECESSOR_SEARCH = 1, POINT_SEARCH = 0};
enum INSERT_RESULT {INSERT_SUCCESS = 0, INSERT_DUPLICATED_KEY = 1, INSERT_OVERFLOW = 2};
enum REMOVE_RESULT {REMOVE_SUCCESS = 0, REMOVE_NODE_EMPTY = 1, REMOVE_UNDERFLOW = 2, REMOVE_KEY_NOT_FOUND = 3};

#define KEY_SIZE_BYTE 16
#define KEY_SIZE_IN_BITS KEY_SIZE_BYTE * 8
#define MAX_NODE_SIZE 32
// The Key length is contsant and up to 255 bits

using namespace std;

int CompareStage (const uint8_t *key1, const uint8_t *key2, bool *eq, bool *big_small);

// A BlindI node
class LinearBlindiNode {
    uint8_t *blindiKeys; // An array of BlindIKeys
    uint8_t **data_ptr; // An array of pointers
    uint8_t len;// the current number of blindiKeys in the blindiNode

    /* The size of the blinfikeys array is lenght -1 while the size of teh value is lengthh
     Example for value length 6 keys
     BlindiKeys (5 elements) 	  0             1              2  	     3         4
     Values  (6 elements)        0          1            2               3            4          5
    */

public:
    // A function to search first stage on this key
    // return the position
    int SearchFirstStage (const uint8_t *key_ptr) {
        int first_stage_position = 0; // the position of the potential value
        int high_miss = 0xFFFFFFF;; // the high subtree that was a miss

        for (int i=0; i< (this->len - 1); i++) {
            int idx = this->blindiKeys[i];
            if (isNthBitSetInString(key_ptr, idx, KEY_SIZE_BYTE)) {// check if the bit is set
                if (idx <= high_miss) { // this position is a potintial one
                    high_miss = 0xFFFFFFF;
                    first_stage_position = i+1;
                }  //Else the subtree is still don’t relevant
            } else { // this position is not relevent -to be relevent need to be higher than idx and isNthBitSetInString = 1
                high_miss = min(high_miss, idx);
            }
        }
        return first_stage_position;
    }


    uint8_t *bring_key(int pos) {
        return  data_ptr[pos]; // bring the key from the DB
    }


    // A function to search second stage on this key
    int SearchSecondStage (int second_stage_pos, int diff_pos, bool right_left, bool *smaller_than_node) {

        if (right_left) { // going right
            if (this->len == 1) // we have just one item in the node we return this item
                return 0;
            for (int i=second_stage_pos; i<= this->len -2 ;  i++) {
                if (this->blindiKeys[i] < diff_pos) {
                    return i;
                }
            }
            return this->len - 1;
        }

        // Going left
        if (second_stage_pos == 0) { // The key is smaller than the smallest element in the node - this search is smaller than the entire node, mark as miss
            *smaller_than_node = true;
            return 0;
        }
        second_stage_pos -= 1;
        for (int i=second_stage_pos; i>=0; i--) {
            if (this->blindiKeys[i] < diff_pos) {
                return i;
            }
        }
        *smaller_than_node = true;	 // The key is smaller than the smallest element in the node
        return 0;
    }


public:

    LinearBlindiNode(int nodeSize) {
        blindiKeys = new uint8_t[2*nodeSize-1];
        data_ptr = new uint8_t*[nodeSize];
        len = 0;
    }   // Constructor

    ~LinearBlindiNode() {
        delete [] blindiKeys;
        delete [] data_ptr;
    }   // destructor


    uint8_t get_len() {
        return this->len;
    }

    uint8_t *get_data_ptr(int pos) {
        return this->data_ptr[pos];
    }

    void first_insert(uint8_t *data_ptr) {
        this->data_ptr[0] = data_ptr;
        this->len += 1;
    }

    // A function to insert a BlindiKey in position in non-full BlindiNode


    // shift blindikeys (no need: position == last, first element)
    // shift values (no need: position == last, first element)
    // calc successor key (bring the successor key,  calculate the diff for the successor key, update the successor key) (no need: position == last, first element)
    // insert new data ptr


    // We have two modes in upadte the blind_key:
    // 1.  the first without the results of the search which means that we need to caculate the predeccessor and the successor in the blind_key see Example 1, in the function right_left = NULL &  diff_pos = NULL
    // bring the predecessor key, calculate the BlindI key of the inserted key   (no need: position == 0)
    // len++
    // Example1: before:   blindiKeys   0 1 2 3         insert to position 2          After   0 1   pre     succ         3(shift)
    // 		      data_ptr    0 1 2 3 4 						0 1  2     new      3(shift)   4(shift)

    // 2. with right_left & diff_pos :
    // 2A. Example 2A  right_left = 1
    // Example2a: before:   blindiKeys   0 1 2 3         insert_position 2, right_left =1           After   0 1  diff_pos  2       3(shift)
    // 		        data_ptr    0 1 2 3 4 					   	               0 1  2     new      3(shift)   4(shift)
    // 2BA. Example 2B  right_left = 1
    // Example2a: before:   blindiKeys   0 1 2 3         insert_position 2, right_left =0           After   0 1   2     diff_pos  3(shift)
    // 		        data_ptr    0 1 2 3 4 					   	               0 1  2     new      3(shift)   4(shift)

    void insert2BlindiNodeInPosition(int position, uint8_t *insert_key, uint8_t *data_ptr, bool smaller_than_node = 0, int *diff_pos = NULL, bool *right_left =  NULL) {
        bool hit = 0;
        bool eq = 0;

        if (smaller_than_node | (position != this->len - 1 & (this->len != 0))) { // shift sucesssor key (If we don't insert to the last item)
            //shift the array data_ptr, blindiKeys
            // - shift  Data_ptr from position + 1 till len -1
            //  shift in blindiKeys from posion +1 till len -2
            uint8_t *temp_data = this->data_ptr[len -1];
            for (int i = len - 2; i > position; i--) {
                this->blindiKeys[i+1] = this->blindiKeys[i];
                this->data_ptr[i+1] = this->data_ptr[i];
            }
            this->data_ptr[len] = temp_data;
            if (smaller_than_node) { // if smaller than the first element, need to shift also position 0
                this->data_ptr[1] = this->data_ptr[0];
                if (this->len > 1) { // if we have just one item we don't have blindiKey
                    this->blindiKeys[1] = this->blindiKeys[0];
                }
            }
            if (diff_pos == NULL | right_left == NULL) { // we need to bring the key and calculate the diff
                uint8_t *successor_key = this->bring_key(position + 2 - smaller_than_node);  // bring the key from the DB
                this->blindiKeys[position + 1 - smaller_than_node] = (uint8_t)CompareStage(successor_key ,insert_key, &hit, &eq);
            } else {
                if (*right_left) { // like Example 2A
                    this->blindiKeys[position + 1 - smaller_than_node] = this->blindiKeys[position];
                } else { // Example 2B
                    this->blindiKeys[position + 1 - smaller_than_node] = *diff_pos;
                }
            }
        }



        // calc and insert the new predecesor blindKey (If we don't smaller_than_node)
        if (!smaller_than_node & (this->len != 0)) { // calc the blindIkey predecessor key
            if (diff_pos == NULL | right_left == NULL) { // we need to bring the key and calculate the diff
                uint8_t *predecessor_key = this->bring_key(position);  // bring the key from the DB
                this->blindiKeys[position] = (uint8_t)CompareStage(insert_key, predecessor_key, &hit, &eq); // calc blindikey
            } else if (*right_left) { // like Example 2A
                this->blindiKeys[position] = *diff_pos;
            }
        }

        // insert the pointer to the data_ptr
        this->data_ptr[position + 1 - smaller_than_node] = data_ptr;

        // update the len
        this->len += 1;
    }

    // split
    int splitBlindiNode(LinearBlindiNode *new_node) {
        for (int i=0; i<MAX_NODE_SIZE -1; i++) {
//			printf("Before:  BlindiKeys[%d] = %d\n",i , this->blindiKeys[i]);
        }

        // Update the new node and the orignal_node
        int half_node_size = MAX_NODE_SIZE /2;
        for (int i=0; i <= half_node_size -2; i++) {
            new_node->data_ptr[i] = this->data_ptr[half_node_size + i];
            this->data_ptr[half_node_size + i] = 0;
            new_node->blindiKeys[i] = this->blindiKeys[half_node_size + i];
            this->blindiKeys[half_node_size + i] = 0;
        }

        new_node->data_ptr[half_node_size - 1] = this->data_ptr[MAX_NODE_SIZE - 1];
        this->data_ptr[MAX_NODE_SIZE - 1] = 0;
        this->blindiKeys[half_node_size - 1] = 0;
        new_node->len = half_node_size;
        this->len = half_node_size;


        for (int i=0; i<half_node_size -1; i++) {
//			printf("After: this BlindiKeys[%d] = %d\n",i , this->blindiKeys[i]);
        }

        for (int i=0; i<half_node_size -1; i++) {
//			printf("After: new_node BlindiKeys[%d] = %d\n",i , new_node->blindiKeys[i]);
        }

        return half_node_size;
    }



    // Remove shit data_ptr and shift BlindiKeys, & len -= 1
    int RemoveFromBlindiNodeInPosition(int position) {

        // if the position is the last one or the len is not larger than 2- zeros the position
        if (position == this->len - 1 || this->len <= 2) {
            if (this->len > 2 || (this->len == 2 & position == 1)) { //
                this->data_ptr[position] = 0;
                this->len -= 1;
                this->blindiKeys[position-1] = 0;
                return REMOVE_SUCCESS;
            }
            if (this->len == 2) { // len == 2 and the position is 0
                this->blindiKeys[0] = 0;
                this->data_ptr[0] = this->data_ptr[1];
                this->data_ptr[1] = 0;
                this->len = 1;
                return REMOVE_SUCCESS;
            }
            // if len == 1
            this->len = 0;
            return REMOVE_NODE_EMPTY;
        }

        // if len above 2 and not the last one
        if (position != 0) {
            this->blindiKeys[position - 1] = min(this->blindiKeys[position], this->blindiKeys[position-1]);
        }
        for (int i = position + 1; i <= this->len-2; i++) {
            this->data_ptr[i - 1] = this->data_ptr[i];
            this->blindiKeys[i - 1] = this->blindiKeys[i];
        }
        this->data_ptr[len - 2] = this->data_ptr[len-1];
        this->len -= 1;
        return REMOVE_SUCCESS;
    }

    // The merge is from the right node to the left node
    int MergeBlindiNodes(LinearBlindiNode *left_node, LinearBlindiNode *right_node) {
        // copy the data from right node to left node
        for (int i=0; i< right_node->len-1; i++) {
            left_node->data_ptr[left_node->len + i] = right_node->data_ptr[i];
            left_node->blindiKeys[left_node->len + i] = right_node->blindiKeys[i];
        }
        left_node->data_ptr[left_node->len + right_node->len - 1] = right_node->data_ptr[right_node->len - 1];
        // calculate the BlindKey between the two nodes
        uint8_t *last_left_key = left_node->bring_key(left_node->len - 1);  // bring the last key from the left node from the DB
        uint8_t *first_right_key = right_node->bring_key(0);  // bring the first key from the right node from the DB
        bool hit, eq;
        left_node->blindiKeys[left_node->len - 1] = (uint8_t)CompareStage(last_left_key ,first_right_key, &hit, &eq); // put the new blindiKey in the blindiKeys[len-1] (middle between the two nodes)

        // calc new len
        left_node->len += right_node->len;
    }
};

template<class NODE>
class GenericBlindiNode {
public:
    NODE node;

    GenericBlindiNode(int nodeSize) : node(nodeSize) {
    }

    uint8_t get_len() {
        return node.get_len();
    }

    uint8_t *get_data_ptr(int pos) {
        return node.get_data_ptr(pos);
    }

    // Predecessor search -> return the position of the largest key not greater than x.
    // get the node, the key and a pointer to hit, return a position in the node. If hit: the was exact as the key in the DB. If miss: the key does not appear in the DB
    // We return the value of diff_pos and right_left on demand (if they are not NULL)
    int BlindiNode_search(const uint8_t *search_key, bool search_type, bool *hit, bool *smaller_than_node = 0, int *diff_pos = NULL, bool *right_left =  NULL) {
        bool _eg = false;
        bool _right_left = false;
        int _diff_pos = 0;

        int first_stage_pos = 0;
        if (node.get_len() > 1) {
            first_stage_pos = node.SearchFirstStage(search_key);
        }

        uint8_t *db_key = node.bring_key(first_stage_pos);  // bring the key from the DB

        _diff_pos = CompareStage (search_key, db_key, hit, &_right_left);
        if (search_type == POINT_SEARCH || *hit) { // We return a value if it is a Point search or if we had a hit
            return first_stage_pos;
        }
        if ((diff_pos != NULL) & (right_left != NULL)) {
            *diff_pos = _diff_pos;
            *right_left = _right_left;
        }
        if (node.get_len() == 1) {
            *smaller_than_node = !_right_left;
            return first_stage_pos;
        }
        return node.SearchSecondStage(first_stage_pos, _diff_pos, _right_left, smaller_than_node);
    }

    // BlindiNode_search
    // insert2BlindiNodeInPosition
    // return success or duplicated key or overflow
    int insert2BlindiNodewithKey(uint8_t *insert_key, uint8_t *data_ptr) {
        bool hit = 0;
        bool smaller_than_node = 0;
        int position = 0;
        int diff_pos = 0;
        bool right_left = 0;

        if (node.get_len() == MAX_NODE_SIZE) {
            return INSERT_OVERFLOW;
        }

        if (node.get_len() != 0) { // if the node is not empty
            position = BlindiNode_search(insert_key, PREDECESSOR_SEARCH, &hit, &smaller_than_node, &diff_pos, &right_left);
        } else {
            node.first_insert(data_ptr);
            return INSERT_SUCCESS;
        }

        if (hit) {  // it was exeact we have duplicated key
//			printf("Error: Duplicated key\n");
            return INSERT_DUPLICATED_KEY;
        }
//		insert2BlindiNodeInPosition(position, insert_key, data_ptr, smaller_than_node);
        node.insert2BlindiNodeInPosition(position, insert_key, data_ptr, smaller_than_node, &diff_pos, &right_left);
        return INSERT_SUCCESS;
    }

    int RemoveFromBlindiNodewithKey(uint8_t *remove_key) {
        bool hit = 0;
        bool smaller_than_node = 0;
        int position = 0;

        position = BlindiNode_search(remove_key, POINT_SEARCH, &hit, &smaller_than_node);
        if (hit == 0) {
            return REMOVE_KEY_NOT_FOUND;
        }
        int remove_result =  node.RemoveFromBlindiNodeInPosition(position);
        if ((node.get_len() < MAX_NODE_SIZE / 2)) {
            if (remove_result == REMOVE_SUCCESS) {
                return REMOVE_UNDERFLOW;
            }
        }
        return remove_result;
    }

    void splitBlindiNode(GenericBlindiNode *new_node, uint8_t *insert_key, uint8_t *data_ptr) {
        bool hit;
        bool smaller_than_node;
        int position;
        // search key
        position = this->BlindiNode_search(insert_key, PREDECESSOR_SEARCH, &hit, &smaller_than_node);

        int half_node_size = node.splitBlindiNode(&new_node->node);

        // insert tuple
        if (position >= half_node_size) {
            new_node->node.insert2BlindiNodeInPosition(position - half_node_size, insert_key, data_ptr);
        } else {
            this->node.insert2BlindiNodeInPosition(position, insert_key, data_ptr, smaller_than_node);
        }
    }

    int MergeBlindiNodes(GenericBlindiNode *left_node, GenericBlindiNode *right_node) {
        return left_node->node.MergeBlindiNodes(&left_node->node, &right_node->node);
    }
};
/*
// compare the bring key with search key- find the first bit fdiifreance between key and the data we bring from DB.
int CompareStage (const uint8_t *key1, const uint8_t *key2, bool *eq, bool *big_small)
{
    *eq = false;
    for (int i=KEY_SIZE_BYTE - 1; i >= 0 ; i--) {
        int mask_byte =  *(key1+i) ^ *(key2+i);
        if (mask_byte != 0) {
            if (*(key1+i) > *(key2+i)) {
                *big_small = true;
            } else {
                *big_small = false;
            }
            return 8*(KEY_SIZE_BYTE - 1 - i) + firstMsbLookUpTable(mask_byte);
        }
    }
    *eq = true;
    return 0;
}
*/
