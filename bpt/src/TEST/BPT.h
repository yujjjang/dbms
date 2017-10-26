#ifndef __BPT_H__
#define __BPT_H__

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#define BSZ 4096

/**
HEADER_PAGE
0~7 : FREE_PAGE
8~15 : ROOT_PAGE
16~23 : #_OF_PAGES
24~4095 : RESERVED.
**/
typedef struct header_page_f {
  off_t free_page;
  off_t root_page;
  int64_t number_of_pages;
} header_page;
/**
RECORD
0~7 : key
8~127 : vaule
**/
typedef struct record_f {
  int64_t key;
  char value[120];
} record_f;
/**
INDEX
0~7 : key
8~15 : page offset that is pointing the file including bigger key values.
**/
typedef struct index_f {
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
typedef struct leaf_node_f {
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
typedef struct internal_node_f {
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
void close_db();
//TODO::FIND_ON_GOING
//CHAR* 리턴하는 방법에대해서. 동적할당 VS PARAM 으로 넘겨줭.
header_page* getHeader();
leaf_node_f* getNewLeaf();
internal_node_f* getNewInter();
int find_i_in_leaf(int64_t key, record_f* record, int number_of_keys);
int search_leaf(int64_t key, record_f* record, int number_of_keys);
int find_i_in_index(int64_t key, index_f* index, int number_of_keys);
int search_index(int64_t key, index_f* index, int number_of_keys);
char* find(int64_t key);
//insert
off_t getNewPage();
int64_t find_V_leaf(leaf_node_f* L, int64_t key);
int64_t find_V_index(internal_node_f* I, int64_t key);
leaf_node_f* findLeaf(int64_t key, off_t* off_leaf);//Find 랑 비슷. 그냥 리프노드 찾아서 리턴해주는 역할.

int insert_in_leaf(leaf_node_f* L, int64_t key, char* value);
int insert_in_index(internal_node_f* I, int64_t key, off_t page_offset);
int64_t split_in_leaf(leaf_node_f* L, int64_t key, char* value, off_t off_parent);
int64_t split_in_index(internal_node_f* I, int64_t V, int64_t key, off_t page_offset, off_t parent_offset, off_t* off_r_I);


int insert_entry_index(internal_node_f* I,off_t I_offset,int64_t key, off_t page_offset);
int insert(int64_t key, char* value);

//delete
void deletePage(off_t off_delete);
int find_i_index_V(internal_node_f* parent, off_t off_L);
void findValue(leaf_node_f* L, off_t off_L,off_t off_parent, off_t* off_right, off_t* off_left, int64_t* key_R, int64_t* key_L);

void delete_in_leaf(leaf_node_f* L, int64_t key, off_t L_offset);
int delete(int64_t key);
















#endif
