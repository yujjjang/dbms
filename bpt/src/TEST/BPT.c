//2013012096_YuJaeSun
#include "BPT.h"

/**
	If we make a new file, we have to initialize our header page.
	This function will help you to do it.
	Make new header page and Write it in file.
	Set free page offset to the first page offset.
 **/
void headerInit(){
	HeaderPage* head = (HeaderPage*)malloc(sizeof(HeaderPage));
	memset(head, 0, sizeof(head));
	head -> free_page = (off_t)4096;
	head -> root_page = (off_t)0;
	head -> number_of_pages = 0;
	writeInFile(head, 0);
	free(head);
}

/**
	Before using other operation like insert, delete, find, we must call this function before.
	If the file named pathname already exists, It will open it with read & write mode.
	If not, It will make the new file named pathname and open it with write & read mode.
 **/
int open_db(char* pathname){
	if((fp = fopen(pathname, "r+")) == NULL){
		fp = fopen(pathname, "w+");
		headerInit();
	}
	if(fp == NULL) return -1;
	return 0;
}

void close_db(){
	if(fp != NULL)
		fclose(fp);
}

void writeInFile(void* data, off_t off_data){
	fseek(fp, off_data, SEEK_SET);
	fwrite(data, BLOCK_SIZE, 1, fp);
}

void readFromFile(void* data, off_t off_data){
	fseek(fp, off_data, SEEK_SET);
	fread(data, BLOCK_SIZE, 1, fp);
}

/**
	These three functions are needed when we try to get header page, new leaf node and new internal node.
 **/
HeaderPage* getHeaderPage(){
	HeaderPage* tmp = (HeaderPage*)malloc(sizeof(HeaderPage));
	readFromFile(tmp, 0);
	return tmp;
}

LeafNode* getNewLeafNode(){
	LeafNode* tmp = (LeafNode*)malloc(sizeof(LeafNode));
	memset(tmp, 0, BLOCK_SIZE);
	tmp -> parent_page_offset = 0;
	tmp -> is_leaf = 1;
	tmp -> number_of_keys = 0;
	tmp -> right_page_offset = 0;
	return tmp;
}

InternalNode* getNewInternalNode(){
	InternalNode* tmp = (InternalNode*)malloc(sizeof(InternalNode));
	memset(tmp, 0, BLOCK_SIZE);
	tmp -> parent_page_offset = 0;
	tmp -> is_leaf = 0;
	tmp -> number_of_keys = 0;
	tmp -> left_most_offset = 0;
	return tmp;
}

/**
	This function is used to find the child offset from index to find the leaf node which is including the key.
	Index consists of key and page offset like the right one. <key, page offset>
	The page offset in Index is pointing the node, keys in the node are larger than the key paired with page offset.
	
	Return : Position of the key in Parent Node which is the smallest of those that are larger than key we want to find.
 **/
int findPositionInIndex(int64_t key, Index* index, int number_of_keys){
	int start = 0;
	int end = number_of_keys-1;
	int mid = (start + end) / 2;
	while(start <= end){
		if(index[mid].key == key) return mid + 1;
		else if(index[mid].key < key) {
			start = mid + 1;
			mid = (start + end) / 2;
		}else{
			end = mid - 1;
			mid = (start + end) / 2;
		}
	}
	return start;
}

/**
	1> check out whether the key is smaller than the smallest key in index.
	2> check out whether the key is larger than the largest key in index.
	3> If not, find position by using function 'findPositionInLeaf'.

	Return : Same as 'findPositionInLeaf'.
					 In case 1> or 2>, return left most or right most position.
 **/
int searchIndex(int64_t key, Index* index, int number_of_keys){
	int i = 0;
	// Case 1>
	if(key < index[0].key) {
		return -1;
	// Case 2>
	} else if (key >= index[number_of_keys-1].key) {
		return number_of_keys;
	// Case 3>
	} else {
		i = findPositionInIndex(key, index, number_of_keys);
		return i;
	}
}

/**
	This function is used to find the key from leaf node.
	
	Return : the position of key in leaf node.
					 If we cannot find it, return -1.
 **/
int findPositionInLeaf(int64_t key, Record* record, int number_of_keys){
	int start = 0;
	int end = number_of_keys - 1;
	int mid = (start + end) / 2;
	while(start <= end){
		if(record[mid].key == key) return mid;
		else if (record[mid].key < key) {
			start = mid + 1;
			mid = (start + end) / 2;
		} else {
			end = mid - 1;
			mid = (start + end) / 2;
		}
	}
	return -1;
}

/**
	Same as function 'findPositionInLeaf'.
 **/
int searchLeaf(int64_t key, Record* record, int number_of_keys){
	return findPositionInLeaf(key, record, number_of_keys);
}
/**
	1> If it is an empty tree, return NULL. if not, keep going down.
	2> Get root page offset from file and check out whether leaf or not.
		 If it is a leaf node, search the key we want to find in the node.
	3> If the root page is not a leaf node, find the leaf node by loop.
		 When we find the leaf node, search the key we want to find in the node.
	
	Return : value in key or NULL
 **/
