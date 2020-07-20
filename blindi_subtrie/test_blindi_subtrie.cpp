#include <iostream>
#include <stdint.h>
#include <blindi_subtrie.hpp>
#include <assert.h>

using namespace std;
#define TEST_EASY
#define TEST_MID1
#define TEST_MID2
#define TEST_INSERT
#define INSERT_SEARCH
#define TEST_RAND
#define TEST_SPLIT
#define TEST_REMOVE
#define TEST_MERGE
// #define TEST_MERGE_m1
// #define TEST_MERGE_m2
// #define TEST_MERGE_m3
//// #define TEST_MERGE_BUG // need to change the MAX_NODE_SIZE to 4
/// #define TEST_MERGE_BUG3 // need to change the MAX_NODE_SIZE to 8
// #define TEST_MIX
#define DEBUG
uint64_t first_stage_len_agg = 0;

typedef GenericBlindiNode<TreeBlindiNode> BlindiNode;

void print_key_in_octet(uint8_t *key)
{
    for (int i=KEY_SIZE_BYTE-1; i>= 0; i--) {
        printf("%03u ", key[i]);
    }
    printf(" ");
}

int cmp2keys(uint8_t *key1, uint8_t *key2)
{
    for (int i=KEY_SIZE_BYTE-1; i>= 0; i--) {
        if (key1[i] > key2[i])
            return 1;
        if (key1[i] < key2[i])
            return -1;
    }
    return 0;
}

