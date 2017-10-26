#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdbool.h>
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// Default order is 4.
#define DEFAULT_ORDER 4

// Minimum order is necessarily 3.  We set the maximum
// order arbitrarily.  You may change the maximum order.
#define MIN_ORDER 3
#define MAX_ORDER 20

// Constants for printing part or all of the GPL license.
#define LICENSE_FILE "LICENSE.txt"
#define LICENSE_WARRANTEE 0
#define LICENSE_WARRANTEE_START 592
#define LICENSE_WARRANTEE_END 624
#define LICENSE_CONDITIONS 1
#define LICENSE_CONDITIONS_START 70
#define LICENSE_CONDITIONS_END 625

// TYPES.

/* Type representing the record
 * to which a given key refers.
 * In a real B+ tree system, the
 * record would hold data (in a database)
 * or a file (in an operating system)
 * or some other information.
 * Users can rewrite this part of the code
 * to change the type and content
 * of the value field.
 */
typedef struct record {
    int value;
} record;

/* Type representing a node in the B+ tree.
 * This type is general enough to serve for both
 * the leaf and the internal node.
 * The heart of the node is the array
 * of keys and the array of corresponding
 * pointers.  The relation between keys
 * and pointers differs between leaves and
 * internal nodes.  In a leaf, the index
 * of each key equals the index of its corresponding
 * pointer, with a maximum of order - 1 key-pointer
 * pairs.  The last pointer points to the
 * leaf to the right (or NULL in the case
 * of the rightmost leaf).
 * In an internal node, the first pointer
 * refers to lower nodes with keys less than
 * the smallest key in the keys array.  Then,
 * with indices i starting at 0, the pointer
 * at i + 1 points to the subtree with keys
 * greater than or equal to the key in this
 * node at index i.
 * The num_keys field is used to keep
 * track of the number of valid keys.
 * In an internal node, the number of valid
 * pointers is always num_keys + 1.
 * In a leaf, the number of valid pointers
 * to data is always num_keys.  The
 * last leaf pointer points to the next leaf.
 */
typedef struct node {
    void ** pointers;
    int * keys;
    struct node * parent;
    bool is_leaf;
    int num_keys;
    struct node * next; // Used for queue.
} node;

// GLOBALS.

/* The order determines the maximum and minimum
 * number of entries (keys and pointers) in any
 * node.  Every node has at most order - 1 keys and
 * at least (roughly speaking) half that number.
 * Every leaf has as many pointers to data as keys,
 * and every internal node has one more pointer
 * to a subtree than the number of keys.
 * This global variable is initialized to the
 * default value.
 */
extern int order;

/* The queue is used to print the tree in
 * level order, starting from the root
 * printing each entire rank on a separate
 * line, finishing with the leaves.
 */
extern node * queue;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
extern bool verbose_output;


// FUNCTION PROTOTYPES.

// Output and utility.

void license_notice( void );
void print_license( int licence_part );
void usage_1( void );
void usage_2( void );
void usage_3( void );
void enqueue( node * new_node );
node * dequeue( void );
int height( node * root );
int path_to_root( node * root, node * child );
void print_leaves( node * root );
void print_tree( node * root );
void find_and_print(node * root, int key, bool verbose); 
void find_and_print_range(node * root, int range1, int range2, bool verbose); 
int find_range( node * root, int key_start, int key_end, bool verbose,
        int returned_keys[], void * returned_pointers[]); 
node * find_leaf( node * root, int key, bool verbose );
record * find( node * root, int key, bool verbose );
int cut( int length );

// Insertion.

record * make_record(int value);
node * make_node( void );
node * make_leaf( void );
int get_left_index(node * parent, node * left);
node * insert_into_leaf( node * leaf, int key, record * pointer );
node * insert_into_leaf_after_splitting(node * root, node * leaf, int key,
                                        record * pointer);
node * insert_into_node(node * root, node * parent, 
        int left_index, int key, node * right);
node * insert_into_node_after_splitting(node * root, node * parent,
                                        int left_index,
        int key, node * right);
node * insert_into_parent(node * root, node * left, int key, node * right);
node * insert_into_new_root(node * left, int key, node * right);
node * start_new_tree(int key, record * pointer);
node * insert( node * root, int key, int value );

// Deletion.

int get_neighbor_index( node * n );
node * adjust_root(node * root);
node * coalesce_nodes(node * root, node * n, node * neighbor,
                      int neighbor_index, int k_prime);
node * redistribute_nodes(node * root, node * n, node * neighbor,
                          int neighbor_index,
        int k_prime_index, int k_prime);
node * delete_entry( node * root, node * n, int key, void * pointer );
node * delete( node * root, int key );

void destroy_tree_nodes(node * root);
node * destroy_tree(node * root);

// Disk_Based_B+_Tree. 2013012096_YuJaeSun
// Free Page 관련:
// Free page 는 2개의 offset 으로 관리함.
// header page 에서 1개. header가 가르키는 page 에서 1개.
// 새로운 노드 만들어질때 그
// DEFINE SOME MACROS TO USE IT EXPLICT
#define H_F_P_O 0
#define H_R_P_O 8
#define H_N_O_P 16
#define BSZ 4096

/**
HEADER_PAGE
0~7 : FREE_PAGE
8~15 : ROOT_PAGE
16~23 : #_OF_PAGES
24~4095 : RESERVED.
**/
typedef header_page_f {
	off_t free_page;
	off_t root_page;
	int64_t number_of_pages;
} header_page;
/**
RECORD
0~7 : key
8~127 : vaule
**/
typedef record_f {
	int64_t key;
	char value[120];
} record_f;
/**
INDEX
0~7 : key
8~15 : page offset that is pointing the file including bigger key values.
**/
typedef index_f {
	int64_t key;
	off_t page_offset;
} index_f;
// is_leaf =1 when it is leaf_node.
/**
LEAF NODE
0~7 : parent_page_offset or free page offset (If it is free page)
8~11 : checking whether leaf or not
12~15 : number of keys. In Leaf node, it is smaller than 32.
16~119 : HOLE
120~127 : right sibling page offset
128 ~ 4095 : holing records.
**/
typedef leaf_node_f {
	off_t parent_page_offset;
	int is_leaf;
	int number_of_keys;
	char HOLE[104];
	off_t right_page_offset;
	record_f records[31];
}leaf_node_f;
/**
INTERNAL NODE
0~7 : parent page offset or free page offset
8~11 : checking whether leaf or not
12~15 : number of keys. In internal node, it is smaller than 249.
16~119 : HOLE
120~127 : left most offset
128~4095 : holding indexes
**/
typedef internal_node_f {
	off_t parent_page_offset;
	int is_leaf;
	int number_of_keys;
	char HOLE[104];
	off_t left_most_offset;
	index_f indexes[248];
} internal_node_f;

//TODO::OPEN_DB_FIN
FILE* fp;
void header_init();
int open_db(char* pathname);
//TODO::FIND_ON_GOING
//CHAR* 리턴하는 방법에대해서. 동적할당 VS PARAM 으로 넘겨줭.
extern char* VALUE[120];
header_page* getHeader();
leaf_node_f* getNewLeaf();
internal_node_f* getNewInter();
int find_i_in_leaf(int64_t key, record_f* record, int number_of_keys);
int search_leaf(int64_t key, record_f* record, int number_of_keys);
int find_i_in_indexes(int64_t key, index_f* index, int number_of_keys);
int search_index(int64_t key, index_f* index);
char* Find(int64_t key);
//
leaf_node_f* findLeaf(int64_t key);



int Insert(int64_t key, char* value);






int Delete(int64_t key);
#endif /* __BPT_H__*/