char* find(int64_t key){
	HeaderPage* head = getHeaderPage();
	// Case 1>
	if (head -> number_of_pages == 0){
		return NULL;
	}
	
	char* VALUE = (char*)malloc(sizeof(char)*120);
	memset(VALUE, 0, sizeof(char) * 120);

	off_t off_root = head -> root_page; // get root page offset.
	free(head);

	LeafNode* L = getNewLeafNode();
	InternalNode* I = getNewInternalNode();

	readFromFile(I, off_root);
	int index;
	off_t off_tmp;
	// Case 2>
	if (I -> is_leaf == 1){ 
		free(I);
		readFromFile(L, off_root);
		index = searchLeaf(key, L -> records, L -> number_of_keys);
		if (index == -1){ // Fail to find the key.
			free(L);
			return NULL;
		} else {          // Success to find the key.
			strcpy(VALUE, (L -> records)[index].value); 
			free(L);
			return VALUE;
		}
	// Case 3>
	} else { 
		while (I -> is_leaf != 1){
			index = searchIndex(key, I -> indexes, I -> number_of_keys);
			if (index == -1){
				off_tmp = (I -> left_most_offset);
			} else {
				off_tmp = (I -> indexes)[index-1].page_offset;
			}
			readFromFile(I,off_tmp);
		}
		free(I);
		readFromFile(L,off_tmp);

		index = searchLeaf(key, L -> records, L -> number_of_keys);
		if (index == -1){ // Fail to find the key.
			free(L); 
			return NULL;
		} else {          // Success to find the key.
			strcpy(VALUE, (L -> records)[index].value);
			free(L);
			return VALUE;
		}
		return NULL;
	}
}

/**
	First, read header page from file to get the free page offset.
	if the free page pointed by header page does not point other free page, we set the free page offset in header page to the block next to the last one.
	if not, set the free page offset in header page to the offset pointed by the page pointed by header page.
	Last, increase the number of pages in header page.

	Return : new page offset.
	
 **/
off_t getNewPage(){
	HeaderPage* head = getHeaderPage();
	off_t new_page_offset = head -> free_page;
	int64_t number_of_pages = head -> number_of_pages;

	LeafNode* tmp = getNewLeafNode();
	readFromFile(tmp, new_page_offset);
	head -> number_of_pages = head -> number_of_pages + 1;
	// There is an only free page in tree.
	if (tmp -> parent_page_offset == 0) {
		head -> free_page = BLOCK_SIZE + BLOCK_SIZE * (head -> number_of_pages);
	// There are some free pages in tree.
	} else {
		head -> free_page = tmp -> parent_page_offset;
	}
	writeInFile(head, 0);
	free(head);
	free(tmp);
	return new_page_offset;
}

/**
	Unfortunately, leaf node struct and internal node struct are different.
	So, the function is implemented by two different types.
	I will fix it as soon as possible.

	Let v prime be the value such that exactly ceil(n/2) of the values are less than v prime. n is MAX KEY IN LEAF of INDEX + 1.
	Find the v prime by branch and the moving position that is next to the v prime' s position.
	parameter 'moving_position' helps you to move the keys and pointers left node to right one.

	Return : v prime
 **/

int64_t findVPrimeInLeaf(LeafNode* L, int64_t key, int* moving_position){
	int v_prime_pos = ceil((MAX_KEY_IN_LEAF + 1) / 2);

	if (key < (L -> records)[v_prime_pos].key){
		if (key < (L -> records)[v_prime_pos].key){
			*moving_position = v_prime_pos - 1;
			return (L -> records)[v_prime_pos - 1].key;
		} else {
			*moving_position = v_prime_pos;
			return key;
		}
	} else {
		*moving_position = v_prime_pos;
		return (L -> records)[v_prime_pos].key;
	}
}

/**
	Same as function 'findVPrimeInLeaf'.
**/
int64_t findVPrimeInIndex(InternalNode* I, int64_t key, int* moving_position){
	int v_prime_pos = ceil((MAX_KEY_IN_INDEX + 1) / 2);

	if ( key < (I -> indexes)[v_prime_pos].key){
		if (key < (I -> indexes)[v_prime_pos - 1].key){
			*moving_position = v_prime_pos;
			return (I -> indexes)[v_prime_pos - 1].key;
		} else {
			*moving_position = v_prime_pos;
			return key;
		}
	} else {
		*moving_position = v_prime_pos + 1;
		return (I -> indexes)[v_prime_pos].key;
	}
}

/**
	1> If it is an empty tree, return NULL. if not keep going down.
	2> Get root page offset from file and check out whether leaf or not.
	   If it is a leaf node, return the leaf node.
	3> If the root node is not a leaf, find the leaf node by loop.
		 When we find the leaf node, return the leaf node.

	Return : Leaf Node Pointer that can include the key.
 **/
LeafNode* findLeaf(int64_t key, off_t* off_leaf){
	HeaderPage* head = getHeaderPage();
	// Case 1>
	if (head -> number_of_pages == 0){
		free(head);
		return NULL;
	}
	
	off_t off_root = head -> root_page;
	free(head);

	LeafNode* L = getNewLeafNode();
	InternalNode* I = getNewInternalNode();
	readFromFile(I, off_root);

	int is_leaf;
	off_t off_tmp;
	int index;
	// Case 2>
	if (I -> is_leaf == 1){ 
		free(I);
		readFromFile(L, off_root);
		*off_leaf = off_root;
		return L;
	// Case 3>
	} else {
		while (I -> is_leaf != 1){
			index = searchIndex(key ,I -> indexes, I -> number_of_keys);
			if (index == -1){
				off_tmp = (I -> left_most_offset);
			} else {
				off_tmp = (I -> indexes)[index-1].page_offset;
			}
			readFromFile(I, off_tmp);
		}
		free(I);
		readFromFile(L, off_tmp);
		*off_leaf = off_tmp;
		return L;
	}
	return NULL;
}
/**
	Unfortunately, leaf node struct and internal node struct are different.
	So, the function is implemented by two different types.
	I will fix it as soon as possible.


	1> If it is an empty node, put the key and value on the first position.
	2> If the key is smaller than the smallest in node, put the key and value on the first position, by shifting other keys and values right.
	3> If the key is larger than the largest in node, put the key and value on the last position.
	4> If not, find the position and put it in there, by shifting keys and values on the right side right.

	Return : 0.
 **/
