/**
	Need to Implementation
	**/
#ifndef __BPT_H__
#define __BPT_H__

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <stdint.h>
/**
	Define some macros.
	**/
#define BLOCK_SIZE 4096
#define MAX_KEY_IN_LEAF 31
#define MAX_KEY_IN_INDEX 248
#define MIN_KEY_IN_LEAF (int)ceil((MAX_KEY_IN_LEAF-1)/2)
#define MIN_KEY_IN_INDEX (int)ceil(MAX_KEY_IN_INDEX/2) - 1
#define FREE(x) (free(x), x = NULL)
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

//open_db
FILE* fp;
HeaderPage* head;
void headerInit();
int open_db(char* pathname);
void close_db();
void writeInFile(void* data, off_t off_data);
void readFromFile(void* data, off_t off_data);
void mappingInRecord(Record* R, int start_R, Record* L, int start_L, int size);
void mappingInIndex(Index* In, int start_In, Index* I, int start_I, int size);
//find
void getHeaderPage();
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
LeafNode* findLeaf(int64_t key, off_t* off_leaf);

int insertInLeaf(LeafNode* L, int64_t key, char* value);
int insertInIndex(InternalNode* I, int64_t key, off_t page_offset);
int64_t splitInLeaf(LeafNode* L, int64_t key, char* value, off_t off_parent);
int64_t splitInIndex(InternalNode* I, int64_t v_prime, int moving_position, int64_t key, off_t page_offset, off_t parent_offset, off_t* off_r_I);

int insertEntryIndex(InternalNode* I,off_t I_offset,int64_t key, off_t page_offset);
int insert(int64_t key, char* value);

//delete
void deletePage(off_t off_delete);
int findPositionInParent(InternalNode* parent, off_t off_L);
int getVPrimeIndex(off_t off_L,off_t off_parent, off_t* off_right, off_t* off_left);
int deleteInLeaf(LeafNode* L, int64_t key, off_t L_offset);
void deleteInIndex(InternalNode* I, int i);
void borrowInLeftLeafNode(LeafNode* L, LeafNode* left_node, off_t off_parent, int v_prime_index);
void borrowInRightLeafNode(LeafNode* L, LeafNode* right_node, off_t off_parent, int v_prime_index);
void borrowInLeftInternalNode(InternalNode* I, InternalNode* left_node, off_t off_parent, int v_prime_index, off_t off_I);
void borrowInRightInternalNode(InternalNode* I, InternalNode* right_node, off_t off_parent, int v_prime_index, off_t off_I);
void deleteEntryIndex(int index, off_t off_I);
int delete(int64_t key);

#endif
