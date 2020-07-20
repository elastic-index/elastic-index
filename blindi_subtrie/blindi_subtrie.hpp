// C++ program for BlindI node
#ifndef _BLINDI_SUBTRIE_H_
#define _BLINDI_SUBTRIE_H_


#include<iostream>
#include <cassert>
#include <bit_manipulation.hpp>
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

// The Key valid_length is contsant and up to 255 bits

// #define FINGERPRINT
// #define KEY_VAR_LEN

//#define ASSERT
//#define DEBUG_BLINDI
//#define DEBUG_FIRST_STAGE
//#define BREATHING_SIZE 2
//#define BREATHING_DATA_ONLY

//#define BREATHING_STSTS
//#ifndef _BREATHING_STSTS_DEFINE_
//#define _BREATHING_STSTS_DEFINE_
//static uint64_t breathing_sum = 0;
//static uint64_t breathing_count = 0;
//static uint64_t insert_count = 0;
//#endif

using namespace std;

int CompareStage (const uint8_t *key1, const uint8_t *key2, bool *eq, bool *big_small, uint16_t key_len_key1, uint16_t key_len_key2);

// A BlindI node
template <typename _Key, int NUM_SLOTS>
class SubTrieBlindiNode {


    static_assert((NUM_SLOTS % 2) == 0, "NUM_SLOTS is not even");
//    static_assert((NUM_SLOTS <= 256), "NUM_SLOTS > 256");


    typedef typename std::conditional<sizeof(_Key) < 32,
    uint8_t, uint16_t>::type bitidx_t;

    typedef typename std::conditional<NUM_SLOTS < 256,
    uint8_t, uint16_t>::type idx_t;


#ifdef BREATHING_SIZE
     uint16_t currentmaxslot;
     uint8_t **key_ptr;// An array of pointers
#ifdef BREATHING_DATA_ONLY
    bitidx_t blindiKeys[NUM_SLOTS - 1]; // An array of BlindIKeys
    idx_t subtrie_array[NUM_SLOTS - 1]; // An Array of numbr of children in the subtrie
#else
     bitidx_t *blindiKeys; // An array of BlindIKeys
     idx_t *subtrie_array; // An Array of numbr of children in the subtrie
#endif
#else
    bitidx_t blindiKeys[NUM_SLOTS - 1];// An array of BlindIKeys
    idx_t subtrie_array[NUM_SLOTS - 1]; // An Array of numbr of children in the subtrie
    uint8_t *key_ptr[NUM_SLOTS]; // An array of pointers
#endif

#ifdef KEY_VAR_LEN
    uint16_t key_len[NUM_SLOTS]; // the length of the key TBD
#endif
#ifdef FINGERPRINT
    uint8_t fingerprint[NUM_SLOTS]; // a hash to make sure this is the right key
#endif
    idx_t valid_len;// the current number of blindiKeys in the blindiNode

public:
    SubTrieBlindiNode() {
        valid_len = 0;
    }   // Constructor

    uint16_t get_valid_len() const {
        return this->valid_len;
    }

    void set_valid_len(uint16_t val) { 
        this->valid_len = val;
    }

    uint8_t *get_key_ptr(uint16_t pos) const {
        return this->key_ptr[pos];
    }

	uint8_t **get_ptr2key_ptr(){
		return this->key_ptr;
	}

    uint16_t get_blindiKey(uint16_t pos) const {
        return this->blindiKeys[pos];
    }

    uint16_t get_subtrie(uint16_t pos) const {
        return this->subtrie_array[pos];
    }

#ifdef BREATHING_SIZE
    inline bool isneedbreath() const
    {
	    return (this->valid_len == this->currentmaxslot);
    }
#endif