int insertInLeaf(LeafNode* L, int64_t key, char* value){
	int number_of_keys = L -> number_of_keys;
	// Case 1>
	if (L -> number_of_keys == 0){
		(L -> records)[0].key = key;
		strcpy((L -> records)[0].value, value);
		(L -> number_of_keys)++;
		return 0;
	}
	
	int start = 0;
	int end = number_of_keys - 1;
	int mid = (start + end) / 2;
	Record tmp_records[31];
	memset(tmp_records,0,sizeof(tmp_records));
	// Case 2>
	if (key < (L -> records)[0].key){
		memcpy(tmp_records, L -> records, sizeof(Record) * number_of_keys);
		(L -> records)[0].key = key;
		strcpy((L -> records)[0].value, value);
		memcpy((L -> records) + 1, tmp_records, sizeof(Record) * number_of_keys);
		(L -> number_of_keys)++;
		return 0;
	// Case 3>
	} else if (key > (L -> records)[end].key){
		(L -> records)[number_of_keys].key = key;
		strcpy((L -> records)[number_of_keys].value, value);
		(L -> number_of_keys)++;
		return 0;
	// Case 4>
	} else {
		while(start <= end){
			if ((L -> records)[mid].key > key){
				end = mid - 1;
				mid = (start + end) / 2;
			} else {
				start = mid + 1;
				mid = (start + end) / 2;
			}
		}
	}
	
	int number_of_copy = number_of_keys - start;
	memcpy(tmp_records, (L -> records) + start, sizeof(Record) * number_of_copy);
	(L -> records)[start].key = key;
	strcpy((L -> records)[start].value, value);
	memcpy((L->records) + start + 1, tmp_records, sizeof(Record) * number_of_copy);
	(L -> number_of_keys)++;
	return 0;
}
/**
	Same as function 'insertInLeaf'.
**/
int insertInIndex(InternalNode* I, int64_t key, off_t page_offset){
	int number_of_keys = I -> number_of_keys;
	// Case 1>
	if (number_of_keys == 0){
		(I -> indexes)[0].key = key;
		(I -> indexes)[0].page_offset = page_offset;
		(I -> number_of_keys)++;
		return 0;
	}

	int start = 0;
	int end = number_of_keys - 1;
	int mid = (start + end) / 2;
	Index tmp_indexes[248];
	memset(tmp_indexes, 0, sizeof(tmp_indexes));
	// Case 2>
	if (key < (I -> indexes)[0].key){
		memcpy(tmp_indexes, I -> indexes, sizeof(Index) * number_of_keys);
		(I -> indexes)[0].key = key;
		(I -> indexes)[0].page_offset = page_offset;
		memcpy((I -> indexes) + 1, tmp_indexes, sizeof(Index) * number_of_keys);
		(I -> number_of_keys)++;
		return 0;
	// Case 3>
	} else if (key > (I -> indexes)[end].key){
		(I -> indexes)[number_of_keys].key = key;
		(I -> indexes)[number_of_keys].page_offset = page_offset;
		(I -> number_of_keys)++;
		return 0;
	// Case 4>
	} else {
		while (start <= end){
			if ((I -> indexes)[mid].key > key){
				end = mid - 1;
				mid = (start + end) / 2;
			} else {
				start = mid + 1;
				mid = (start + end) / 2;
			}
		}
	}

	int number_of_copy = number_of_keys - start;
	memcpy(tmp_indexes, (I -> indexes) + start, sizeof(Index) * number_of_copy);
	(I -> indexes)[start].key = key;
	(I -> indexes)[start].page_offset = page_offset;
	memcpy((I -> indexes) + start + 1, tmp_indexes, sizeof(Index) * number_of_copy);
	(I -> number_of_keys)++;
	return 0;
}
/**
	This function is used to split the leaf node because of insertion overflow.
	
	1> Make the new leaf node and set the right page offset and parent page offset.
	2> Set the right page offset and parent page offset in leaf node named 'L'.
	3> Find the v prime and moving position by using function 'findVPrimeInLeaf', then move the keys and values in 'L' to new node.
	4> Write the data of right leaf node in file.

	Return : v prime.
 **/
int64_t splitInLeaf(LeafNode* L, int64_t key, char* value, off_t off_parent){
	// Case 1>
	off_t new_page = getNewPage(); 
	LeafNode* right_leaf_node = getNewLeafNode();
	right_leaf_node -> right_page_offset = L -> right_page_offset;
	right_leaf_node -> parent_page_offset = L -> parent_page_offset;
	
	// Case 2>
	L -> right_page_offset = new_page;
	L -> parent_page_offset = off_parent;
	
	// Case 3>
	int moving_position = 0;
	int64_t v_prime = findVPrimeInLeaf(L, key, &moving_position);
	memcpy(right_leaf_node -> records, (L -> records) + moving_position, sizeof(Record) * (MAX_KEY_IN_LEAF - moving_position));
	memset((L -> records) + moving_position, 0, sizeof(Record) * (MAX_KEY_IN_LEAF - moving_position));
	right_leaf_node -> number_of_keys = MAX_KEY_IN_LEAF - moving_position;
	L -> number_of_keys = moving_position;
	
	if (key < v_prime) {
		insertInLeaf(L, key, value);
	} else {
		insertInLeaf(right_leaf_node, key, value);
	}

	// Case 4>
	writeInFile(right_leaf_node, new_page);
	free(right_leaf_node);

	return v_prime;
}