#ifdef TEST_RAND
int rand_test(int in_seed, int number_of_return)
{
    BlindiNode node(MAX_NODE_SIZE);
    bool hit = 0;
    bool smaller_than_node = 0;
    bool large_small = 0;
    int position = 0;
    uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
    int tree_traverse_len;
    uint8_t fingerprint;
    bool eq_key_data = 0;
    // zeroes the inputs
    uint8_t k[MAX_NODE_SIZE][KEY_SIZE_BYTE];
    for (int i=0; i< MAX_NODE_SIZE; i++) {
        for (int j=0; j< KEY_SIZE_BYTE; j++) {
            k[i][j] = {0};
        }
    }

    int test_key_size = 16; //4 // 16
    for (int i=0; i< MAX_NODE_SIZE; i++) {
        for (int j=0; j< test_key_size; j++) {
            k[i][j] = rand()%255;
        }
        for (int j = 0; j < i; j++) {
            CompareStage (&k[j][0], &k[i][0], &eq_key_data, &large_small);
            if (eq_key_data) {
                i--;
                continue;
            }
        }
    }
    uint8_t tree_traverse_[MAX_NODE_SIZE] = {0};
    node.Insert2BlindiNodeWithKey(&k[0][0], &k[0][0]);
#ifdef DEBUG
    node.node.print_node();
#endif
    for (int i=1; i<MAX_NODE_SIZE; i++) {
        uint8_t tree_traverse_[MAX_NODE_SIZE] = {0};
        node.Insert2BlindiNodeWithKey(&k[i][0], &k[i][0]);
#ifdef DEBUG
        node.node.print_node();
        position = node.SearchBlindiNode(&k[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        printf("After insert search key %d -> position %d hit %d smaller_than_node %d \n ", k[i][0], position, hit, smaller_than_node);
        printf("%d search_key -> ", i);
        print_key_in_octet(&k[i][0]);
        printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
        CompareStage (&k[i][0], node.get_data_ptr(position), &eq_key_data, &large_small);
        assert(hit == 1 & eq_key_data == 1);

#endif //DEBUG
        int save_pos = 0;
        bool eq_test;
        bool large_small_test;
        for (int j=0; j< MAX_NODE_SIZE; j++) {
            position = node.SearchBlindiNode(&k[j][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
#ifdef DEBUG_FIRST_STAGE
            first_stage_len_agg += first_stage_len;
#endif
#ifdef DEBUG
            printf("check search key %d -> position %d hit %d smaller_than_node %d \n ", k[j][0], position, hit, smaller_than_node);
            CompareStage (&k[j][0], node.node.get_data_ptr(position), &eq_key_data, &large_small);
            assert ((eq_key_data | large_small) | (smaller_than_node & !large_small));
            if (j >=1) {
                CompareStage (node.node.get_data_ptr(save_pos), node.node.get_data_ptr(position), &eq_key_data, &large_small);
                CompareStage (&k[j-1][0], &k[j][0], &eq_test, &large_small_test);
                printf("pos %d: %d  pos: %d, %d large_small %d, eq %d\n", j-1, save_pos, j, position, large_small_test, eq_test);
                if (large_small_test) {
                    assert(large_small | eq_key_data);
                } else {
                    assert(!large_small | eq_key_data);
                }

            }
            save_pos = position;
#endif //DEBUG
        }
#ifdef DEBUG
// check max min mid
        uint8_t *max = node.get_max_in_node();
        int sum = 0;
        for (int m =0 ; m < node.get_valid_len(); m++) {
            CompareStage (&k[m][0], max, &eq_key_data, &large_small);
            if (!large_small || eq_key_data) {
                sum += 1;
            }
        }
        printf ("CHECK max[%d] = %d sum = %d\n",test_key_size - 1, max[test_key_size - 1], sum);
        assert(sum == node.get_valid_len());

        uint8_t *min = node.get_min_in_node();
        sum = 0;
        for (int m =0 ; m < node.get_valid_len(); m++) {
            CompareStage (&k[m][0], min, &eq_key_data, &large_small);
            if (large_small || eq_key_data) {
                sum += 1;
            }
        }
        printf ("CHECK min[%d] = %d sum = %d\n", test_key_size - 1, min[test_key_size -1], sum);
        assert(sum == node.get_valid_len());

        if (node.get_valid_len()%2 == 0) {
            uint8_t *mid = node.get_mid_in_node();
            sum = 0;
            for (int m =0 ; m < node.get_valid_len(); m++) {
                CompareStage (&k[m][0], mid, &eq_key_data, &large_small);
                if (!large_small || eq_key_data) {
                    sum += 1;
                }
            }
            printf ("CHECK mid[%d] = %d sum = %d\n",test_key_size - 1, mid[test_key_size - 1], sum);
            assert(sum == node.get_valid_len() / 2);
        }


#endif //DEBUG
    }
#ifdef DEBUG

    for (int i=0; i< node.node.get_valid_len() - 2; i++) {
        CompareStage (node.node.get_data_ptr(i), node.node.get_data_ptr(i+1), &eq_key_data, &large_small);
        assert (!eq_key_data & !large_small);
    }


#endif // end DEBUG	
}
#endif// end RAND
int main()
{
    uint8_t k[MAX_NODE_SIZE][KEY_SIZE_BYTE];
    for (int i=0; i< MAX_NODE_SIZE; i++) {
        for (int j=0; j< KEY_SIZE_BYTE; j++) {
            k[i][j] = {0};
        }
    }


//     srand(1);
    bool hit = 0;
    bool smaller_than_node = 0;
    bool large_small = 0;
    int position = 0;
    int tree_traverse_len;
    uint8_t fingerprint;
    bool eq_key_data = 0;
    uint8_t sorted_positions[MAX_NODE_SIZE] = {0};
    uint8_t desc_positions[MAX_NODE_SIZE] = {0};
    uint8_t *T[MAX_NODE_SIZE];

#ifdef TEST_EASY

    k[0][0] = {0};
    k[1][0] = {1};
    k[2][0] = {2};
    k[3][0] = {3};


    BlindiNode node1(MAX_NODE_SIZE);

    for (int i=0; i< 4; i++) {
        uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
        node1.Insert2BlindiNodeWithKey(&k[i][0], &k[i][0]);
        node1.node.print_node();
        position = node1.SearchBlindiNode(&k[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH,  &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        // checking
        printf("search key %d -> position %d hit %d smaller_than_node %d \n", k[i][0], position, hit, smaller_than_node);
        assert(hit == 1);
// search key 1 -> position 0 hit 1 smaller_than_node 0
// search key 2 -> position 1 hit 1 smaller_than_node 0
// search key 3 -> position 2 hit 1 smaller_than_node 0
// search key 4 -> position 3 hit 1 smaller_than_node 0
    }
    /*     // sorted order
         node1.node.bring_sorted_items(sorted_positions);
         for (int i=0; i< 4; i++) {
    	       int pos = sorted_positions[i];
    	       uint8_t *T;
    	       T = node1.node.get_data_ptr(pos);
    	       printf(" %d insert key: %d tree key %d\n", i, k[i][0], *T);
    	       CompareStage (&k[i][0], &T[0], &eq_key_data, &large_small);
    	       assert (eq_key_data);
         }
         // desc order
         node1.node.bring_descending_items(desc_positions);
         for (int i=0; i< 4; i++) {
    	       int pos = desc_positions[3 - i];
    	       uint8_t *T;
    	       T = node1.node.get_data_ptr(pos);
    	       printf(" %d insert key: %d tree key %d\n", i, k[i][0], *T);
    	       CompareStage (&k[i][0], &T[0], &eq_key_data, &large_small);
    	       assert (eq_key_data);
         }
    */

#endif// finish first test	

#ifdef TEST_MID1

    k[0][0] = {2};
    k[1][0] = {6};
    k[2][0] = {4};
    k[3][0] = {1};
    k[4][0] = {7};
    k[5][0] = {5};
    k[6][0] = {3};
    k[7][0] = {0};

    BlindiNode node2(8);
    for (int i=0; i<8; i++) {
        uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
        node2.Insert2BlindiNodeWithKey(&k[i][0], &k[i][0]);
        node2.node.print_node();
        position = node2.SearchBlindiNode(&k[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        printf("After insert search key %d -> position %d hit %d smaller_than_node %d \n ", k[i][0], position, hit, smaller_than_node);
//	     node2.node.print_tree_traverse(tree_traverse, &tree_traverse_len);
        assert(hit == 1);
        for (int j=0; j<8; j++) {
            position = node2.SearchBlindiNode(&k[j][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
            printf("check search key %d -> position %d hit %d smaller_than_node %d \n ", k[j][0], position, hit, smaller_than_node);
            CompareStage (&k[j][0], node2.node.get_data_ptr(position), &eq_key_data, &large_small);
            printf ( " eq_key_data %d large_small %d smaller_than_node %d\n", eq_key_data, large_small, smaller_than_node);
            assert ((eq_key_data | large_small) | (smaller_than_node & !large_small));
        }
    }

    // checking
    for (int i=0; i<8; i++) {
        uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
        position = node2.SearchBlindiNode(&k[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
//	     printf("search key %d -> position %d hit %d smaller_than_node %d \n ", k[i][0], position, hit, smaller_than_node);
        assert(hit == 1);
    }
    // check sorted order
    /*
         sorted_positions[MAX_NODE_SIZE] = {0};
         node2.node.bring_sorted_items(sorted_positions);
         for (int i=0; i< 8; i++) {
    	       int pos = sorted_positions[i];
    	       T[i] = node2.node.get_data_ptr(pos);
         }
         for (int i=0; i< 7; i++) {
    	     printf(" tree_key[%d] : %d tree_key [%d] %d\n", i, T[i][0], i+1, T[i+1][0]);
    	     CompareStage (&T[i][0], &T[i+1][0], &eq_key_data, &large_small);
    	     assert (!eq_key_data & !large_small);
         }
         // check desc order
         desc_positions[MAX_NODE_SIZE] = {0};
         node2.node.bring_descending_items(desc_positions);
         for (int i=0; i< 8; i++) {
    	       int pos = desc_positions[i];
    	       T[i] = node2.node.get_data_ptr(pos);
         }
         for (int i=0; i< 7; i++) {
    	     printf(" tree_key[%d] : %d tree_key [%d] %d\n", i, T[i][0], i+1, T[i+1][0]);
    	     CompareStage (&T[i][0], &T[i+1][0], &eq_key_data, &large_small);
    	     assert (!eq_key_data & large_small);
         }
    */
#endif

#ifdef TEST_MID2

    k[0][0] = {7};
    k[1][0] = {6};
    k[2][0] = {5};
    k[3][0] = {0};
    k[4][0] = {1};
    k[5][0] = {2};
    k[6][0] = {3};
    k[7][0] = {4};

    BlindiNode node3(8);

    for (int i=0; i<8; i++) {
        uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
        node3.Insert2BlindiNodeWithKey(&k[i][0], &k[i][0]);
        position = node3.SearchBlindiNode(&k[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        printf("After insert search key %d -> position %d hit %d smaller_than_node %d \n ", k[i][0], position, hit, smaller_than_node);
//	     node2.node.print_tree_traverse(tree_traverse, &tree_traverse_len);
        assert(hit == 1);
        for (int j=0; j<8; j++) {
            position = node3.SearchBlindiNode(&k[j][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
            printf("check search key %d -> position %d hit %d smaller_than_node %d \n ", k[j][0], position, hit, smaller_than_node);
            CompareStage (&k[j][0], node3.node.get_data_ptr(position), &eq_key_data, &large_small);
            assert ((eq_key_data | large_small) | (smaller_than_node & !large_small));
        }
    }

    for (int i=0; i<8; i++) {
        uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
        position = node3.SearchBlindiNode(&k[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        printf("search key %d -> position %d hit %d smaller_than_node %d \n ", k[i][0], position, hit, smaller_than_node);
        assert(hit == 1);
    }
    /*
    // check sorted order
         sorted_positions[MAX_NODE_SIZE] = {0};
         node3.node.bring_sorted_items(sorted_positions);
         for (int i=0; i< 8; i++) {
    	       int pos = sorted_positions[i];
    	       T[i] = node3.node.get_data_ptr(pos);
         }
         for (int i=0; i< 7; i++) {
    	     printf(" tree_key[%d] : %d tree_key [%d] %d\n", i, T[i][0], i+1, T[i+1][0]);
    	     CompareStage (&T[i][0], &T[i+1][0], &eq_key_data, &large_small);
    	     assert (!eq_key_data & !large_small);
         }
         }
    */

#endif // finish mod2 test

#ifdef TEST_RAND

    int number_of_return = 1000;
    for (int in=1; in<= number_of_return; in++) {
#ifdef DEBUG
        printf("\n\n\n\n ############### ROUND number %d #################### \n\n\n", in);
#endif
        rand_test(1, number_of_return);
    }

#ifdef DEBUG_FIRST_STAGE
    printf ("debug: tree length first stage: %02f\n", 1.0 * first_stage_len_agg / MAX_NODE_SIZE / number_of_return);
#endif
#endif// TEST_RAND	


#ifdef TEST_INSERT
    uint8_t sq[MAX_NODE_SIZE + 1][KEY_SIZE_BYTE];
    // zeroes the inputs
    for (int i=0; i< MAX_NODE_SIZE+1; i++) {
        for (int j=0; j< KEY_SIZE_BYTE; j++) {
            sq[i][j] = {0};
        }
    }
    for (int j=0; j<1000; j++) {
        BlindiNode node_s1(MAX_NODE_SIZE);
        BlindiNode node_n1(MAX_NODE_SIZE);
        BlindiNode node_n2(MAX_NODE_SIZE);
        int test_key_size = 16; //4 // 16
        for (int i=0; i< MAX_NODE_SIZE; i++) {
            sq[i][0] = i; // lsb is diffrent in all nodes
            for (int j=1; j< test_key_size; j++) {
                sq[i][j] = rand()%255;
            }
        }

        for (int i=0; i<MAX_NODE_SIZE; i++) {
            node_s1.Insert2BlindiNodeWithKey(&sq[i][0], &sq[i][0]);
        }
#ifdef INSERT_SEARCH
        for (int i=1; i<MAX_NODE_SIZE ; i++) {
            uint8_t tree_len;
            uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
            position = node_s1.SearchBlindiNode(&sq[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        }
#endif
    }
#endif


#ifdef TEST_SPLIT
    uint8_t s[MAX_NODE_SIZE + 1][KEY_SIZE_BYTE];
    // zeroes the inputs
    for (int i=0; i< MAX_NODE_SIZE+1; i++) {
        for (int j=0; j< KEY_SIZE_BYTE; j++) {
            s[i][j] = {0};
        }
    }
    for (int j=0; j<1000; j++) {
        BlindiNode node_s1(MAX_NODE_SIZE);
        BlindiNode node_n1(MAX_NODE_SIZE);
        BlindiNode node_n2(MAX_NODE_SIZE);
        int test_key_size = 16; //4 // 16
        for (int i=0; i< MAX_NODE_SIZE; i++) {
            s[i][0] = i; // lsb is diffrent in all nodes
            for (int j=1; j< test_key_size; j++) {
                s[i][j] = rand()%255;
            }
        }

        node_s1.Insert2BlindiNodeWithKey(&s[0][0], &s[0][0]);
        for (int i=1; i<MAX_NODE_SIZE; i++) {
            node_s1.Insert2BlindiNodeWithKey(&s[i][0], &s[i][0]);
        }

#ifdef DEBUG
        printf(" SPLIT ROUND %d \n", j +1);
        for (int i=0; i<MAX_NODE_SIZE ; i++) {
            uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
            position = node_s1.SearchBlindiNode(&s[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
            printf("search_key -> ");
            print_key_in_octet(&s[i][0]);
            printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
            if (i != MAX_NODE_SIZE) assert(hit == 1);
        }
#endif


        int position2 = 0;
        bool hit2 = 0;
        bool smaller_than_node2 = 0;
        tree_traverse_len = 0;
#ifdef DEBUG
        node_s1.node.print_node();
#endif
        node_s1.SplitBlindiNode(&node_n1, &node_n2);
#ifdef DEBUG
        node_n1.node.print_node();
        node_n2.node.print_node();
#endif

#ifdef DEBUG
        for (int i=MAX_NODE_SIZE - 1; i>0; i--) {
            uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
            tree_traverse_len =0;
            printf("First Node After Split\n ");
            position = node_n1.SearchBlindiNode(&s[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
            tree_traverse_len =0;
            printf("search_key -> ");
            print_key_in_octet(&s[i][0]);
            printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
            printf("Second Node After Split\n ");
            position2 = node_n2.SearchBlindiNode(&s[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit2, &smaller_than_node2, tree_traverse, &tree_traverse_len, fingerprint);
            printf("search_key -> ");
            print_key_in_octet(&s[i][0]);
            printf("-> position %d hit %d smaller_than_node %d \n ", position2, hit2, smaller_than_node2);
            assert(hit2 ^ hit == 1);
            if ( hit == 1) {	// if hit == 1 than node_s1 data_ptr == search key
                CompareStage (&s[i][0], node_n1.get_data_ptr(position), &eq_key_data, &large_small);
            } else {// if hit == 1 than node_s2 data_ptr == search key
                CompareStage (&s[i][0], node_n2.get_data_ptr(position2), &eq_key_data, &large_small);
            }
            assert (eq_key_data == 1);

            assert(position + position2 < MAX_NODE_SIZE);
            if (hit == 1) {
                assert(smaller_than_node2 == 1);  // if we are in the first node than the second node is smaller_than_node2
            }
        }
#endif
    }


#endif// finish split test	

#ifdef TEST_REMOVE

    int test_key_size = 16; //4 // 16
    int nu_return = 1000;
    for (int ra = 0; ra < nu_return; ra++) {
        BlindiNode node_d1(MAX_NODE_SIZE);
        uint8_t d[MAX_NODE_SIZE + 1][KEY_SIZE_BYTE];
        int remove_result;

        // zeroes the inputs
        for (int i=0; i< MAX_NODE_SIZE; i++) {
            for (int j=0; j< KEY_SIZE_BYTE; j++) {
                d[i][j] = {0};
            }
        }


        for (int i=0; i< MAX_NODE_SIZE; i++) {
            d[i][0] = i;
            for (int j=1; j< test_key_size; j++) {
                d[i][j] = rand()%255;
            }
        }

        for (int i=0; i<MAX_NODE_SIZE; i++) {
            node_d1.Insert2BlindiNodeWithKey(&d[i][0], &d[i][0]);
#ifdef DEBUG
            printf("%d insert_key -> ", i);
            print_key_in_octet(&d[i][0]);
            printf("\n");
#endif
        }

#ifdef DEBUG
        if (ra == 458) {
            printf("help\n");
        }
        printf(" REMOVE ROUND %d \n", ra+1);
        printf("After insert");
        node_d1.node.print_node();
#endif
        int remove_nu = 0;
        int remove_pos_arry[MAX_NODE_SIZE] = {0};
        for (int i=0; i< MAX_NODE_SIZE; i++) {
            uint8_t tree_traverse1[MAX_NODE_SIZE] = {0};
            hit = 0;
            int remove_pos = rand()% MAX_NODE_SIZE;

            node_d1.SearchBlindiNode(&d[remove_pos][0], KEY_SIZE_BYTE, POINT_SEARCH, &hit, &smaller_than_node, tree_traverse1, &tree_traverse_len, fingerprint);
            if (hit != 1) {
                continue;
            }
#ifdef DEBUG
            printf("remove_nu %d pos %d valid_len %d round %d \n", remove_nu, remove_pos, node_d1.node.get_valid_len(), ra);
#endif
            remove_result = node_d1.RemoveFromBlindiNodeWithKey(&d[remove_pos][0]);
            remove_pos_arry [remove_pos] = 1;

#ifdef DEBUG
            printf("After remove %d: ", i);
            print_key_in_octet(&d[remove_pos][0]);
            node_d1.node.print_node();
#endif

#ifdef DEBUG
            if (remove_nu == MAX_NODE_SIZE - 1) {
                assert(remove_result == 1); // REMOVE_NODE_EMPTY
            } else if (remove_nu >=  MAX_NODE_SIZE /2) {
                assert(remove_result == 2); // REMOVE_UNDERFLOW
            } else {
                printf("remove_result = %d %d \n", remove_result, remove_nu);
                assert(remove_result == 0); // REMOVE_SUCCESS
            }
            remove_nu += 1;



            printf ("This is remove nu %d !!!!!\n", remove_nu);
            if (remove_result != 1) {// check this part just if the node not empty after the remove
                for (int r=0; r< MAX_NODE_SIZE; r++) {
                    //			     printf("Search after Remove\n");
                    uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
                    smaller_than_node = 0;
                    hit = 0;
                    fingerprint =0;
                    position = node_d1.SearchBlindiNode(&d[r][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
                    //		     printf("%d search_key -> ", r);
                    //		     print_key_in_octet(&d[r][0]);
                    //		     printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
                    assert (position < node_d1.node.get_valid_len());
                    if (remove_pos_arry[r] == 1) { // the key has been removed
                        assert(hit == 0);
                    } else { // the key is here
                        bool eq_key_data = 0;
                        CompareStage (&d[r][0], node_d1.get_data_ptr(position), &eq_key_data, &large_small);
                        assert(hit == 1 & eq_key_data == 1);
                    }

                }
            }
#endif
        }
    }


#endif// TEST REMOVE

#ifdef TEST_MERGE_m1
    uint8_t e[MAX_NODE_SIZE + 1][KEY_SIZE_BYTE];
    // zeroes the inputs
    for (int i=0; i< MAX_NODE_SIZE; i++) {
        for (int j=0; j< KEY_SIZE_BYTE; j++) {
            e[i][j] = {0};
        }
    }
    for (int j=0; j< 1000; j++) {
        BlindiNode node_e1(MAX_NODE_SIZE);
        BlindiNode node_e2(MAX_NODE_SIZE);
        int test_key_size = 16; //4 // 16
        int max_in_small_node;
        switch (j%3) {
        case 0:
            max_in_small_node = 96;
            break;
        case 1:
            max_in_small_node = 128;
            break;
        case 2:
            max_in_small_node = 168;
            break;
        }

        for (int i=0; i< MAX_NODE_SIZE / 2 ; i++) {
            for (int j = 0; j< test_key_size; j++) {
                e[i][j] = rand()%max_in_small_node;
            }
            e[i][0] = i; // lsb is diffrent in all nodes
        }
        for (int i=MAX_NODE_SIZE / 2 ; i< MAX_NODE_SIZE ; i++) {
            for (int j = 0; j< test_key_size; j++) {
                e[i][j] = rand()%(256 - max_in_small_node) + max_in_small_node;
            }
            e[i][0] = i; // lsb is diffrent in all nodes
        }


        for (int i=0; i< (MAX_NODE_SIZE / 2 - 1) ; i++) {
            node_e1.Insert2BlindiNodeWithKey(&e[i][0], &e[i][0]);
            node_e2.Insert2BlindiNodeWithKey(&e[MAX_NODE_SIZE/2 + i][0], &e[MAX_NODE_SIZE/2 + i][0]);
        }
        node_e1.Insert2BlindiNodeWithKey(&e[MAX_NODE_SIZE/2 - 1][0], &e[MAX_NODE_SIZE/2 - 1][0]);
        node_e1.MergeBlindiNodes(&node_e2);

#ifdef DEBUG
        printf("ROUND #%d\n ", j);
        for (int i=0;  i < MAX_NODE_SIZE - 1; i++) {
            uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
            uint8_t fingerprint = 0;
            printf("Node After Merge\n ");
            position = node_e1.SearchBlindiNode(&e[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
            printf("search_key -> ");
            print_key_in_octet(&e[i][0]);
            printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
            bool eq_key_data = 0;
            CompareStage (&e[i][0], node_e1.get_data_ptr(position), &eq_key_data, &large_small);
            assert(hit == 1 & eq_key_data == 1);
        }
#endif
    }
#endif

#ifdef TEST_MERGE_m3
    uint8_t w[MAX_NODE_SIZE + 1][KEY_SIZE_BYTE];
    // zeroes the inputs
    for (int i=0; i< MAX_NODE_SIZE; i++) {
        for (int j=0; j< KEY_SIZE_BYTE; j++) {
            w[i][j] = {0};
        }
    }
    for (int j=0; j< 1000; j++) {
        BlindiNode node_w1(MAX_NODE_SIZE);
        BlindiNode node_w2(MAX_NODE_SIZE);
        int test_key_size = 16; //4 // 16
        int max_in_small_node;
        switch (j%3) {
        case 0:
            max_in_small_node = 96;
            break;
        case 1:
            max_in_small_node = 128;
            break;
        case 2:
            max_in_small_node = 168;
            break;
        }

        for (int i=0; i< MAX_NODE_SIZE / 2 ; i++) {
            for (int j = 0; j< test_key_size; j++) {
                w[i][j] = rand()%max_in_small_node;
            }
            w[i][0] = i; // lsb is diffrent in all nodes
        }
        for (int i=MAX_NODE_SIZE / 2  ; i< MAX_NODE_SIZE ; i++) {
            for (int j = 0; j< test_key_size; j++) {
                w[i][j] = rand()%(256 - max_in_small_node) + max_in_small_node;
            }
            w[i][0] = i; // lsb is diffrent in all nodes
        }

        int w1_size = (rand()%((MAX_NODE_SIZE / 2) - 1)) + 1;
        int w2_size = (rand()%((MAX_NODE_SIZE / 2) - 1)) + 1;
        for (int i=0; i< w1_size ; i++) {
            node_w1.Insert2BlindiNodeWithKey(&w[i][0], &w[i][0]);
        }
        for (int i=0; i< w2_size ; i++) {
            node_w2.Insert2BlindiNodeWithKey(&w[MAX_NODE_SIZE/2 + i][0], &w[MAX_NODE_SIZE/2 + i][0]);
        }
        printf("ROUND #%d\n ", j);
        if (j == 32) {
            printf ("I am here\n");
        }
        printf ("w1_size %d w2_size %d\n", w1_size, w2_size);
        node_w1.MergeBlindiNodes(&node_w2);

#ifdef DEBUG
        for (int i=0;  i < w1_size; i++) {
            uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
            uint8_t fingerprint = 0;
            printf("Node After Merge\n ");
            position = node_w1.SearchBlindiNode(&w[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
            printf("search_key -> ");
            print_key_in_octet(&w[i][0]);
            printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
            bool eq_key_data = 0;
            CompareStage (&w[i][0], node_w1.get_data_ptr(position), &eq_key_data, &large_small);
            assert(hit == 1 & eq_key_data == 1);
            assert(position < w1_size);
        }
        for (int i= MAX_NODE_SIZE / 2;  i < (MAX_NODE_SIZE / 2) + w2_size; i++) {
            uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
            uint8_t fingerprint = 0;
            printf("Node After Merge\n ");
            position = node_w1.SearchBlindiNode(&w[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
            printf("search_key -> ");
            print_key_in_octet(&w[i][0]);
            printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
            bool eq_key_data = 0;
            CompareStage (&w[i][0], node_w1.get_data_ptr(position), &eq_key_data, &large_small);
            assert(hit == 1 & eq_key_data == 1);
            assert(position >= w1_size);
        }
#endif
    }
#endif


#ifdef TEST_MERGE_m2
    uint8_t p[MAX_NODE_SIZE + 1][KEY_SIZE_BYTE];
    // zeroes the inputs
    for (int i=0; i< MAX_NODE_SIZE; i++) {
        for (int j=0; j< KEY_SIZE_BYTE; j++) {
            p[i][j] = {0};
        }
    }
    for (int j=0; j< 1000; j++) {
        BlindiNode node_p1(MAX_NODE_SIZE);
        BlindiNode node_p2(MAX_NODE_SIZE);
        int test_key_size = 16; //4 // 16
        int max_in_small_node;
        switch (j%3) {
        case 0:
            max_in_small_node = 96;
            break;
        case 1:
            max_in_small_node = 128;
            break;
        case 2:
            max_in_small_node = 168;
            break;
        }

        for (int i=0; i< MAX_NODE_SIZE / 2 ; i++) {
            for (int j = 0; j< test_key_size; j++) {
                p[i][j] = rand()%max_in_small_node;
            }
            p[i][0] = i; // lsb is diffrent in all nodes
        }
        for (int i=MAX_NODE_SIZE / 2  ; i< MAX_NODE_SIZE ; i++) {
            for (int j = 0; j< test_key_size; j++) {
                p[i][j] = rand()%(256 - max_in_small_node) + max_in_small_node;
            }
            p[i][0] = i; // lsb is diffrent in all nodes
        }


        for (int i=0; i< (MAX_NODE_SIZE / 2) - 1 ; i++) {
            node_p1.Insert2BlindiNodeWithKey(&p[i][0], &p[i][0]);
            node_p2.Insert2BlindiNodeWithKey(&p[MAX_NODE_SIZE/2 + i][0], &p[MAX_NODE_SIZE/2 + i][0]);
        }
        node_p2.Insert2BlindiNodeWithKey(&p[MAX_NODE_SIZE - 1][0], &p[MAX_NODE_SIZE - 1][0]);
        node_p1.MergeBlindiNodes(&node_p2);

#ifdef DEBUG
        printf("ROUND #%d\n ", j);
        for (int i=0;  i < MAX_NODE_SIZE / 2 - 1; i++) {
            uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
            uint8_t fingerprint = 0;
            printf("Node After Merge\n ");
            position = node_p1.SearchBlindiNode(&p[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
            printf("search_key -> ");
            print_key_in_octet(&p[i][0]);
            printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
            bool eq_key_data = 0;
            CompareStage (&p[i][0], node_p1.get_data_ptr(position), &eq_key_data, &large_small);
            assert(hit == 1 & eq_key_data == 1);
            assert(position < MAX_NODE_SIZE - 1);
        }
        for (int i= MAX_NODE_SIZE / 2;  i < MAX_NODE_SIZE - 1; i++) {
            uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
            uint8_t fingerprint = 0;
            printf("Node After Merge\n ");
            position = node_p1.SearchBlindiNode(&p[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
            printf("search_key -> ");
            print_key_in_octet(&p[i][0]);
            printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
            bool eq_key_data = 0;
            CompareStage (&p[i][0], node_p1.get_data_ptr(position), &eq_key_data, &large_small);
            assert(hit == 1 & eq_key_data == 1);
            assert(position >= MAX_NODE_SIZE / 2 - 1);
        }
#endif
    }
#endif




#ifdef TEST_MERGE
    uint8_t m[MAX_NODE_SIZE + 1][KEY_SIZE_BYTE];
    // zeroes the inputs
    for (int i=0; i< MAX_NODE_SIZE; i++) {
        for (int j=0; j< KEY_SIZE_BYTE; j++) {
            m[i][j] = {0};
        }
    }
    for (int j=0; j< 1000; j++) {
        BlindiNode node_m1(MAX_NODE_SIZE);
        BlindiNode node_m2(MAX_NODE_SIZE);
        int test_key_size = 16; //4 // 16
        for (int i=0; i< MAX_NODE_SIZE / 2 ; i++) {
            for (int j = 0; j< test_key_size; j++) {
                m[i][j] = rand()%128;
            }
            m[i][0] = i; // lsb is diffrent in all nodes
        }
        for (int i=MAX_NODE_SIZE / 2 ; i< MAX_NODE_SIZE ; i++) {
            for (int j = 0; j< test_key_size; j++) {
                m[i][j] = rand()%128 + 128;
            }
            m[i][0] = i; // lsb is diffrent in all nodes
        }


        for (int i=0; i< (MAX_NODE_SIZE / 2) ; i++) {
            node_m1.Insert2BlindiNodeWithKey(&m[i][0], &m[i][0]);
            node_m2.Insert2BlindiNodeWithKey(&m[MAX_NODE_SIZE/2 + i][0], &m[MAX_NODE_SIZE/2 + i][0]);
        }
#ifdef DEBUG
        printf("ROUND #%d\n ", j);
        printf("M1 before merge");
        node_m1.node.print_node();
        printf("M2 before merge");
        node_m2.node.print_node();
#endif
        node_m1.MergeBlindiNodes(&node_m2);

#ifdef DEBUG
        printf("M1 after merge");
        node_m1.node.print_node();
        for (int i=0;  i < MAX_NODE_SIZE; i++) {
            uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
            uint8_t fingerprint = 0;
            printf("Node After Merge\n ");
            position = node_m1.SearchBlindiNode(&m[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
            printf("search_key -> ");
            print_key_in_octet(&m[i][0]);
            printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
            bool eq_key_data = 0;
            CompareStage (&m[i][0], node_m1.get_data_ptr(position), &eq_key_data, &large_small);
            assert(hit == 1 & eq_key_data == 1);
        }
#endif
    }
#endif
#ifdef TEST_MERGE_BUG2
    uint8_t b[10][4];
    // zeroes the inputs
    b[0][3] = 0;
    b[0][2] = 4;
    b[0][1] = 9;
    b[0][0] = 2;
    b[1][3] = 0;
    b[1][2] = 8;
    b[1][1] = 8;
    b[1][0] = 6;
    b[2][3] = 2;
    b[2][2] = 7;
    b[2][1] = 7;
    b[2][0] = 7;
    b[3][3] = 5;
    b[3][2] = 3;
    b[3][1] = 8;
    b[3][0] = 6;
    b[4][3] = 1;
    b[4][2] = 4;
    b[4][1] = 2;
    b[4][0] = 1;


    b[5][3] = 6;
    b[5][2] = 9;
    b[5][1] = 1;
    b[5][0] = 5;
    b[6][3] = 7;
    b[6][2] = 7;
    b[6][1] = 9;
    b[6][0] = 3;
    b[7][3] = 8;
    b[7][2] = 3;
    b[7][1] = 3;
    b[7][0] = 5;
    b[8][3] = 9;
    b[8][2] = 3;
    b[8][1] = 8;
    b[8][0] = 3;
    b[9][3] = 6;
    b[9][2] = 6;
    b[9][1] = 4;
    b[9][0] = 9;




    BlindiNode node_b1(8);
    BlindiNode node_b2(8);
    for (int i=0; i < 5 ; i++) {
        node_b1.Insert2BlindiNodeWithKey(&b[i][0], &b[i][0]);
    }
    for (int i=5; i < 10 ; i++) {
        node_b2.Insert2BlindiNodeWithKey(&b[i][0], &b[i][0]);
    }
    node_b1.RemoveFromBlindiNodeWithKey(&b[2][0]);
#ifdef DEBUG
    for (int i=0;  i < 5; i++) {
        uint8_t tree_traverse[8] = {0};
        uint8_t fingerprint = 0;
        printf("Node After Remove node 1\n ");
        position = node_b1.SearchBlindiNode(&b[i][0], 4, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        printf("search_key -> ");
        print_key_in_octet(&b[i][0]);
        printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
        bool eq_key_data = 0;
        CompareStage (&b[i][0], node_b1.get_data_ptr(position), &eq_key_data, &large_small);
        if (i == 2) {
            assert(hit == 0 & eq_key_data == 0);

        } else {
            assert(hit == 1 & eq_key_data == 1);
        }
    }
#endif

    node_b2.RemoveFromBlindiNodeWithKey(&b[7][0]);
    node_b2.RemoveFromBlindiNodeWithKey(&b[5][0]);
#ifdef DEBUG
    for (int i=5;  i < 9; i++) {
        uint8_t tree_traverse[8] = {0};
        uint8_t fingerprint = 0;
        printf("Node After Remove node 2\n ");
        position = node_b2.SearchBlindiNode(&b[i][0], 4, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        printf("search_key -> ");
        print_key_in_octet(&b[i][0]);
        printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
        bool eq_key_data = 0;
        CompareStage (&b[i][0], node_b2.get_data_ptr(position), &eq_key_data, &large_small);
        if (i == 5 || i == 7) {
            assert(hit == 0 & eq_key_data == 0);

        } else {
            assert(hit == 1 & eq_key_data == 1);
        }
    }
#endif


    node_b1.MergeBlindiNodes(&node_b2);
    printf("Node After Merge\n ");
#ifdef DEBUG
    for (int i=0;  i < 9; i++) {
        uint8_t tree_traverse[8] = {0};
        uint8_t fingerprint = 0;
        printf("Node After Merge\n ");
        position = node_b1.SearchBlindiNode(&b[i][0], 4, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        printf("search_key -> ");
        print_key_in_octet(&b[i][0]);
        printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
        bool eq_key_data = 0;
        CompareStage (&b[i][0], node_b1.get_data_ptr(position), &eq_key_data, &large_small);
        if (i == 2 || i == 5 || i == 7) {
            assert(hit == 0 & eq_key_data == 0);

        } else {
            assert(hit == 1 & eq_key_data == 1);
        }
    }

    uint8_t sorted_positions_b[7] = {0};
    uint8_t *Tb[MAX_NODE_SIZE];
    node_b1.node.bring_sorted_items(sorted_positions_b);
    for (int i=0; i< node_b1.node.get_valid_len(); i++) {
        int pos = sorted_positions_b[i];
        Tb[i] = node_b1.node.get_data_ptr(pos);
        printf("sorted iteam %d pos %d\n", i, pos);
        print_key_in_octet(&Tb[i][0]);
        printf("\n");
    }
#endif
#endif// TEST_MERGE_BUG2 

#ifdef TEST_MERGE_BUG3
    uint8_t b[10][4];
    // zeroes the inputs
    b[0][3] = 0;
    b[0][2] = 4;
    b[0][1] = 9;
    b[0][0] = 2;
    b[1][3] = 2;
    b[1][2] = 3;
    b[1][1] = 6;
    b[1][0] = 2;
    b[2][3] = 2;
    b[2][2] = 7;
    b[2][1] = 7;
    b[2][0] = 7;


    b[5][3] = 6;
    b[5][2] = 9;
    b[5][1] = 1;
    b[5][0] = 5;




    BlindiNode node_b1(8);
    BlindiNode node_b2(8);
    for (int i=0; i < 3 ; i++) {
        node_b1.Insert2BlindiNodeWithKey(&b[i][0], &b[i][0]);
    }
    for (int i=5; i < 6 ; i++) {
        node_b2.Insert2BlindiNodeWithKey(&b[i][0], &b[i][0]);
    }
#ifdef DEBUG
    for (int i=0;  i < 3; i++) {
        uint8_t tree_traverse[8] = {0};
        uint8_t fingerprint = 0;
        printf("Node After Remove node 1\n ");
        position = node_b1.SearchBlindiNode(&b[i][0], 4, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        printf("search_key -> ");
        print_key_in_octet(&b[i][0]);
        printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
        bool eq_key_data = 0;
        CompareStage (&b[i][0], node_b1.get_data_ptr(position), &eq_key_data, &large_small);
        assert(hit == 1 & eq_key_data == 1);
    }

    for (int i=5;  i < 6; i++) {
        uint8_t tree_traverse[8] = {0};
        uint8_t fingerprint = 0;
        printf("Node After Remove node 2\n ");
        position = node_b2.SearchBlindiNode(&b[i][0], 4, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        printf("search_key -> ");
        print_key_in_octet(&b[i][0]);
        printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
        bool eq_key_data = 0;
        CompareStage (&b[i][0], node_b2.get_data_ptr(position), &eq_key_data, &large_small);
        assert(hit == 1 & eq_key_data == 1);
    }


    node_b1.MergeBlindiNodes(&node_b2);

    for (int i=0;  i < 3; i++) {
        uint8_t tree_traverse[8] = {0};
        uint8_t fingerprint = 0;
        printf("Node After Remove node 1\n ");
        position = node_b1.SearchBlindiNode(&b[i][0], 4, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        printf("search_key -> ");
        print_key_in_octet(&b[i][0]);
        printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
        bool eq_key_data = 0;
        CompareStage (&b[i][0], node_b1.get_data_ptr(position), &eq_key_data, &large_small);
        assert(hit == 1 & eq_key_data == 1);
    }

    for (int i=5;  i < 6; i++) {
        uint8_t tree_traverse[8] = {0};
        uint8_t fingerprint = 0;
        printf("Node After Remove node 2\n ");
        position = node_b1.SearchBlindiNode(&b[i][0], 4, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        printf("search_key -> ");
        print_key_in_octet(&b[i][0]);
        printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
        bool eq_key_data = 0;
        CompareStage (&b[i][0], node_b1.get_data_ptr(position), &eq_key_data, &large_small);
        assert(hit == 1 & eq_key_data == 1);
    }

    uint8_t sorted_positions_b[7] = {0};
    uint8_t *Tb[MAX_NODE_SIZE];
    node_b1.node.bring_sorted_items(sorted_positions_b);
    for (int i=0; i< node_b1.node.get_valid_len(); i++) {
        int pos = sorted_positions_b[i];
        Tb[i] = node_b1.node.get_data_ptr(pos);
        printf("sorted iteam %d pos %d\n", i, pos);
        print_key_in_octet(&Tb[i][0]);
        printf("\n");
    }
#endif
#endif// TEST_MERGE_BUG2 

#ifdef TEST_MERGE_BUG
    uint8_t b[8][4];
    // zeroes the inputs
    b[0][3] = 0;
    b[0][2] = 4;
    b[0][1] = 9;
    b[0][0] = 2;
    b[1][3] = 0;
    b[1][2] = 8;
    b[1][1] = 8;
    b[1][0] = 6;
    b[2][3] = 1;
    b[2][2] = 4;
    b[2][1] = 2;
    b[2][0] = 1;
    b[3][3] = 5;
    b[3][2] = 3;
    b[3][1] = 8;
    b[3][0] = 6;
//	b[4][3] = 9; b[4][2] = 3;b[4][1] = 8;b[4][0] = 3;
//	b[5][3] = 7; b[5][2] = 7;b[5][1] = 9;b[5][0] = 3;
//	b[6][3] = 6; b[6][2] = 6;b[6][1] = 4;b[6][0] = 9;
    b[6][3] = 9;
    b[6][2] = 3;
    b[6][1] = 8;
    b[6][0] = 3;
    b[5][3] = 7;
    b[5][2] = 7;
    b[5][1] = 9;
    b[5][0] = 3;
    b[4][3] = 6;
    b[4][2] = 6;
    b[4][1] = 4;
    b[4][0] = 9;
    BlindiNode node_b1(8);
    BlindiNode node_b2(8);
    for (int i=0; i < 4 ; i++) {
        node_b1.Insert2BlindiNodeWithKey(&b[i][0], &b[i][0]);
    }
    for (int i=4; i < 7 ; i++) {
        node_b2.Insert2BlindiNodeWithKey(&b[i][0], &b[i][0]);
    }
    node_b1.MergeBlindiNodes(&node_b2);
    printf("Node After Merge\n ");
#ifdef DEBUG
    for (int i=0;  i < 7; i++) {
        uint8_t tree_traverse[8] = {0};
        uint8_t fingerprint = 0;
        printf("Node After Merge\n ");
        position = node_b1.SearchBlindiNode(&b[i][0], 4, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
        printf("search_key -> ");
        print_key_in_octet(&b[i][0]);
        printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
        bool eq_key_data = 0;
        CompareStage (&b[i][0], node_b1.get_data_ptr(position), &eq_key_data, &large_small);
        assert(hit == 1 & eq_key_data == 1);
    }


#endif
#endif// TEST_MERGE_BUG 




#ifdef TEST_MIX
    uint8_t z[MAX_NODE_SIZE + 1][KEY_SIZE_BYTE];
    // zeroes the inputs
    for (int i=0; i< MAX_NODE_SIZE; i++) {
        for (int j=0; j< KEY_SIZE_BYTE; j++) {
            z[i][j] = {0};
        }
    }
    for (int j=0; j< 1000; j++) {
        BlindiNode node_z1(MAX_NODE_SIZE);
        BlindiNode node_z2(MAX_NODE_SIZE);
        BlindiNode node_z3(MAX_NODE_SIZE);
        int test_key_size = 16; //4 // 16
        for (int i=0; i< MAX_NODE_SIZE ; i++) {
            for (int j = 0; j< test_key_size; j++) {
                z[i][j] = rand()%128;
            }
            z[i][0] = i; // lsb is diffrent in all nodes
            i = i+1;
            for (int j = 0; j< test_key_size; j++) {
                z[i][j] = rand()%128 + 128;
            }
            z[i][0] = i + 128; // lsb is diffrent in all nodes


        }
        for (int i=0; i<MAX_NODE_SIZE; i++) {
            node_z1.Insert2BlindiNodeWithKey(&z[i][0], &z[i][0]);
        }
        node_z1.SplitBlindiNode(&node_z2, &node_z3);
        node_z2.MergeBlindiNodes(&node_z3);

#ifdef DEBUG
        printf("ROUND #%d\n ", j);
        for (int i=0;  i < MAX_NODE_SIZE; i++) {
            uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
            uint8_t fingerprint = 0;
            printf("Node After MIX\n ");
            position = node_z2.SearchBlindiNode(&z[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
            printf("search_key -> ");
            print_key_in_octet(&z[i][0]);
            printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
            bool eq_key_data = 0;
            CompareStage (&z[i][0], node_z2.get_data_ptr(position), &eq_key_data, &large_small);
            assert(hit == 1 & eq_key_data == 1);
        }
#endif
        for (int i=1 ; i<MAX_NODE_SIZE; i=i+2) { // delete the large values from n2
            node_z2.RemoveFromBlindiNodeWithKey(&z[i][0]);
        }
        node_z2.MergeBlindiNodes(&node_z3);
#ifdef DEBUG
        printf("ROUND #%d\n ", j);
        for (int i=0;  i < MAX_NODE_SIZE; i++) {
            uint8_t tree_traverse[MAX_NODE_SIZE] = {0};
            uint8_t fingerprint = 0;
            printf("Node After MIX\n ");
            position = node_z2.SearchBlindiNode(&z[i][0], KEY_SIZE_BYTE, PREDECESSOR_SEARCH, &hit, &smaller_than_node, tree_traverse, &tree_traverse_len, fingerprint);
            printf("search_key -> ");
            print_key_in_octet(&z[i][0]);
            printf("-> position %d hit %d smaller_than_node %d \n ", position, hit, smaller_than_node);
            bool eq_key_data = 0;
            CompareStage (&z[i][0], node_z2.get_data_ptr(position), &eq_key_data, &large_small);
            assert(hit == 1 & eq_key_data == 1);
        }
#endif

    }




#endif// TEST MIX 



}