    void first_insert(uint8_t *insert_key, uint32_t key_len = 0, uint8_t fingerprint = 0) {
#ifdef BREATHING_STSTS 
	breathing_sum += NUM_SLOTS;
	breathing_count++;
#endif
#ifdef BREATHING_SIZE
	this->currentmaxslot = NUM_SLOTS;
        this->key_ptr = (uint8_t **)malloc (NUM_SLOTS * sizeof(uint8_t *));  
#ifndef BREATHING_DATA_ONLY 
        this->blindiKeys = (bitidx_t *)malloc ((NUM_SLOTS - 1) * sizeof(bitidx_t));  
        this->subtrie_array = (idx_t *)malloc ((NUM_SLOTS - 1) * sizeof(idx_t)); 
#endif 
#endif
	this->blindiKeys[0] = 0;
	this->subtrie_array[0] = 0;
	this->key_ptr[0] = insert_key;

#ifdef KEY_VAR_LEN
        this->key_len[0] = key_len;
#endif
#ifdef FINGERPRINT
        this->fingerprint[0] = fingerprint;
#endif
        this->valid_len = 1;
    }

public:

    void print_node () {
        printf ("\nvalid_len %d", valid_len);

        printf ("\n(blindiKeys, subkeys ) ");
        for (int i = 0; i<= valid_len - 2; i++) {
            printf( "( %d , %d ), ", get_blindiKey(i), get_subtrie(i));
        }
        printf ("\n");
        printf ("key_ptr ");
        for (int i = 0; i<= valid_len - 1; i++) {
            printf ("%d ", get_key_ptr(i)[0]);
        }
        printf ("\n ");
    }


#ifdef KEY_VAR_LEN
    uint16_t get_key_len (uint16_t pos) {
        return this->key_len[pos]; // the length of the key
    }
#endif

    // A function to search first stage on this key
    // return the position