/**
	This function is used to split the internal node because of insertion overflow.

	1> Make the new internal node and set the left-most page offset and parent page offset.
	2> Move the keys and page offsets in 'I' to new node.
	3> Change the child nodes' parent page offset, the child node of new node. 

	Return : v prime.
 **/
int64_t splitInIndex(InternalNode* I, int64_t v_prime, int moving_position, int64_t key, off_t page_offset, off_t parent_offset, off_t* off_r_I){
	// Case 1>
	off_t off_new = getNewPage();
	InternalNode* right_internal_node = getNewInternalNode();
	right_internal_node -> parent_page_offset = parent_offset;
	*off_r_I = off_new;

	if (key == v_prime){
		right_internal_node -> left_most_offset = page_offset;
	} else {
		right_internal_node -> left_most_offset = (I -> indexes)[moving_position - 1].page_offset;
	}
	
	// Case 2>
	memcpy(right_internal_node -> indexes, (I -> indexes) + moving_position, sizeof(Index) * (MAX_KEY_IN_INDEX - moving_position));
	memset((I -> indexes) + moving_position, 0, sizeof(Index) * (MAX_KEY_IN_INDEX - moving_position));	
	I -> number_of_keys = moving_position;
	right_internal_node -> number_of_keys = MAX_KEY_IN_INDEX - moving_position;
	
	if (key < v_prime) {
		insertInIndex(I, key, page_offset);
	} else if (key > v_prime) {
		insertInIndex(right_internal_node, key, page_offset);
	}
	writeInFile(right_internal_node, off_new);

	// Case 3>
	InternalNode* child = getNewInternalNode();
	// 1. Change the child pointed by left-most offset.
	off_t off_child = right_internal_node -> left_most_offset;
	readFromFile(child, off_child);
	child -> parent_page_offset = off_new;
	writeInFile(child, off_child);
	// 2. Change other child nodes.
	for (int i = 0; i < right_internal_node -> number_of_keys; ++i) {
		off_child = (right_internal_node -> indexes)[i].page_offset;
		readFromFile(child, off_child);
		child -> parent_page_offset = off_new;
		writeInFile(child, off_child);
	}

	free(right_internal_node);
	return v_prime;
}

//Leaf insert 에서 split 상황에 현재의 L 이 부모가 있을때 이제 그 부모를 나눌때의 함수를 재귀로 구현.
int insertEntryIndex(InternalNode* I,off_t off_I, int64_t key, off_t page_offset){
	off_t off_parent = I -> parent_page_offset;
	off_t off_right_internal_node = 0;
	if(I -> number_of_keys < MAX_KEY_IN_INDEX) { // 나눌필요가 없을떄 넣고 쓰고 끝냄.
		insertInIndex(I ,key ,page_offset);
		writeInFile(I, off_I);
		return 0;
	} else { //mSPLIT 상황 //TODO:: 10/26 에러.
		if (I -> parent_page_offset == 0){
			off_parent = getNewPage();
		}

		int moving_position = 0;
		int64_t v_prime = findVPrimeInIndex(I,key,&moving_position);
		splitInIndex(I, v_prime, moving_position, key, page_offset, off_parent, &off_right_internal_node);

		if (I->parent_page_offset == 0){ // 루트 일때는 부모노드 만들어서 V` 값 올리고 끝 !.
			I -> parent_page_offset = off_parent;
			InternalNode* root_node = getNewInternalNode();
			HeaderPage* head = getHeaderPage();
			head -> root_page = off_parent;
			writeInFile(head, 0);
			
			root_node -> left_most_offset = off_I;
			insertInIndex(root_node, v_prime, off_right_internal_node);
			
			writeInFile(root_node, off_parent);
			writeInFile(I, off_I);
			
			free(head);
			free(root_node);
			free(I);
			return 0;

		}else { // 루트아닐때는 V` 부모에 삽입 재귀적으로 구현.
			InternalNode* parent = getNewInternalNode();
			readFromFile(parent, off_parent);
			writeInFile(I, off_I);
			insertEntryIndex(parent, off_parent, v_prime, off_right_internal_node);
			
			free(parent);
			free(I);
			return 0;
		}
	}
	return 0;
}

/**
	This function is used to insert key and value.
	1> If it is an empty tree, make the new leaf node and set it root. And put the key and value on it.
	2> If the key already exists, exit the function.
	3> No need to split after insert the key and value.
	4> Case that 'L' is the root node. Make the new root node.
		 Set the root node's left-most page offset and put the v prime and right node page offset.
	5> Case that 'L' is not the root node. So we have to put the v prime and right node page offset to parent of 'L'.
		 Then we do it again by using recursive function named 'InsertEntryIndex' until we reach to root node.

	Return : 0 if success, -1 if fail to insert.
 **/
