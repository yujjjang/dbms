#ifndef __BPT_H__
#define __BPT_H__

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>

#define BLOCK_SIZE 4096
#define MAX_KEY_IN_LEAF 31
#define MAX_KEY_IN_INDEX 248
#define MIN_KEY_IN_LEAF ceil((MAX_KEY_IN_LEAF-1)/2)
#define MIN_KEY_IN_INDEX ceil(MAX_KEY_IN_INDEX) - 1
/**
HEADER_PAGE
0~7 : FREE_PAGE
8~15 : ROOT_PAGE
16~23 : #_OF_PAGES
24~4095 : RESERVED.
**/
typedef struct HeaderPage {
  off_t free_page;
  off_t root_page;
  int64_t number_of_pages;
	char hole[4072];
} HeaderPage;
/**
RECORD
0~7 : key
8~127 : vaule
**/
typedef struct Record {
  int64_t key;
  char value[120];
} Record;
/**
INDEX
0~7 : key
8~15 : page offset that is pointing the file including bigger key values.
**/
typedef struct Index {
  int64_t key;
  off_t page_offset;
} Index;
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
typedef struct LeafNode {
  off_t parent_page_offset;
  int is_leaf;
  int number_of_keys;
  char hole[104];
  off_t right_page_offset;
  Record records[31];
}LeafNode;
/**
INTERNAL NODE
0~7 : parent page offset or free page offset
8~11 : checking whether leaf or not
12~15 : number of keys. In internal node, it is smaller than 249.
16~119 : HOLE
120~127 : left most offset
128~4095 : holding indexes
**/
typedef struct InternalNode {
  off_t parent_page_offset;
  int is_leaf;
  int number_of_keys;
  char hole[104];
  off_t left_most_offset;
  Index indexes[248];
} InternalNode;

//TODO::OPEN_DB_FIN
FILE* fp;
void headerInit();
int open_db(char* pathname);
void close_db();
void writeInFile(void* data, off_t off_data);
void readFromFile(void* data, off_t off_data);
//TODO::FIND_ON_GOING
//CHAR* 리턴하는 방법에대해서. 동적할당 VS PARAM 으로 넘겨줭.
HeaderPage* getHeaderPage();
LeafNode* getNewLeafNode();
InternalNode* getNewInternalNode();
int findPositionInLeaf(int64_t key, Record* record, int number_of_keys);
int searchLeaf(int64_t key, Record* record, int number_of_keys);
int findPositionInIndex(int64_t key, Index* index, int number_of_keys);
int searchIndex(int64_t key, Index* index, int number_of_keys);
char* find(int64_t key);
//insert
off_t getNewPage();
int64_t findVPrimeInLeaf(LeafNode* L, int64_t key, int* moving_position);
int64_t findVPrimeInIndex(InternalNode* I, int64_t key, int* moving_position);
LeafNode* findLeaf(int64_t key, off_t* off_leaf);//Find 랑 비슷. 그냥 리프노드 찾아서 리턴해주는 역할.

int insertInLeaf(LeafNode* L, int64_t key, char* value);
int insertInIndex(InternalNode* I, int64_t key, off_t page_offset);
int64_t splitInLeaf(LeafNode* L, int64_t key, char* value, off_t off_parent);
int64_t splitInIndex(InternalNode* I, int64_t v_prime, int moving_position, int64_t key, off_t page_offset, off_t parent_offset, off_t* off_r_I);


int insertEntryIndex(InternalNode* I,off_t I_offset,int64_t key, off_t page_offset);
int insert(int64_t key, char* value);

//delete
void deletePage(off_t off_delete);
int find_i_index_V(InternalNode* parent, off_t off_L);
int findValue(LeafNode* L, off_t off_L,off_t off_parent, off_t* off_right, off_t* off_left, int64_t* key_R, int64_t* key_L);
int findValue_index(InternalNode* I, off_t off_I, off_t off_parent, off_t* off_right, off_t* off_left, int64_t* key_R, int64_t* key_L);
int delete_in_leaf(LeafNode* L, int64_t key, off_t L_offset);
void delete_in_index(InternalNode* I, int i);
// v_prime_index 가 가르키고 있는 값은 결국 사이값이다. 양 포인터의. 왼쪽은 그냥 , 오른쪽은 +1 해서 넘길것.
void borrow_in_left(LeafNode* L, LeafNode* left_node, off_t off_parent, int v_prime_index);
void borrow_in_right(LeafNode* L, LeafNode* right_node, off_t off_parent, int v_prime_index);
void borrow_in_left_index(InternalNode* I, InternalNode* left_node, off_t off_parent, int v_prime_index);
void borrow_in_right_index(InternalNode* I, InternalNode* right_node, off_t off_parent, int v_prime_index);
void delete_entry_index(InternalNode* I, int index, off_t off_I);
int delete(int64_t key);
















#endif
