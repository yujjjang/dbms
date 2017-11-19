/**
	Need to Implementation
 **/
#ifndef __BPT_V2_H__
#define __BPT_V2_H__

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
/**
	INTERNAL NODE
	0~7 : parent page offset or free page offset
	8~11 : checking whether leaf or not
	12~15 : number of keys. In internal node, it is smaller than 249.
	16~119 : HOLE
	120~127 : left most offset
	128~4095 : holding indexes
 **/
typedef struct Node {
	off_t parent_page_offset;
	int is_leaf;
	int number_of_keys;
	char hole[104];
	union {
		off_t right_page_offset;
		off_t left_most_offset;
	};
	union {
		Record records[31];
		Index indexes[248];
	};
} Node;
// table_schema
typedef struct TableManager {
	short free[11];
	char pathname[11][120];
} TableManager;
//open_db
const char tableschema[16] = "tableschema.txt";
FILE* fp[11];
TableManager* TM;
HeaderPage* head;
void headerInit();
int open_table(char* pathname);
void close_db();
void writeInFile(void* data, off_t off_data);
void readFromFile(void* data, off_t off_data);
void mappingInRecord(Record* R, int start_R, Record* L, int start_L, int size);
void mappingInIndex(Index* In, int start_In, Index* I, int start_I, int size);
//find
void getHeaderPage();
Node* getNewLeafNode();
Node* getNewInternalNode();
int findPositionInLeaf(int64_t key, Record* record, int number_of_keys);
int searchLeaf(int64_t key, Record* record, int number_of_keys);
int findPositionInIndex(int64_t key, Index* index, int number_of_keys);
int searchIndex(int64_t key, Index* index, int number_of_keys);
char* find(int table_id, int64_t key);
//insert
off_t getNewPage();
int64_t findVPrimeInLeaf(Node* L, int64_t key, int* moving_position);
int64_t findVPrimeInIndex(Node* I, int64_t key, int* moving_position);
Node* findLeaf(int64_t key, off_t* off_leaf, int* pin_loc);

int insertInLeaf(Node* L, int64_t key, char* value);
int insertInIndex(Node* I, int64_t key, off_t page_offset);
int64_t splitInLeaf(Node* L, int64_t key, char* value, off_t off_parent);
int64_t splitInIndex(Node* I, int64_t v_prime, int moving_position, int64_t key, off_t page_offset, off_t parent_offset, off_t* off_r_I);

int insertEntryIndex(Node* I,off_t I_offset,int64_t key, off_t page_offset);
int insert(int table_id, int64_t key, char* value);

//delete
void deletePage(off_t off_delete);
int findPositionInParent(Node* parent, off_t off_L);
int getVPrimeIndex(off_t off_L,off_t off_parent, off_t* off_right, off_t* off_left);
int deleteInLeaf(Node* L, int64_t key, off_t L_offset);
void deleteInIndex(Node* I, int i);
void borrowInLeftLeafNode(Node* L, Node* left_node, off_t off_parent, int v_prime_index);
void borrowInRightLeafNode(Node* L, Node* right_node, off_t off_parent, int v_prime_index);
void borrowInLeftInternalNode(Node* I, Node* left_node, off_t off_parent, int v_prime_index, off_t off_I);
void borrowInRightInternalNode(Node* I, Node* right_node, off_t off_parent, int v_prime_index, off_t off_I);
void deleteEntryIndex(int index, off_t off_I);
int delete(int table_id, int64_t key);
Node *BufferPool;
short *dirty_slot;
int *pin_count;
int *table_id_slot;
int64_t *offset_slot;
short *free_slot;
short *ref_slot;
int g_clock;
int g_table_id;
int g_buf_num;

long long unsigned int cache_access;
long long unsigned int cache_hit;
int init_db(int buf_num);
int close_table(int table_id);
int shutdown_db(void);

void printBuffer();
void clockReplacement();
void writeInBuffer(Node* node, int64_t offset);

int readFromBuffer(int64_t offset);
#endif