int insert(int64_t key, char* value){
	
	off_t off_L = 0;
	LeafNode* L = findLeaf(key, &off_L);

	InternalNode* parent = getNewInternalNode();
	
	//Case 1>
	if(L == NULL) {
		off_t off_new = getNewPage();
		HeaderPage* head = getHeaderPage();
		head -> root_page = off_new;
		free(L);
		L = getNewLeafNode();
		insertInLeaf(L,key,value);
		
		writeInFile(L, off_new);
		writeInFile(head, 0);
		free(L);
		free(parent);
		free(head);
		return 0;
	}

	// Case 2>
	if (findPositionInLeaf(key, L -> records, L -> number_of_keys) != -1) {
		return -1;
	}
	
	off_t off_parent = L -> parent_page_offset;
	off_t off_right = 0;
	int64_t v_prime = 0;
	
	// Case 3>
	if (L -> number_of_keys < MAX_KEY_IN_LEAF){
		insertInLeaf(L, key, value);
		writeInFile(L, off_L);
		free(L);
		free(parent);
	} else {
		// Case 4>
		if(L -> parent_page_offset == 0) { 
			off_parent = getNewPage();
			L -> parent_page_offset = off_parent;

			v_prime = splitInLeaf(L, key, value, off_parent);
			off_right = L -> right_page_offset;
			
			parent -> left_most_offset =  off_L;
			insertInIndex(parent, v_prime, off_right);
			
			HeaderPage* head = getHeaderPage();	
			head -> root_page = off_parent;
			writeInFile(head, 0);
			writeInFile(parent, off_parent);
			writeInFile(L, off_L);
			free(parent);
			free(L);
			free(head);
		// Case 5>
		} else { 
			v_prime = splitInLeaf(L, key, value, off_parent);
			off_right = L -> right_page_offset;
			
			writeInFile(L, off_L);
			readFromFile(parent, L -> parent_page_offset);
			
			insertEntryIndex(parent, L -> parent_page_offset, v_prime, off_right);
			
			free(L);
			//free(head);
		}
	}
	return 0;
}

// DELETION 시작 지점.
// TODO::: 10/27.
/**
	페이지 삭제.
	페이지 개수가 0이라면 그냥 초기화 시키듯 해버려줌.
	그거 아니라면 해드가 가르키던걸 tmp 에 넣고 해드가 tmp를 가르키게함.
	전체 페이지 개수 1개 줄임. 
	그리고 지워지는 녀석하고 해드 파일에다가 씀!!.
	head 전역으로 선언해서 하는게 더 편하겠네ㅔㅔㅔㅔㅔㅔㅔㅔㅔㅔㅔㅔㅔㅔㅔㅔ
 **/
void deletePage(off_t off_delete){
	HeaderPage* head = getHeaderPage();
	LeafNode* tmp = getNewLeafNode();
	tmp->parent_page_offset = head -> free_page;
	head -> free_page = off_delete;
	head -> number_of_pages = head->number_of_pages-1;
	if (head -> number_of_pages == 0){
		head -> root_page = 0;
		head -> free_page = 4096;
	}
	fseek(fp,0,SEEK_SET);
	fwrite(head, sizeof(HeaderPage), 1, fp);
	fseek(fp, off_delete, SEEK_SET);
	fwrite(tmp, BLOCK_SIZE, 1, fp);
	free(head);
	free(tmp);
}

int find_i_index_V(InternalNode* parent, off_t off_L){
	Index tmp[248];
	int number_of_keys = parent -> number_of_keys;
	memset(tmp, 0, sizeof(tmp));
	memcpy(tmp, parent -> indexes, sizeof(Index) * number_of_keys);
	// 아 이거는 페이지 뒤죽박죽이라 순회밖에 안되네 
	for (int i = 0; i < number_of_keys; ++i){
		if(off_L == tmp[i].page_offset) return i;
	}
	return -1; // 나를 가르키던게 상위 페이지의 left_most 일때.
}

int findValue(LeafNode* L,off_t off_L, off_t off_parent, off_t* off_right, off_t* off_left, int64_t* key_R, int64_t* key_L){
	InternalNode* parent = getNewInternalNode();
	fseek(fp, off_parent, SEEK_SET);
	fread(parent, BLOCK_SIZE, 1, fp);
	int i = find_i_index_V(parent, off_L);
	if (i==-1){
		*off_right = (parent->indexes)[0].page_offset;
		*key_R = (parent->indexes)[0].key;
	} else if(i==0){
		*off_left = parent -> left_most_offset;
		*key_L = (parent->indexes)[0].key;
		*off_right = (parent->indexes)[1].page_offset;
		*key_R = (parent->indexes)[1].key;
	} else if (i == L->number_of_keys - 1){
		*off_left = (parent->indexes)[i-1].page_offset;
		*key_L = (parent->indexes)[i].key;
	} else {
		*off_left = (parent -> indexes)[i-1].page_offset;
		*key_L = (parent->indexes)[i].key;
		*off_right = (parent->indexes)[i+1].page_offset;
		*key_R = (parent->indexes)[i].key;
	}
	free(parent);
	return i;
}

int findValue_index(InternalNode* I, off_t off_I, off_t off_parent, off_t* off_right, off_t* off_left, int64_t* key_R, int64_t* key_L){
	InternalNode* parent = getNewInternalNode();
	fseek(fp, off_parent, SEEK_SET);
	fread(parent,BLOCK_SIZE,1,fp);
	int i = find_i_index_V(parent, off_I);
	if (i == -1){
		*off_right = (parent->indexes)[0].page_offset;
		*key_R = (parent->indexes)[0].key;
	} else if(i==0){
		*off_left = parent -> left_most_offset;
		*key_L = (parent->indexes)[0].key;
		*off_right = (parent->indexes)[1].page_offset;
		*key_R = (parent->indexes)[1].key;
	} else if (i == I->number_of_keys - 1){
		*off_left = (parent->indexes)[i-1].page_offset;
		*key_L = (parent->indexes)[i].key;
	} else {
		*off_left = (parent -> indexes)[i-1].page_offset;
		*key_L = (parent->indexes)[i].key;
		*off_right = (parent->indexes)[i+1].page_offset;
		*key_R = (parent->indexes)[i].key;
	}
	free(parent);
	return i;
}

/**
	리프노드에서 그냥 지워주기만 할때 병합이나 재분배 필요없이 
	지우면서 종료시켜준다. 기록은 밖에서함.
	못찾을 경우 -1 리턴. 잘지우면 0 리턴.
 **/