    uint16_t SearchFirstStage (const uint8_t *key_ptr, uint16_t key_len) {
        uint16_t key_ptr_pos = 0;
        uint16_t blindiKey_pos = 0;
        uint16_t n = valid_len - 1;

        while (n > 0) {
            if (isNthBitSetInString(key_ptr, blindiKeys[blindiKey_pos], key_len)) {// check if the bit is set  - the blindiKey is because of branch
                key_ptr_pos = key_ptr_pos + subtrie_array[blindiKey_pos];
                n = n - subtrie_array[blindiKey_pos];
                blindiKey_pos = blindiKey_pos + subtrie_array[blindiKey_pos];
            } else {
                n = subtrie_array[blindiKey_pos] - 1;
                blindiKey_pos += 1;
            }
        }
        return key_ptr_pos;
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
    uint16_t SearchSecondStage (int diff_pos, bool large_small, uint16_t first_stage_pos, bool *smaller_than_node) {
        uint16_t key_ptr_pos = 0; // i
        uint16_t blindiKey_pos = 0; // j
        uint16_t n = valid_len - 1;
        uint16_t k = first_stage_pos;

        while ((n > 0) && (blindiKeys[blindiKey_pos] < diff_pos)) {
            if (k >= subtrie_array[blindiKey_pos]) {
                k = k - subtrie_array[blindiKey_pos];
                key_ptr_pos = key_ptr_pos + subtrie_array[blindiKey_pos];
                n = n - subtrie_array[blindiKey_pos];
                blindiKey_pos = blindiKey_pos +  subtrie_array[blindiKey_pos];
            } else {
                n = subtrie_array[blindiKey_pos] - 1;
                blindiKey_pos = blindiKey_pos +  1;
            }
        }
//	printf ("first_stage_pos %d key_ptr_pos %d large_small %d n %d\n", first_stage_pos, key_ptr_pos, large_small, n);
        if (large_small) {
            return key_ptr_pos + n;
        } else {
            if (key_ptr_pos == 0) {
                *smaller_than_node = true;
            } else {
                key_ptr_pos -= 1;
            }
        }
        return key_ptr_pos;
    }
    // A function to insert a BlindiKey in position in non-full BlindiNode
    // the position - is the position in the blindiKeys

    void Insert2BlindiNodeInPosition(uint16_t search_pos, uint16_t key_len, uint8_t *insert_key, int *diff_pos, bool *large_small, bool *smaller_than_node, uint16_t *tree_traverse, uint16_t *tree_traverse_len, uint8_t fingerprint = 0) {
        // insert all the items in the new plac//e
        int key_ptr_pos = 0;
        int blindiKey_pos = 0;
        int n = valid_len - 1;
        int k = search_pos;
#ifdef BREATHING_SIZE
	if (this->isneedbreath()) { 
         
		if (currentmaxslot + BREATHING_SIZE > NUM_SLOTS) 
		{
#ifdef BREATHING_STSTS 
			breathing_sum += NUM_SLOTS - currentmaxslot;
			breathing_count++;
#endif
			currentmaxslot = NUM_SLOTS;
		}
		else 
		{
			currentmaxslot = currentmaxslot + BREATHING_SIZE;
#ifdef BREATHING_STSTS 
			breathing_sum += BREATHING_SIZE;
			breathing_count++;
#endif
		}
		uint8_t **tmp_key_ptr = (uint8_t **)malloc (currentmaxslot * sizeof(uint8_t *)); 
		std::copy(key_ptr, key_ptr + valid_len,
				tmp_key_ptr);
		free(key_ptr);
		key_ptr = tmp_key_ptr;
 
#ifndef BREATHING_DATA_ONLY
		bitidx_t *tmp_blindiKeys = (bitidx_t *)malloc ((currentmaxslot - 1) * sizeof(bitidx_t));
		idx_t *tmp_subtrie_array = (idx_t *)malloc ((currentmaxslot - 1) * sizeof(idx_t));
		std::copy(blindiKeys, blindiKeys + valid_len - 1,
				tmp_blindiKeys);
		std::copy(subtrie_array, subtrie_array + valid_len - 1,
			tmp_subtrie_array);
		free(blindiKeys);
		free(subtrie_array);
		blindiKeys = tmp_blindiKeys;
		subtrie_array = tmp_subtrie_array; 
#endif 
	} 
#endif

        // update the subtrie array
        while ((n > 0) && (blindiKeys[blindiKey_pos] < *diff_pos)) {
//		printf ("k %d subtrie_array[blindiKey_pos] %d \n", k,subtrie_array[blindiKey_pos]);
            if (k + !*large_small - *smaller_than_node >= subtrie_array[blindiKey_pos]) {
                k = k - subtrie_array[blindiKey_pos];
                key_ptr_pos = key_ptr_pos + subtrie_array[blindiKey_pos];
                n = n - subtrie_array[blindiKey_pos];
                blindiKey_pos = blindiKey_pos +  subtrie_array[blindiKey_pos];
            } else {
                n = subtrie_array[blindiKey_pos] - 1;
                subtrie_array[blindiKey_pos] += 1; // add one since the new insert is in this left subtrie
                blindiKey_pos = blindiKey_pos +  1;
            }
        }
        // insert the diff_pos to the new place - update the subtrie_array
        // shift key_ptr
        int i = 0;
        for (i = valid_len - 1; i > search_pos - *smaller_than_node; i--) {
            this->key_ptr[i + 1] = this->key_ptr[i];
        }
        this->key_ptr[search_pos  + 1 - *smaller_than_node] = insert_key;
        // shift subtrie array
        for (i = valid_len - 2; i >=blindiKey_pos; i--) {
            subtrie_array[i+1] = subtrie_array[i];
        }

        if (n > 0 && k + !*large_small - *smaller_than_node >= subtrie_array[blindiKey_pos])	{ // if connected the new blindiKey above the small subtrie -> add 1 to the previous subtrie of the root than valid_len
            if (blindiKey_pos == 0) {
                subtrie_array[blindiKey_pos] = valid_len;
            } else {
                subtrie_array[blindiKey_pos] = k + 1;
            }
        } else {// if connected the new blindiKey above the small subtrie -> 1
            subtrie_array[blindiKey_pos] = 1;
        }

        // shift blindiKeys_array
        for (i = valid_len - 2; i >=blindiKey_pos; i--) {
            blindiKeys[i+1] = blindiKeys[i];
        }
        blindiKeys[blindiKey_pos] = *diff_pos;
        valid_len ++;
    }

    // split
    // create two sub trees positions - tmp_node1 - tmp_node2
    //  1. (0 - NUM_SLOTS /2 -1) -> copy it to new_node1
    //  2. (NUM_SLOTS/2 : NUM_SLOTS -1) -> copy it to new_node2

    uint16_t SplitBlindiNode(SubTrieBlindiNode *node_small, SubTrieBlindiNode *node_large) {
#ifdef BREATHING_SIZE
	uint16_t cms;
	if (NUM_SLOTS / 2 + BREATHING_SIZE > NUM_SLOTS) 
	{
#ifdef BREATHING_STSTS 
		breathing_sum += NUM_SLOTS;
#endif
	        cms = NUM_SLOTS;
	}
	else 
	{
#ifdef BREATHING_STSTS 
		breathing_sum += NUM_SLOTS / 2 + BREATHING_SIZE;
#endif
	        cms = NUM_SLOTS / 2 + BREATHING_SIZE;
	}
	node_small->currentmaxslot = cms;
	node_large->currentmaxslot = cms;
        node_small->key_ptr = (uint8_t **)malloc (cms * sizeof(uint8_t *));  
        node_large->key_ptr = (uint8_t **)malloc (cms * sizeof(uint8_t *));  
#ifndef BREATHING_DATA_ONLY 
        node_small->blindiKeys = (bitidx_t *)malloc ((cms - 1) * sizeof(bitidx_t));  
        node_small->subtrie_array = (idx_t *)malloc ((cms - 1) * sizeof(idx_t)); 
        node_large->blindiKeys = (bitidx_t *)malloc ((cms - 1) * sizeof(bitidx_t));  
        node_large->subtrie_array = (idx_t *)malloc ((cms - 1) * sizeof(idx_t)); 
#endif 
#endif
        uint16_t mid = valid_len /2;
        uint16_t mid_m1 = mid - 1;
        // copy key ptr
        for (int i = 0; i < mid ; i++) {
            node_small->key_ptr[i] = this->key_ptr[i];
            node_large->key_ptr[i] = this->key_ptr[i+mid];
        }
        uint16_t cur_i = 0;
        uint16_t small_i = 0;
        uint16_t large_i = this->valid_len - 2;
        uint16_t j;
        uint16_t sum_movers2leftnode_from_traverse;
        uint16_t large_tree_traverse[NUM_SLOTS];
        uint16_t large_tree_traverse_len = 0;
        uint16_t tree_traverse[NUM_SLOTS];
        uint16_t tree_traverse_len = 0;
        uint16_t key_ptr_pos = 0;

        while (1) {
            if (key_ptr_pos +  subtrie_array[cur_i] <= mid_m1) { // this pos of blindiKey is smaller than all the pointers
                for (j = cur_i; j < cur_i + subtrie_array[cur_i]; j++) {
                    node_small->blindiKeys[small_i] = this->blindiKeys[j];
                    node_small->subtrie_array[small_i] = this->subtrie_array[j];
                    small_i++;
                }
                tree_traverse[tree_traverse_len] = cur_i;
                tree_traverse_len++;
                key_ptr_pos += subtrie_array[cur_i];
                cur_i += subtrie_array[cur_i];
                continue;
            }
            if (key_ptr_pos + subtrie_array[cur_i] > mid) { // this pos of blindiKey is smaller than all the pointers
                // copy the larger position of cur_i + subtrie_array
                for (j = large_i; j >= (cur_i + subtrie_array[cur_i]) ; j--) {
                    node_large->blindiKeys[j - mid] = this->blindiKeys[j];
                    node_large->subtrie_array[j - mid] = this->subtrie_array[j];
                    large_i--;
                }
                large_tree_traverse[large_tree_traverse_len] = cur_i;
                large_tree_traverse_len++;
                tree_traverse[tree_traverse_len] = cur_i;
                tree_traverse_len++;
                cur_i ++;
            } else { // this blindiKey is the divider
                // copy the small blindikeys
                for (j = cur_i + 1; j < cur_i + subtrie_array[cur_i]; j++) {
                    node_small->blindiKeys[small_i] = this->blindiKeys[j];
                    node_small->subtrie_array[small_i] = this->subtrie_array[j];
                    small_i++;
                }
                // copy the large blindikeys
                for (j = large_i; j >= (cur_i + subtrie_array[cur_i]) ; j--) {
                    node_large->blindiKeys[j - mid] = this->blindiKeys[j];
                    node_large->subtrie_array[j - mid] = this->subtrie_array[j];
                }
                large_i =  large_i - (cur_i + subtrie_array[cur_i]) + 1;
                sum_movers2leftnode_from_traverse = subtrie_array[cur_i];
                break;
            }
        }
        node_small->valid_len = mid;
        node_large->valid_len = mid;
        if ( large_tree_traverse_len == 0) {
            return 0;
        }
        for (j = tree_traverse_len - 1; j >= 0; j--) {
            if (tree_traverse[j] == large_tree_traverse[large_tree_traverse_len - 1]) {
                uint16_t pos = large_tree_traverse[large_tree_traverse_len - 1];
                node_large->blindiKeys[large_tree_traverse_len - 1] = this->blindiKeys[pos];
                node_large->subtrie_array[large_tree_traverse_len - 1] = this->subtrie_array[pos] - sum_movers2leftnode_from_traverse;
                if (large_tree_traverse_len == 1) {
                    break;
                } else {
                    large_tree_traverse_len--;
                }
            } else {
                sum_movers2leftnode_from_traverse += subtrie_array[tree_traverse[j]];
            }
        }
#ifdef BREATHING_SIZE
        free(key_ptr);  
#ifndef BREATHING_DATA_ONLY 
        free(blindiKeys);  
        free(subtrie_array); 
#endif 
#endif
        return 0;

    }
// Remove shfit key_ptr and shift BlindiKeys, & valid_len -= 1
    uint16_t RemoveFromBlindiNodeInPosition(uint16_t remove_position) {
        // insert all the items in the new plac//e
        uint16_t key_ptr_pos = 0;
        uint16_t blindiKey_pos = 0;
        uint16_t n = valid_len - 1;
        uint16_t k = remove_position;


        // update the subtrie array
        // find the location of the subtrie
        while (n > 0) {
            if (key_ptr_pos +  subtrie_array[blindiKey_pos] < remove_position + 1) { // // the pos is i the right subtrie
                k = k - subtrie_array[blindiKey_pos];
                key_ptr_pos = key_ptr_pos + subtrie_array[blindiKey_pos];
                n = n - subtrie_array[blindiKey_pos];
                blindiKey_pos += (n > 0) ?  subtrie_array[blindiKey_pos] : 0;
            }  else {
                n = subtrie_array[blindiKey_pos] - 1;
                subtrie_array[blindiKey_pos] = subtrie_array[blindiKey_pos]  - 1; // sub one since the new remove is in this left subtrie
                blindiKey_pos += (n > 0);
            }
        }


        // shift the key_ptr array
        int i = 0;
        for (i = remove_position; i < valid_len - 1; i++) {
            this->key_ptr[i] = this->key_ptr[i + 1];
        }

        // shift sub_tree
        for (i = blindiKey_pos; i < valid_len - 1; i++) {
            subtrie_array[i] = subtrie_array[i + 1];
            blindiKeys[i] = blindiKeys[i + 1];
        }
        /*
        // update the subtrie_array in the blindiKey_pos
        if (n > 0){ // if connected the new blindiKey above the small subtrie -> add 1 to the previous subtrie of the root than valid_len
        subtrie_array[blindiKey_pos] = k - 1;
        } else {// if connected the new blindiKey above the small subtrie -> 1
        subtrie_array[blindiKey_pos] = 1;
        }
         */
        valid_len --;
        return REMOVE_SUCCESS;
    }

// For Merge
// The merge is from the large node to small_node (this)
uint16_t MergeBlindiNodes(SubTrieBlindiNode *node_large) {
	// copy the key_ptr from large node to small_node (this) -> create two trees pointing to small node 	
	uint16_t small_node_len = this->get_valid_len();	
	uint16_t large_node_len = node_large->get_valid_len();	
	for (int i=0; i < large_node_len; i++){
		this->key_ptr[small_node_len + i] = node_large->key_ptr[i];   
#ifdef DATA_VAR_LEN                                                                                                
		this->data_len[small_node_len + i] = node_large->data_len[i];   
#endif                                                                                                             
#ifdef KEY_VAR_LEN                                                                                                 
		this->key_len[small_node_len + i] =  node_large->key_len[i] ;   
#endif                                                                                                             
#ifdef FINGERPRINT                                                                                                 
		this->fingerprint[small_node_len + i] = node_large->fingerprint[i];   
#endif
	}

	for (int i=0; i < large_node_len - 1; i++){
		this->blindiKeys[small_node_len + i] = node_large->blindiKeys[i];   
	}

	uint8_t *last_small_key = this->bring_key(valid_len - 1);  // bring the key from the DB
	uint8_t *first_large_key = node_large->bring_key(0);  // bring the key from the DB
	bool _eq = false;	
	bool _large_small = false;
	int _diff_pos = 0;
        _diff_pos = CompareStage (last_small_key, first_large_key, &_eq, &_large_small, sizeof(_Key), sizeof(_Key));
	this->blindiKeys[small_node_len] = _diff_pos;   

	uint16_t i_small = 0, i_large = 0; 
	uint16_t  new_valid_len = small_node_len + large_node_len;
	uint16_t i_new_forward = 0, i_new_backwords = large_node_len - 1;
	bool is_small_end = (small_node_len < 2);
	bool is_large_end = (large_node_len < 2);
	
	uint16_t new_subtrie_array[new_valid_len - 1]; 
	bitidx_t new_blindikeys_array[new_valid_len - 1]; 
	int j;	
	while (!is_small_end || !is_large_end) {
		// 3 options 
		if ((is_small_end || _diff_pos < this->blindiKeys[i_small]) && (is_large_end || _diff_pos < node_large->blindiKeys[i_large])) { // the delimetr is next
			break;
		}
		
		if (is_large_end || (!is_small_end && (this->blindiKeys[i_small] < node_large->blindiKeys[i_large]))) { // node_small is next
			for (j = i_small; j < i_small + this->subtrie_array[i_small]; j++) {
				new_subtrie_array[i_new_forward] = this->subtrie_array[j];
				new_blindikeys_array[i_new_forward] = this->blindiKeys[j];
				i_new_forward++;
			}
			i_small = j; 
			if(i_small == (small_node_len  - 1)) {
				is_small_end = 1;	
			}	 
		}else {// node_large is next
			new_subtrie_array[i_new_forward] = node_large->subtrie_array[i_large] + small_node_len - i_small;
			new_blindikeys_array[i_new_forward] = node_large->blindiKeys[i_large];
			for (j = i_new_backwords; j > node_large->subtrie_array[i_large] + i_large; j--) {
				new_subtrie_array[i_new_backwords + small_node_len - 1] = node_large->subtrie_array[j - 1];
				new_blindikeys_array[i_new_backwords + small_node_len - 1] = node_large->blindiKeys[j - 1];
				i_new_backwords--;
			}
			i_new_backwords = j; 
			i_new_forward++;
			i_large++;
			if(i_large - i_new_backwords == 0) {
				is_large_end = 1;	
			}	 
		}		
	}
        
        
	new_subtrie_array[i_new_forward] = small_node_len - i_small;
	new_blindikeys_array[i_new_forward] = _diff_pos;
	i_new_forward++;
          
	for (j = i_small; j < small_node_len - 1 ; j++) {
		new_subtrie_array[i_new_forward] = this->subtrie_array[j];
		new_blindikeys_array[i_new_forward] = this->blindiKeys[j];
		i_new_forward++;
	}
	for (j = i_large; j < i_new_backwords ; j++) {
		new_subtrie_array[i_new_forward] = node_large->subtrie_array[j];
		new_blindikeys_array[i_new_forward] = node_large->blindiKeys[j];
		i_new_forward++;
	}

	for (int i=0; i < new_valid_len - 1; i++) {
		this->subtrie_array[i] = new_subtrie_array[i];	
		this->blindiKeys[i] = new_blindikeys_array[i];	
	}
	this->valid_len = new_valid_len;		
	return 0;
    }

    int CopyNode(SubTrieBlindiNode *node_large) {
         // copy the key_ptr from large node to small_node (this) -> create two trees pointing to small node
        int large_node_len = node_large->get_valid_len();
        for (int i=0; i < large_node_len - 1; i++) {
            this->key_ptr[i] = node_large->key_ptr[i];
            this->blindiKeys[i] = node_large->blindiKeys[i];
            this->subtrie_array[i] = node_large->subtrie_array[i];
             
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
class GenericBlindiSubTrieNode {
public:
    NODE node;

    GenericBlindiSubTrieNode() : node() { }

    uint16_t get_valid_len() const {
        return node.get_valid_len();
    }

    uint16_t get_valid_len() {
        return node.get_valid_len();
    }

    uint8_t *get_key_ptr(uint16_t pos) const {
        return node.get_key_ptr(pos);
    }

	uint8_t **get_ptr2key_ptr(){
		return node.get_ptr2key_ptr();
	}

    _Key get_max_key_in_node() const {
        return (*((_Key*) node.get_key_ptr(node.get_valid_len() - 1)));
    }

    _Key get_min_key_in_node() const {
        return (*((_Key*) node.get_key_ptr(0)));
    }


    _Key get_mid_key_in_node() const {
        return (*((_Key*) node.get_key_ptr(node.get_valid_len() / 2  - 1)));
    }

    // Predecessor search -> return the position of the largest key not greater than x.
    // get the node, the key and a pointer to hit, return a position in the node. If hit: the was exact as the key in the DB. If miss: the key does not appear in the DB
    // We return the value of diff_pos and large_small on demand (if they are not NULL)
    // Tree  search
    uint16_t SearchBlindiNode(const uint8_t *search_key, uint16_t key_len, bool search_type, bool *hit, bool *smaller_than_node, uint16_t *tree_traverse, uint16_t *tree_traverse_len, uint8_t fingerprint = 0, int *diff_pos = NULL, bool *large_small =  NULL) {
        bool _large_small = false;
        int _diff_pos = 0;
        *smaller_than_node = false;
        uint16_t first_stage_pos = 0;
        if (node.get_valid_len() > 1) {
            first_stage_pos = node.SearchFirstStage(search_key, key_len);
        }
#ifdef DEBUG_BLINDI
        printf("AFTER first stage\n");
//	node.print_tree_traverse(tree_traverse, tree_traverse_len);
        printf("XXXXXXX\n");
#endif

#ifdef FINGERPRINT
        if (search_type == POINT_SEARCH && fingerprint !=  node.bring_fingerprint(first_stage_pos)) { // We return a value if it is a Point search or if we had a hit
            *smaller_than_node = false;
            *hit = false;
            return first_stage_pos;
        }
#endif

        uint8_t *db_key = node.bring_key(first_stage_pos);  // bring the key from the DB

        _diff_pos = CompareStage (search_key, db_key, hit, &_large_small, sizeof(_Key), sizeof(_Key));
        if (search_type == POINT_SEARCH || *hit) { // We return a value if it is a Point search or if we had a hit
            *smaller_than_node = false;
            return first_stage_pos;
        }

        if ((diff_pos != NULL) && (large_small != NULL)) { // this search is for insert - don't need to bring the real position
            *diff_pos = _diff_pos;
            *large_small = _large_small;
        }
//	node.print_tree_traverse(tree_traverse, tree_traverse_len);
        return node.SearchSecondStage(_diff_pos, _large_small, first_stage_pos, smaller_than_node);
    }


    // SearchBlindiNode
    // Insert2BlindiNodeInPosition
    // return success or duplicated key or overflow
    // INSERT TREE
    uint16_t Insert2BlindiNodeWithKey(uint8_t *insert_key, uint16_t key_len = sizeof(_Key), uint8_t fingerprint = 0) {
#ifdef BREATHING_STSTS 
	insert_count++;
#endif
        bool hit = 0;
        int diff_pos = 0;
        bool large_small = 0;
        uint16_t tree_traverse[NUM_SLOTS] = {0};
        uint16_t tree_traverse_len;
        bool smaller_than_node = false;
        uint16_t search_pos;

        if (node.get_valid_len() == NUM_SLOTS) {
            return INSERT_OVERFLOW;
        }

        if (node.get_valid_len() != 0) { // if the node is not empty
            search_pos = SearchBlindiNode(insert_key, key_len, PREDECESSOR_SEARCH, &hit, &smaller_than_node, &tree_traverse[0], &tree_traverse_len, fingerprint, &diff_pos, &large_small);
        } else {
            node.first_insert(insert_key, key_len, fingerprint);
            return INSERT_SUCCESS;
        }

        if (hit) {  // it was exeact we have duplicated key
//			printf("Error: Duplicated key\n");
            return INSERT_DUPLICATED_KEY;
        }
//	std::cout << "before insert " << std::endl; 
//	node.print_node();
        node.Insert2BlindiNodeInPosition(search_pos, key_len, insert_key, &diff_pos, &large_small, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
//	std::cout << "after insert " << std::endl; 
#ifdef BREATHING_STSTS
	if (!(insert_count % (1024*1024))){ 
		std::cout << "insert_count " << insert_count <<" breathing_sum " << breathing_sum << " breathing_count " << breathing_count << " ";
	}
#endif
//	node.print_node();
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
        uint16_t search_pos;

        if (node.get_valid_len() == 0) {  // if the node is not empty
            node.first_insert(insert_key, key_len, fingerprint);
            return INSERT_SUCCESS;
        }

        search_pos = SearchBlindiNode(insert_key, key_len, PREDECESSOR_SEARCH, &hit, &smaller_than_node, &tree_traverse[0], &tree_traverse_len, fingerprint, &diff_pos, &large_small);

        if (node.get_valid_len() == NUM_SLOTS && !hit)
            return INSERT_OVERFLOW;

        node.Insert2BlindiNodeInPosition(search_pos, key_len, insert_key, &diff_pos, &large_small, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        return (hit)? INSERT_DUPLICATED_KEY : INSERT_SUCCESS;
    }


    void SplitBlindiNode(GenericBlindiSubTrieNode *node_small, GenericBlindiSubTrieNode *node_large) {
//	std::cout << "before split " << std::endl; 
        node.SplitBlindiNode(&node_small->node, &node_large->node);
//	std::cout << "after split " << std::endl; 
    }


    uint16_t MergeBlindiNodes(GenericBlindiSubTrieNode *large_node) {
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
        uint16_t position = 0;
        uint16_t tree_traverse[NUM_SLOTS] = {0};
        uint16_t tree_traverse_len;
        bool smaller_than_node;


        uint16_t remove_position = SearchBlindiNode(remove_key, key_len, POINT_SEARCH, &hit, &smaller_than_node, &tree_traverse[0], &tree_traverse_len, fingerprint);
        if (hit == 0) {
            return REMOVE_KEY_NOT_FOUND;
        }
        if (node.get_valid_len() == 1) {
	    node.set_valid_len(0);	
            return REMOVE_NODE_EMPTY;
        }
        uint16_t remove_result =  node.RemoveFromBlindiNodeInPosition(remove_position);
        uint16_t valid_len = node.get_valid_len();
        if ((valid_len < NUM_SLOTS / 2)) {
            if (remove_result == REMOVE_SUCCESS) {
                return REMOVE_UNDERFLOW;
            }
        }
        return remove_result;
    }
};
/*
#ifndef _BLINDI_COMPARE_H_
#define _BLINDI_COMPARE_H_
// compare the bring key with search key- find the first bit fdiifreance between searched key and the key we bring from DB.
int CompareStage (const uint8_t *key1, const uint8_t *key2, bool *eq, bool *big_small, uint16_t key_len_key1, uint16_t key_len_key2)
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
            if (*(key1 + key_len_key1 - i) > *(key2 + + key_len_key2 - i)) {
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
#endif // _BLINDI_SEQTREE_H_