int delete_in_leaf(LeafNode* L, int64_t key, off_t L_offset){
	int i = searchLeaf(key, L->records, L->number_of_keys);
	if(i==-1) return -1;
	int number_of_copy = L->number_of_keys - i - 1;
	Record tmp_records[31];
	memset(tmp_records,0,sizeof(tmp_records));

	memcpy(tmp_records, (L->records)+i+1, sizeof(Record) * number_of_copy);
	memset((L->records)+i,0,sizeof(Record) * (number_of_copy+1));
	memcpy((L->records)+i, tmp_records, sizeof(Record) * number_of_copy);
	L->number_of_keys = L->number_of_keys-1;
	return 0;
}

/**
	인덱스에서 일단 지우고 봄.
 **/
void delete_in_index(InternalNode* I, int i){
	Index tmp_index[248];
	memset(tmp_index,0,sizeof(tmp_index));
	int number_of_keys = I->number_of_keys;
	memcpy(tmp_index, (I->indexes)+i+1, sizeof(Index) * (number_of_keys - i-1));
	memcpy((I->indexes)+i, tmp_index, sizeof(Index) * (number_of_keys-i-1));
	memset((I->indexes)+number_of_keys-2,0,sizeof(Index));
	(I->number_of_keys)--;
}
// parent 만 쓰고 나머지는 본래 함수에서 write.
void borrow_in_left(LeafNode* L, LeafNode* left_node, off_t off_parent, int v_prime_index){
	InternalNode* parent = getNewInternalNode();
	fseek(fp,off_parent,SEEK_SET);
	fread(parent,BLOCK_SIZE,1,fp);

	int number_of_keys = left_node -> number_of_keys;
	//left_node 에서 마지막 Key, VAlue 지우고 L 로 옴기기 !
	Record tmp_record[31];
	memset(tmp_record, 0, sizeof(tmp_record));
	tmp_record[0].key = (left_node->records)[number_of_keys-1].key;
	strcpy(tmp_record[0].value, (left_node->records)[number_of_keys-1].value);
	memset((left_node->records)+number_of_keys-1,0,sizeof(Record) * (31-number_of_keys+1)); // left 는 끝났어
	number_of_keys = L->number_of_keys;
	memcpy(tmp_record + 1, L->records, sizeof(Record) * number_of_keys);
	memcpy(L->records, tmp_record, sizeof(Record) *(number_of_keys+1));
	// L 도 세팅 완료상태야.
	(parent->indexes)[v_prime_index].key = tmp_record[0].key;
	(left_node->number_of_keys)--;
	(L->number_of_keys)++;
	fseek(fp,off_parent,SEEK_SET);
	fwrite(parent,BLOCK_SIZE,1,fp);
	free(parent);
}

void borrow_in_right(LeafNode* L, LeafNode* right_node, off_t off_parent, int v_prime_index){
	InternalNode* parent = getNewInternalNode();
	fseek(fp,off_parent,SEEK_SET);
	fread(parent,BLOCK_SIZE,1,fp);
	int number_of_keys = right_node->number_of_keys;
	Record tmp_record[31];
	memset(tmp_record, 0, sizeof(tmp_record));
	memcpy(tmp_record, right_node->records, sizeof(Record) * number_of_keys);
	memset(right_node->records,0, sizeof(Record) * number_of_keys);
	memcpy(right_node->records, tmp_record + 1, sizeof(Record) * (number_of_keys-1));
	number_of_keys = L -> number_of_keys;
	(L->records)[number_of_keys].key =  tmp_record[0].key;
	strcpy((L->records)[number_of_keys].value, tmp_record[0].value);
	(parent->indexes)[v_prime_index].key = tmp_record[1].key;
	(right_node->number_of_keys)--;
	(L->number_of_keys)++;
	fseek(fp,off_parent,SEEK_SET);
	fwrite(parent,BLOCK_SIZE,1,fp);
	free(parent);
}

void borrow_in_left_index(InternalNode* I, InternalNode* left_node, off_t off_parent, int v_prime_index){
	InternalNode* parent = getNewInternalNode();
	fseek(fp,off_parent,SEEK_SET);
	fread(parent,BLOCK_SIZE,1,fp);
	int number_of_keys = left_node -> number_of_keys;
	int64_t tmp_key=(left_node->indexes)[number_of_keys-1].key;
	// Left Node 에서 마지막 값
	Index tmp_index[MAX_KEY_IN_INDEX];
	memset(tmp_index, 0, sizeof(tmp_index));
	tmp_index[0].key = (parent->indexes)[v_prime_index].key;
	tmp_index[0].page_offset = I->left_most_offset;
	I->left_most_offset = (left_node->indexes)[number_of_keys-1].page_offset;
	memset((left_node->indexes)+number_of_keys-1, 0, sizeof(Index));
	number_of_keys = I -> number_of_keys;
	memcpy(tmp_index+1, I->indexes, sizeof(Index) * number_of_keys);

	(I->number_of_keys)++;
	memcpy(I->indexes, tmp_index, sizeof(Index) * number_of_keys+1);
	(left_node->number_of_keys)--;
	// 여기까지 왼쪽노드, 자기노드 완료. 부모 V_prime_index 에 있는 값 바꿔서 저장.

	(parent->indexes)[v_prime_index].key = tmp_key;
	fseek(fp,off_parent,SEEK_SET);
	fwrite(parent,BLOCK_SIZE,1,fp);
	free(parent);
}

void borrow_in_right_index(InternalNode* I, InternalNode* right_node, off_t off_parent, int v_prime_index){
	InternalNode* parent = getNewInternalNode();
	fseek(fp,off_parent,SEEK_SET);
	fread(parent,BLOCK_SIZE,1,fp);
	int number_of_keys = right_node->number_of_keys;
	int64_t tmp_key = (right_node->indexes)[0].key;
	off_t tmp_offset = (right_node->left_most_offset);
	Record tmp_index[MAX_KEY_IN_INDEX];
	memset(tmp_index, 0, sizeof(tmp_index));
	right_node->left_most_offset = (right_node->indexes)[0].page_offset;
	memcpy(tmp_index, (right_node->indexes)+1, sizeof(Index) * (number_of_keys-1));
	memset(right_node->indexes,0, sizeof(Record) * number_of_keys);
	memcpy(right_node->indexes, tmp_index, sizeof(Index) * (number_of_keys-1));
	number_of_keys = I -> number_of_keys;
	(I->indexes)[number_of_keys].key =  (parent->indexes)[v_prime_index].key;
	(I->indexes)[number_of_keys].page_offset = tmp_offset;
	(parent->indexes)[v_prime_index].key = tmp_key;
	(right_node->number_of_keys)--;
	(I->number_of_keys)++;
	fseek(fp,off_parent,SEEK_SET);
	fwrite(parent,BLOCK_SIZE,1,fp);
	free(parent);
}


/**
	이함수는 합병되어서 부모함수 지울떄!!
	부모에서 지워야할 인덱스가 있는 곳을 가르키고 있어 index 는 .
 **/
void delete_entry_index(InternalNode* I, int index, off_t off_I){
	delete_in_index(I, index); // I 내부노드에서 지웠어 !!
	off_t off_parent = I->parent_page_offset;
	off_t off_right=0;
	off_t off_left=0;
	int64_t key_R=0;
	int64_t key_L=0;
	int i = findValue_index(I,off_I,off_parent,&off_right,&off_left,&key_R,&key_L);
	InternalNode* left_node=getNewInternalNode(); // TODO
	InternalNode* right_node=getNewInternalNode(); // TODO
	InternalNode* parent=getNewInternalNode(); // TODO
	if(off_left != 0){
		//left_node = getNewInternalNode();
		fseek(fp, off_left, SEEK_SET);
		fwrite(left_node,BLOCK_SIZE,1,fp);
	}
	if(off_right != 0){
		//right_node = getNewInternalNode();
		fseek(fp, off_right, SEEK_SET);
		fwrite(right_node,BLOCK_SIZE,1,fp);
	}
	if(I->parent_page_offset == 0 && I->number_of_keys == 0){ // 이경우는 그 left_most 만 있고 암것도 없을떄인데 루트
		HeaderPage* head = getHeaderPage();
		head -> root_page = I->left_most_offset;
		LeafNode* new_root = getNewLeafNode();
		fseek(fp,I->left_most_offset,SEEK_SET);
		fread(new_root, BLOCK_SIZE, 1, fp);
		new_root -> parent_page_offset = 0;

		fseek(fp,I->left_most_offset,SEEK_SET);
		fwrite(new_root,BLOCK_SIZE,1,fp);
		fseek(fp,0,SEEK_SET);
		deletePage(off_I);
		fwrite(head,sizeof(HeaderPage),1,fp);
		free(head);
		free(new_root);
		//deletePage(off_I);
	} else if (I->number_of_keys < ceil(MAX_KEY_IN_INDEX/2)-1){ // SPLIT !!
		if(off_left != 0 && left_node->number_of_keys > ceil(MAX_KEY_IN_INDEX/2)-1) { //왼쪽에서 빌려오는 경우 NOT IN LEAF
			borrow_in_left_index(I, left_node, off_parent, i);
			fseek(fp,off_left,SEEK_SET);
			fwrite(left_node,BLOCK_SIZE,1,fp);
			fseek(fp,off_I,SEEK_SET);
			fwrite(I,BLOCK_SIZE,1,fp);
		} else if (off_right != 0 && right_node->number_of_keys > ceil(MAX_KEY_IN_INDEX/2)-1){
			borrow_in_right_index(I, right_node, off_parent, i+1);
			fseek(fp,off_right,SEEK_SET);
			fwrite(right_node,BLOCK_SIZE,1,fp);
			fseek(fp,off_I,SEEK_SET);
			fwrite(I,BLOCK_SIZE,1,fp);
		} else { //  합병하고 재귀함수로 이 함수 호출. 합병은 언제나 왼쪽이랑 하는걸 가정하고 함. 귀찮음.
		// 합병은 딱 왼쪽이나 오른쪽이 정확히 절반일때만 합병이 가능한건데..? 후??
			//parent = getNewInternalNode();
			if(off_left != 0 && left_node -> number_of_keys == ceil(MAX_KEY_IN_INDEX/2)-1){ // 왼쪽일때
				(left_node->indexes)[left_node->number_of_keys].key = (parent->indexes)[index].key;
				(left_node->indexes)[left_node->number_of_keys].page_offset = I -> left_most_offset;
				memcpy((left_node->indexes)+left_node->number_of_keys+1, I->indexes, sizeof(Index) * (I->number_of_keys));
				left_node -> number_of_keys = left_node->number_of_keys + I->number_of_keys + 1;

				fseek(fp, off_parent, SEEK_SET);
				fread(parent, BLOCK_SIZE, 1, fp);
				delete_entry_index(parent, i, off_parent);
				deletePage(off_I);
			} else { // 오른쪽일때
				(I->indexes)[I->number_of_keys].key = (parent->indexes)[index+1].key;
				(I->indexes)[I->number_of_keys].page_offset = right_node -> left_most_offset;
				memcpy((I->indexes)+I->number_of_keys+1, right_node->indexes, sizeof(Index)*(right_node->number_of_keys));
				I -> number_of_keys = I->number_of_keys + right_node->number_of_keys+1;

				fseek(fp, off_parent, SEEK_SET);
				fread(parent, BLOCK_SIZE, 1, fp);
				delete_entry_index(parent, i+1, off_parent);
				deletePage(off_right);
			}
		}
	} else { // NOT to do
	}
	free(left_node);
	free(right_node);
	free(parent);
	//if(I != NULL) free(I);
}

// DELETE 
int delete(int64_t key){
	off_t L_offset = 0;
	LeafNode* L = findLeaf(key, &L_offset);
	InternalNode* I = getNewInternalNode();
	//if (delete_in_leaf(L,key,L_offset) == -1) return -1;
	if(L == NULL) { // 빈노드일때
		free(L);
		return -1;
	}
	if (findPositionInLeaf(key, L->records, L->number_of_keys) == -1) { // 키값이 없을때.
		free(L);
		return -1;
	}
	off_t off_parent = L -> parent_page_offset;
	if (off_parent == 0) { // 지금 가져온 리프가 루트일때. 루트는 그냥 다 지워도 상관없어.
		if(delete_in_leaf(L,key,L_offset) == -1) return -1;
		if(L->number_of_keys == 0) { // 다 지워져서 없어져서 빈 페이지가 되는 경우
			deletePage(L_offset);
		} else {
			fseek(fp, L_offset, SEEK_SET);
			fwrite(L,BLOCK_SIZE,1,fp);
		}
		free(L);
		return 0;
	}
	off_t off_right=0;
	off_t off_left=0;
	int64_t key_R = 0; // 이거 부모 양쪾포인터 사이에있는거.
	int64_t key_L = 0;
	int i = findValue(L,L_offset, L->parent_page_offset, &off_right, &off_left, &key_R, &key_L);
	LeafNode* left_node=getNewLeafNode();
	LeafNode* right_node=getNewLeafNode();
	if (off_left != 0){
		//left_node = getNewLeafNode();
		fseek(fp, off_left, SEEK_SET);
		fread(left_node, BLOCK_SIZE, 1, fp);
	}
	if(off_right != 0){
		//right_node = getNewLeafNode();
		fseek(fp, off_right, SEEK_SET);
		fread(right_node, BLOCK_SIZE, 1, fp);
	}
	// 왼쪽 녀석 오른쪽 녀석( 오른쪽은 없을수도 있어서 이래함.
	if (delete_in_leaf(L, key, L_offset) == -1) return -1; // DELETE !!
	
	if ((L->number_of_keys) >= ceil((MAX_KEY_IN_LEAF-1)/2)){ //No need to merge or redistribute. JUST DELETE !!.
		fseek(fp,L_offset,SEEK_SET);
		fwrite(L,BLOCK_SIZE,1,fp);
	} else { // 병합하거나 재분배해야하는 경우 까다로움. 그 재분배부터.
		if(off_left != 0 && left_node -> number_of_keys > ceil((MAX_KEY_IN_LEAF-1)/2)){ // 15개 보다 커야 빌려올수 있음
			borrow_in_left(L, left_node, off_parent,i);
			fseek(fp,off_left,SEEK_SET);
			fwrite(left_node, BLOCK_SIZE,1, fp);
			fseek(fp,L_offset,SEEK_SET);
			fwrite(L,BLOCK_SIZE,1,fp);
		} else if (off_right != 0 && right_node -> number_of_keys > ceil((MAX_KEY_IN_LEAF-1)/2)) { // right node 
			borrow_in_right(L, right_node, off_parent,i+1);
			fseek(fp,off_right,SEEK_SET);
			fwrite(right_node,BLOCK_SIZE,1,fp);
			fseek(fp,L_offset,SEEK_SET);
			fwrite(L,BLOCK_SIZE,1,fp);
		} else { // MERGE : 포인터 변수만 서로 바꿔준다의 의미가..?
			//I = getNewInternalNode();
			if(off_left != 0 && left_node -> number_of_keys == ceil((MAX_KEY_IN_LEAF-1)/2)+1){
				int number_of_keys = L->number_of_keys;
				memcpy((left_node->records)+(left_node->number_of_keys), L->records, sizeof(Record) * number_of_keys);
				left_node->right_page_offset = L -> right_page_offset;
				left_node->number_of_keys = left_node->number_of_keys + L->number_of_keys;
				
				fseek(fp,off_parent,SEEK_SET);
				fread(I,BLOCK_SIZE,1,fp);
				fseek(fp,off_left,SEEK_SET);
				fwrite(left_node,BLOCK_SIZE,1,fp);
				delete_entry_index(I, i, off_parent);
				deletePage(L_offset);
			} else {
				int number_of_keys = right_node->number_of_keys;
				memcpy((L->records)+(L->number_of_keys), right_node->records, sizeof(Record) * number_of_keys);
				L->right_page_offset = right_node -> right_page_offset;
				right_node->number_of_keys = right_node->number_of_keys + L->number_of_keys;
				
				fseek(fp, off_parent,SEEK_SET);
				fread(I,BLOCK_SIZE,1,fp);
				fseek(fp,L_offset,SEEK_SET);
				fwrite(L,BLOCK_SIZE,1,fp);
				delete_entry_index(I, i+1, off_parent);
				deletePage(off_right);
			}
		}
	}
	free(L);
	free(left_node);
	free(right_node);
	free(I);
	return 0;
}
