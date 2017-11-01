#include "bpt.h"

/**
	If we make a new file, we have to initialize our header page.
	This function will help you to do it.
	Make new header page and Write it in file.
	Set FREE page offset to the first page offset.
 **/

void headerInit(){
	head = (HeaderPage*)malloc(BLOCK_SIZE);
	memset(head, 0, sizeof (head));
	head -> free_page = (off_t)4096;
	head -> root_page = (off_t)0;
	head -> number_of_pages = 0;

	// For initialize the free page offset.
	LeafNode* tmp = getNewLeafNode();
	readFromFile(tmp, BLOCK_SIZE);
	tmp -> parent_page_offset = 2 * BLOCK_SIZE;
	writeInFile(tmp, BLOCK_SIZE);
	FREE(tmp);

	writeInFile(head, 0);
}

/**
	Before using other operation like insert, delete, find, we must call this function before.
	If the file named pathname already exists, It will open it with "r+" mode.
	If not, It will make the new file named pathname and open it with "w+" mode.
 **/

int open_db(const char* pathname){
	if ((fp = fopen(pathname, "r+")) == NULL) {
		fp = fopen(pathname, "w+"); // If not exist, open with "w+"
		headerInit();
	} else {
		getHeaderPage();
	}
	if (fp == NULL) {
		return -1;
	}
	return 0;
}

/**
	This function is used to close the file.
 **/

void close_db(){
	if (fp != NULL) {
		fclose(fp);
	}
}

/**
	These two functions named writeInFile and readFromFile mean by their literal name.
	For multi user, change this function to atomic function.
 **/

void writeInFile(void* data, off_t off_data){
	fseek(fp, off_data, SEEK_SET);
	fwrite(data, BLOCK_SIZE, 1, fp);
}

void readFromFile(void* data, off_t off_data){
	fseek(fp, off_data, SEEK_SET);
	fread(data, BLOCK_SIZE, 1, fp);
}

/**
	This function is used to copy R's data to L's data. 
	start_R is the start index in R. And start_L is the start index in L.
	size is the number of records we want to copy.
 **/

void mappingInRecord(Record* L, int start_L, Record* R, int start_R, int size) {
	for (int i = start_L, j = start_R; j < start_R + size; ++j) {
		L[i].key = R[j].key;
		strcpy(L[i].value, R[j].value);
		i++;
	}
}

/**
	This function is used to copy R's data to L's data.
	start_R is the start index in R. And start_L is the start index in L.
	size is the number of indexes we want to copy.
 **/

void mappingInIndex(Index* L, int start_L, Index* R, int start_R, int size){
	for (int i = start_L, j = start_R; j < start_R + size; ++j) {
		L[i].key = R[j].key;
		L[i].page_offset = R[j].page_offset;
		i++;
	}
}

/**
	This function is used to read header page from file.
 **/

void getHeaderPage(){
	head = (HeaderPage*)malloc(BLOCK_SIZE);
	readFromFile(head, 0);
}

/**
	This function is used to get new leaf node.
	Make the new leaf node with initializing and return it.
 **/

LeafNode* getNewLeafNode(){
	LeafNode* tmp = (LeafNode*)malloc(BLOCK_SIZE);
	memset(tmp, 0, BLOCK_SIZE);
	tmp -> parent_page_offset = 0;
	tmp -> is_leaf = 1;
	tmp -> number_of_keys = 0;
	for (int i = 0; i < 104; ++i) {
		(tmp -> hole)[i] = 'a';
	}
	tmp -> right_page_offset = 0;
	return tmp;
}

/**
	This function is used to get new internal node.
	Make the new internal node with initializing and return it.
 **/

InternalNode* getNewInternalNode(){
	InternalNode* tmp = (InternalNode*)malloc(BLOCK_SIZE);
	memset(tmp, 0, BLOCK_SIZE);
	tmp -> parent_page_offset = 0;
	tmp -> is_leaf = 0;
	tmp -> number_of_keys = 0;
	for (int i = 0; i < 104; ++i) {
		(tmp -> hole)[i] = 'a';
	}
	tmp -> left_most_offset = 0;
	return tmp;
}

/**
	This function is used to find the child offset from index to find the leaf node which is including the key.

Return : Position of the key in Parent Node which is the smallest of those that are larger than the key.
 **/

int findPositionInIndex(int64_t key, Index* index, int number_of_keys){

	int start = 0;
	int end = number_of_keys - 1;
	int mid = (start + end) / 2;

	while (start <= end) {

		if (index[mid].key == key) {
			return mid + 1;

		} else if(index[mid].key < key) {
			start = mid + 1;
			mid = (start + end) / 2;

		} else {
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
In case 1> return left most.
In case 2> return number of keys in index.
 **/
int searchIndex(int64_t key, Index* index, int number_of_keys){

	// key is smaller than the smallest key in index.
	if (key < index[0].key) {
		return -1;
		// key is larger than the largest key in index.
	} else if (key >= index[number_of_keys - 1].key) {

		return number_of_keys;
		// find the position of the smallest key of the larger ones.
	} else {
		return findPositionInIndex(key, index, number_of_keys);
	}
}

/**
	This function is used to find the key from leaf node.

Return : the position of key in leaf node.
If you cannot find it, return -1.
 **/
int findPositionInLeaf(int64_t key, Record* record, int number_of_keys){

	int start = 0;
	int end = number_of_keys - 1;
	int mid = (start + end) / 2;

	while (start <= end) {

		if (record[mid].key == key) {
			return mid;

		} else if (record[mid].key < key) {
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
	1> If it is an empty tree, return NULL. if not, keep going down.
	2> Get root page offset from file and check out whether leaf or not.
	If it is a leaf node, search the key we want to find in the node.
	3> If the root page is not a leaf node, find the leaf node by loop.
	When we find the leaf node, search the key we want to find in the node.

Return : value in key or NULL
 **/

char* find(int64_t key){
	// If it is empty, return NULL
	if (head -> number_of_pages == 0) {
		return NULL;
	}
	// Define VAULE to return char*.
	char* VALUE = (char*)malloc(sizeof (char) * 120);
	memset(VALUE, 0, sizeof (char) * 120);

	off_t off_root = head -> root_page;

	LeafNode* L = getNewLeafNode();
	InternalNode* I = getNewInternalNode();

	readFromFile(I, off_root);
	int index;
	off_t off_tmp;

	// If root node is an leaf node, find the key in node.
	if (I -> is_leaf == 1) { 
		FREE(I);
		readFromFile(L, off_root);
		index = findPositionInLeaf(key, L -> records, L -> number_of_keys);

		if (index == -1) { // Fail to find the key.
			FREE(L);
			FREE(VALUE);
			return NULL;
		}
		else {             // Success to find the key.
			strcpy(VALUE, (L -> records)[index].value); 
			FREE(L);
			return VALUE;
		}
		// If root node is not a leaf node, find leaf node first. 
	} else { 

		while (I -> is_leaf != 1) {

			index = searchIndex(key, I -> indexes, I -> number_of_keys);

			if (index == -1) {
				off_tmp = (I -> left_most_offset);

			} else {
				off_tmp = (I -> indexes)[index - 1].page_offset;
			}

			readFromFile(I,off_tmp);
		}
		FREE(I);
		// off_tmp stores the offset of leaf node.
		readFromFile(L,off_tmp);
		index = findPositionInLeaf(key, L -> records, L -> number_of_keys);

		if (index == -1) { // Fail to find the key.
			FREE(L);
			FREE(VALUE);
			return NULL;

		} else {          // Success to find the key.
			strcpy(VALUE, (L -> records)[index].value);
			FREE(L);
			return VALUE;
		}
		return NULL;
	}
}

/**
	First, read header page from file to get the FREE page offset.
	if the FREE page pointed by header page does not point other FREE page, we set the FREE page offset in header page to the block next to the last one.
	if not, set the FREE page offset in header page to the offset pointed by the page pointed by header page.

Return : new page offset.
 **/
off_t getNewPage(){

	off_t new_page_offset = head -> free_page;
	int64_t number_of_pages = head -> number_of_pages;

	LeafNode* tmp = getNewLeafNode();
	readFromFile(tmp, new_page_offset);
	(head -> number_of_pages)++;
	// There is an unique FREE page in tree.
	if (tmp -> parent_page_offset == 0) {
		head -> free_page = BLOCK_SIZE + BLOCK_SIZE * (head -> number_of_pages);
		// There are some FREE pages in tree.
	} else {
		head -> free_page = tmp -> parent_page_offset;
	}
	FREE(tmp);
	return new_page_offset;
}

/**
	Unfortunately, leaf node struct and internal node struct are different.
	So, the function is implemented by two different types.
	I will fix it as soon as possible.

	Let v prime be the value such that exactly ceil(n/2) of the values are less than v prime. n is MAX KEY IN LEAF of INDEX + 1.
	Find the v prime by branch and the moving position that is next to the v prime' s position.
	parameter 'v_prime_position' helps you to move the keys and pointers left node to right one.

Return : v prime
 **/

int64_t findVPrimeInLeaf(LeafNode* L, int64_t key, int* v_prime_position){
	int v_prime_pos = (int)ceil((L -> number_of_keys + 1) / 2);

	if (key > (L -> records)[v_prime_pos].key) {
		*v_prime_position = v_prime_pos;
		return (L -> records)[v_prime_pos].key;
	}
	else {
		if (key < (L -> records)[v_prime_pos - 1].key) {
			*v_prime_position = v_prime_pos - 1;
			return (L->records)[v_prime_pos - 1].key;
		}
		else {
			*v_prime_position = v_prime_pos;
			return key;
		}
	}
}

/**
	Same as function 'findVPrimeInLeaf'.
 **/

int64_t findVPrimeInIndex(InternalNode* I, int64_t key, int* v_prime_position){
	int v_prime_pos = (int)ceil((I -> number_of_keys + 1) / 2);

	if (key > (I -> indexes)[v_prime_pos].key) {
		*v_prime_position = v_prime_pos;
		return (I -> indexes)[v_prime_pos].key;
	}
	else {
		if (key < (I -> indexes)[v_prime_pos - 1].key) {
			*v_prime_position = v_prime_pos - 1;
			return (I -> indexes)[v_prime_pos - 1].key;
		}
		else {
			*v_prime_position = v_prime_pos;
			return key;
		}
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
	// If it is empty, return NULL
	if (head -> number_of_pages == 0){
		return NULL;
	}

	off_t off_root = head -> root_page;

	LeafNode* L = getNewLeafNode();
	InternalNode* I = getNewInternalNode();
	readFromFile(I, off_root);

	off_t off_tmp;
	int index;
	// If it is a leaf node, return it.
	if (I -> is_leaf == 1) { 
		FREE(I);
		readFromFile(L, off_root);
		*off_leaf = off_root;
		return L;
		// If not, find the leaf node.
	} else {
		while (I -> is_leaf != 1) {

			index = searchIndex(key ,I -> indexes, I -> number_of_keys);

			if (index == -1) {
				off_tmp = (I -> left_most_offset);

			} else {
				off_tmp = (I -> indexes)[index - 1].page_offset;
			}
			readFromFile(I, off_tmp);

		}
		FREE(I);
		readFromFile(L, off_tmp);
		*off_leaf = off_tmp;
		return L;
	}
}

/**
	Unfortunately, leaf node struct and internal node struct are different.
	So, the function is implemented by two different types.
	I will fix it as soon as possible.


	1> If it is an empty node, put the key and value on the first position.
	2> If the key is smaller than the smallest in node, put the key and value on the first position, by shifting other keys and values right.
	3> If the key is larger than the largest in node, put the key and value on the last position.
	4> If not, find the position and put it in there, by shifting keys and values on the right side right.

Return : 0 if OK
 **/

int insertInLeaf(LeafNode* L, int64_t key, char* value){
	int number_of_keys = L -> number_of_keys;
	// If it is empty, put the data in first position.
	if (L -> number_of_keys == 0) {
		(L -> records)[0].key = key;
		strcpy((L -> records)[0].value, value);
		(L -> number_of_keys)++;
		return 0;
	}

	int start = 0;
	int end = number_of_keys - 1;
	int mid = (start + end) / 2;

	Record tmp_records[31];
	memset(tmp_records, 0, sizeof (tmp_records));
	// Find the position of the key.
	// key is smaller than the smallest key in node.
	if (key < (L -> records)[0].key) {
		mappingInRecord(tmp_records, 0, L -> records, 0, L -> number_of_keys);
		(L -> records)[0].key = key;
		strcpy((L -> records)[0].value, value);
		mappingInRecord(L -> records, 1, tmp_records, 0, L -> number_of_keys);
		(L -> number_of_keys)++;
		return 0;
		// key is larger than the largest key in node.
	} else if (key > (L -> records)[end].key) {
		(L -> records)[number_of_keys].key = key;
		strcpy((L -> records)[number_of_keys].value, value);
		(L -> number_of_keys)++;
		return 0;
		// you have to find the position by using binary search.
	} else {
		while (start <= end) {
			if ((L -> records)[mid].key > key) {
				end = mid - 1;
				mid = (start + end) / 2;
			} else {
				start = mid + 1;
				mid = (start + end) / 2;
			}
		}
	}

	int number_of_copy = number_of_keys - start;

	mappingInRecord(tmp_records, 0, L -> records, start, number_of_copy);
	(L -> records)[start].key = key;
	strcpy((L -> records)[start].value, value);
	mappingInRecord(L -> records, start + 1, tmp_records, 0, number_of_copy);
	(L -> number_of_keys)++;

	return 0;
}

/**
	Same as function 'insertInLeaf'.
 **/

int insertInIndex(InternalNode* I, int64_t key, off_t page_offset){
	int number_of_keys = I -> number_of_keys;
	// If it is empty, put the data in first position.	
	if (number_of_keys == 0) {

		(I -> indexes)[0].key = key;
		(I -> indexes)[0].page_offset = page_offset;
		(I -> number_of_keys)++;

		return 0;
	}

	int start = 0;
	int end = number_of_keys - 1;
	int mid = (start + end) / 2;

	Index tmp_indexes[248];
	memset(tmp_indexes, 0, sizeof (tmp_indexes));
	// key is smaller than the smallest key in node.
	if (key < (I -> indexes)[0].key) {	
		mappingInIndex(tmp_indexes, 0, I -> indexes, 0, I -> number_of_keys);
		(I -> indexes)[0].key = key;
		(I -> indexes)[0].page_offset = page_offset;
		mappingInIndex(I -> indexes, 1, tmp_indexes, 0, I -> number_of_keys);
		(I -> number_of_keys)++;
		return 0;
		// key is larger than the largest key in node.
	} else if (key > (I -> indexes)[end].key) {
		(I -> indexes)[number_of_keys].key = key;
		(I -> indexes)[number_of_keys].page_offset = page_offset;
		(I -> number_of_keys)++;
		return 0;
		// you have to find the position by using binary search.
	} else {
		while (start <= end) {
			if ((I -> indexes)[mid].key > key) {
				end = mid - 1;
				mid = (start + end) / 2;
			} else {
				start = mid + 1;
				mid = (start + end) / 2;
			}
		}
	}

	int number_of_copy = number_of_keys - start;

	mappingInIndex(tmp_indexes, 0, I -> indexes, start, number_of_copy);
	(I -> indexes)[start].key = key;
	(I -> indexes)[start].page_offset = page_offset;
	mappingInIndex(I -> indexes, start + 1, tmp_indexes, 0, number_of_copy);
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

	off_t new_page = getNewPage();
	// Define new right leaf node, then set some data.
	LeafNode* right_leaf_node = getNewLeafNode();
	right_leaf_node -> right_page_offset = L -> right_page_offset;
	right_leaf_node -> parent_page_offset = L -> parent_page_offset;

	L -> right_page_offset = new_page;
	L -> parent_page_offset = off_parent;

	// Find the position of the v prime key.
	int v_prime_position = 0;
	int64_t v_prime = findVPrimeInLeaf(L, key, &v_prime_position);

	mappingInRecord(right_leaf_node -> records, 0, L -> records, v_prime_position, MAX_KEY_IN_LEAF - v_prime_position);

	right_leaf_node -> number_of_keys = MAX_KEY_IN_LEAF - v_prime_position;
	L -> number_of_keys = v_prime_position;
	// Put the key in left or right node.
	if (key < v_prime) {
		insertInLeaf(L, key, value);
	} else {
		insertInLeaf(right_leaf_node, key, value);
	}

	writeInFile(right_leaf_node, new_page);
	FREE(right_leaf_node);
	return v_prime;
}

/**
	This function is used to split the internal node because of insertion overflow.

	1> Make the new internal node and set the left-most page offset and parent page offset.
	2> Move the keys and page offsets in 'I' to new node.
	3> Change the child nodes' parent page offset, the child node of new node. 

Return : v prime.
 **/

int64_t splitInIndex(InternalNode* I, int64_t v_prime, int v_prime_position, int64_t key, off_t page_offset, off_t parent_offset, off_t* off_r_I){
	off_t off_new = getNewPage();
	// Define the new right internal node, then set some data.
	InternalNode* right_internal_node = getNewInternalNode();

	right_internal_node -> parent_page_offset = parent_offset;
	*off_r_I = off_new;
	// if key is same as v prime key, 
	if (key == v_prime) {
		right_internal_node -> left_most_offset = page_offset;
		mappingInIndex(right_internal_node -> indexes, 0, I -> indexes, v_prime_position, MAX_KEY_IN_INDEX - v_prime_position);
		I -> number_of_keys = v_prime_position;
		right_internal_node -> number_of_keys = MAX_KEY_IN_INDEX - v_prime_position;
		// if not, you have to consider about copy of data.
	} else {
		right_internal_node -> left_most_offset = (I -> indexes)[v_prime_position].page_offset;
		mappingInIndex(right_internal_node -> indexes, 0, I -> indexes, v_prime_position + 1, MAX_KEY_IN_INDEX - v_prime_position - 1);
		I -> number_of_keys = v_prime_position;
		right_internal_node -> number_of_keys = MAX_KEY_IN_INDEX - v_prime_position - 1;
	}

	if (key < v_prime) {
		insertInIndex(I, key, page_offset);
	} else if (key > v_prime) {
		insertInIndex(right_internal_node, key, page_offset);
	}
	writeInFile(right_internal_node, off_new);

	// Change the parent page offset of children of new right node.
	InternalNode* child = getNewInternalNode();
	// First, change the child pointed by left-most offset.
	off_t off_child = right_internal_node -> left_most_offset;
	readFromFile(child, off_child);
	child -> parent_page_offset = off_new;
	writeInFile(child, off_child);
	// Then, change other children nodes.
	for (int i = 0; i < right_internal_node -> number_of_keys; ++i) {
		off_child = (right_internal_node -> indexes)[i].page_offset;
		readFromFile(child, off_child);
		child -> parent_page_offset = off_new;
		writeInFile(child, off_child);
	}

	FREE(right_internal_node);
	FREE(child);	
	return v_prime;
}

/**
	This function is used to do insert in internal node and is recursive one.

	Case 1> No need to split.
	Case 2> Split, then put the v prime in parent node.
	Case 3> If do Case 2>, do Case 1> and Case 2> in parent node by using recursive function.

 **/

int insertEntryIndex(InternalNode* I, off_t off_I, int64_t key, off_t page_offset){
	off_t off_parent = I -> parent_page_offset;
	off_t off_right_internal_node = 0;
	// No need to split.
	if (I -> number_of_keys < MAX_KEY_IN_INDEX) {
		insertInIndex(I ,key ,page_offset);
		writeInFile(I, off_I);
		FREE(I);
		return 0;
		// Split.
	} else {
		// If it is a root node, first define the new page offset.
		if (I -> parent_page_offset == 0) {
			off_parent = getNewPage();
		}
		// moving data I to new right node.
		int v_prime_position = 0;
		int64_t v_prime = findVPrimeInIndex(I,key,&v_prime_position);
		splitInIndex(I, v_prime, v_prime_position, key, page_offset, off_parent, &off_right_internal_node);
		// In case of root node, you have to make a new root node. Then insert v prime key and set offsets in it.
		if (I->parent_page_offset == 0) {
			I -> parent_page_offset = off_parent;
			InternalNode* root_node = getNewInternalNode();

			head -> root_page = off_parent;
			root_node -> left_most_offset = off_I;
			insertInIndex(root_node, v_prime, off_right_internal_node);
			writeInFile(root_node, off_parent);
			writeInFile(I, off_I);

			FREE(root_node);
			FREE(I);
			return 0;
			// If it is not a root node, do insert v prime key and offset in parent by using recursive form.
		} else {
			InternalNode* parent = getNewInternalNode();
			readFromFile(parent, off_parent);
			writeInFile(I, off_I);
			FREE(I);
			insertEntryIndex(parent, off_parent, v_prime, off_right_internal_node);

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

Return : 0 if success, -1 if fail to insert, for example duplication.
 **/

int insert(int64_t key, char* value){

	off_t off_L = 0;
	LeafNode* L = findLeaf(key, &off_L);
	InternalNode* parent = getNewInternalNode();

	// If it is emtpy, make new leaf node and insert data in it.
	if (L == NULL) {
		off_t off_new = getNewPage();
		head -> root_page = off_new;
		L = getNewLeafNode();
		insertInLeaf(L,key,value);

		writeInFile(L, off_new);
		writeInFile(head, 0);

		FREE(L);
		FREE(parent);
		return 0;
	}

	// If the key already exists, return -1.
	if (findPositionInLeaf(key, L -> records, L -> number_of_keys) != -1) {
		FREE(L);
		FREE(parent);
		return -1;
	}

	off_t off_parent = L -> parent_page_offset;
	off_t off_right = 0;
	int64_t v_prime = 0;

	// No need to split.
	if (L -> number_of_keys < MAX_KEY_IN_LEAF) {
		insertInLeaf(L, key, value);
		writeInFile(L, off_L);
		FREE(L);
		FREE(parent);
		// Split.
	} else {
		// If L is root node, make new root node.
		if (L -> parent_page_offset == 0) { 
			off_parent = getNewPage();
			L -> parent_page_offset = off_parent;
			//find the v prime key to insert in parent.
			v_prime = splitInLeaf(L, key, value, off_parent);
			off_right = L -> right_page_offset;

			parent -> left_most_offset =  off_L;
			insertInIndex(parent, v_prime, off_right);

			head -> root_page = off_parent;

			writeInFile(parent, off_parent);
			writeInFile(L, off_L);

			FREE(parent);
			FREE(L);
			// If L is not a root node, insert the v prime key in parent. 
			// In this case, we use the function 'insertEntryIndex'.
		} else { 
			v_prime = splitInLeaf(L, key, value, off_parent);
			off_right = L -> right_page_offset;

			writeInFile(L, off_L);
			readFromFile(parent, L -> parent_page_offset);

			insertEntryIndex(parent, L -> parent_page_offset, v_prime, off_right);
			FREE(L);
		}
	}
	writeInFile(head, 0);
	return 0;
}


/**
	This function is used to delete page. Then Do write in file.
 **/

void deletePage(off_t off_delete){

	LeafNode* tmp = getNewLeafNode();

	tmp -> parent_page_offset = head -> free_page;
	head -> free_page = off_delete;
	(head -> number_of_pages)--;
	// If it is empty tree, initialize header page.
	if (head -> number_of_pages == 0) {
		headerInit();
	} else {
	}
	writeInFile(tmp, off_delete);
	FREE(tmp);
}

/**
	This function is used to find the child's offset position in parent.
	I used loop, because the page number in file is not sequencial.
 **/

int findPositionInParent(InternalNode* parent, off_t off_L){
	if (off_L == parent -> left_most_offset) {
		return -1;
	}
	for (int i = 0; i < parent -> number_of_keys; ++i) {
		if (off_L == (parent -> indexes)[i].page_offset) {
			return i;
		}
	}
}

/**
	This function is used to find the v prime position and offsets of siblings.
 **/

int getVPrimeIndex(off_t off_L, off_t off_parent, off_t* off_right, off_t* off_left){
	InternalNode* parent = getNewInternalNode();
	readFromFile(parent, off_parent);

	int pos = findPositionInParent(parent, off_L);

	if (pos == -1) { 
		*off_right = (parent -> indexes)[0].page_offset;

	} else if (pos == 0) {
		if (parent -> number_of_keys == 1) {
			*off_left = parent -> left_most_offset;
		} else {
			*off_left = parent -> left_most_offset;
			*off_right = (parent -> indexes)[1].page_offset;
		} 
	} else if (pos != 0 && pos == (parent -> number_of_keys) - 1){
		*off_left = (parent -> indexes)[pos - 1].page_offset;

	} else {
		*off_left = (parent -> indexes)[pos - 1].page_offset;
		*off_right = (parent->indexes)[pos + 1].page_offset;
	}
	FREE(parent);
	return pos;
}

/**
	This function is used to delete the key and value in leaf node.
 **/

int deleteInLeaf(LeafNode* L, int64_t key, off_t off_L){
	int pos = findPositionInLeaf(key, L -> records, L -> number_of_keys);
	// If cannot find, return -1.
	if (pos == -1) {
		return -1;
	}
	// If the key is on right most position, Do decrease number of keys.
	if (pos == L -> number_of_keys - 1) {
		(L -> number_of_keys)--;
		return 0;
	}

	int number_of_copy = (L -> number_of_keys) - pos - 1;
	Record tmp_records[MAX_KEY_IN_LEAF];
	memset(tmp_records, 0, sizeof (Record) * MAX_KEY_IN_LEAF);

	mappingInRecord(tmp_records, 0, L -> records, pos + 1, number_of_copy);
	mappingInRecord(L -> records, pos, tmp_records, 0, number_of_copy);
	(L -> number_of_keys)--;
	return 0;
}

/**
	This function is used to delete the key and offset in internal node.
 **/

void deleteInIndex(InternalNode* I, int pos){
	// If the pos is on right most position, Do decrease number of keys.
	if (pos == I -> number_of_keys - 1){
		(I -> number_of_keys)--;
		return;
	}

	int number_of_copy = (I -> number_of_keys) - pos - 1;
	Index tmp_index[MAX_KEY_IN_INDEX];
	memset(tmp_index, 0, sizeof (Index) * MAX_KEY_IN_INDEX);

	mappingInIndex(tmp_index, 0, I -> indexes, pos + 1, number_of_copy);
	mappingInIndex(I -> indexes, pos, tmp_index, 0, number_of_copy);
	(I -> number_of_keys)--;
}

/**
	This function is used to borrow the record from left sibling.
 **/

void borrowInLeftLeafNode(LeafNode* L, LeafNode* left_node, off_t off_parent, int v_prime_index){
	// First, get parent node.
	InternalNode* parent = getNewInternalNode();
	readFromFile(parent, off_parent);

	int number_of_keys = left_node -> number_of_keys;
	Record tmp_record[MAX_KEY_IN_LEAF];
	memset(tmp_record, 0, sizeof(Record) * MAX_KEY_IN_LEAF);
	// tmp_record[0] is the right most key and value in left sibling.
	tmp_record[0].key = (left_node -> records)[number_of_keys - 1].key;
	strcpy(tmp_record[0].value, (left_node -> records)[number_of_keys - 1].value);

	(left_node -> number_of_keys)--;

	number_of_keys = L -> number_of_keys;
	mappingInRecord(tmp_record, 1, L -> records, 0, number_of_keys);
	mappingInRecord(L -> records, 0, tmp_record, 0, number_of_keys + 1);
	(L -> number_of_keys)++;
	// Change the parent's v prime key to the left most key in node L.
	(parent -> indexes)[v_prime_index].key = tmp_record[0].key;
	writeInFile(parent, off_parent);
	FREE(parent);
}

/**
	This function is used to borrow the record from right sibling.
 **/

void borrowInRightLeafNode(LeafNode* L, LeafNode* right_node, off_t off_parent, int v_prime_index){
	InternalNode* parent = getNewInternalNode();
	readFromFile(parent, off_parent);

	int number_of_keys = right_node -> number_of_keys;
	Record tmp_record[MAX_KEY_IN_LEAF];
	memset(tmp_record, 0, sizeof (Record) * MAX_KEY_IN_LEAF);
	// Copy right node's records to tmp_record. 
	mappingInRecord(tmp_record, 0, right_node -> records, 0, number_of_keys);
	// Copy tmp_record to right node except for left most record to borrow.
	mappingInRecord(right_node -> records, 0, tmp_record, 1, number_of_keys - 1);
	(right_node -> number_of_keys)--;

	number_of_keys = L -> number_of_keys;
	// Put the left most record in right sibling to right most record in node L.
	(L -> records)[number_of_keys].key =  tmp_record[0].key;
	strcpy((L -> records)[number_of_keys].value, tmp_record[0].value);
	(L -> number_of_keys)++;
	// Change the parent's v prime key to the left most key in right node.
	(parent -> indexes)[v_prime_index].key = tmp_record[1].key;
	writeInFile(parent, off_parent);
	FREE(parent);
}

/**
	This function is used to borrow index from left sibling.
	In Internalnode, the process of borrowing is quite different from that in leaf node.
 **/

void borrowInLeftInternalNode(InternalNode* I, InternalNode* left_node, off_t off_parent, int v_prime_index, off_t off_I){
	InternalNode* parent = getNewInternalNode();
	readFromFile(parent, off_parent);
	// tmp_key is for parent's v prime key.
	int64_t tmp_key = (left_node -> indexes)[(left_node -> number_of_keys) - 1].key;
	// tmp_page_offset is for I's left most offset.
	off_t tmp_page_offset = (left_node -> indexes)[(left_node -> number_of_keys) - 1].page_offset;

	Index tmp_index[MAX_KEY_IN_INDEX];
	memset(tmp_index, 0, sizeof (Index) * MAX_KEY_IN_INDEX);
	// set the first Index in tmp_index.
	tmp_index[0].key = (parent -> indexes)[v_prime_index].key;
	tmp_index[0].page_offset = I -> left_most_offset;
	// set the left most offset
	I -> left_most_offset = tmp_page_offset;
	(left_node -> number_of_keys)--;

	mappingInIndex(tmp_index, 1, I -> indexes, 0, I -> number_of_keys);
	mappingInIndex(I -> indexes, 0, tmp_index, 0, (I -> number_of_keys) + 1);
	(I -> number_of_keys)++;
	// Change the parent's v prime key to tmp_key.
	(parent -> indexes)[v_prime_index].key = tmp_key;
	writeInFile(parent, off_parent);
	// Change the child's offset because of changing it's parent node.
	LeafNode* child = getNewLeafNode();
	readFromFile(child, I -> left_most_offset);
	child -> parent_page_offset = off_I;
	writeInFile(child, I -> left_most_offset);
	FREE(child);
	FREE(parent);
}

/**
	This function is used to borrow index from right sibling.
 **/

void borrowInRightInternalNode(InternalNode* I, InternalNode* right_node, off_t off_parent, int v_prime_index, off_t off_I){
	InternalNode* parent = getNewInternalNode();
	readFromFile(parent, off_parent);

	int number_of_keys = right_node -> number_of_keys;
	// tmp_key is for parent's v prime key.
	int64_t tmp_key = (right_node -> indexes)[0].key;
	// tmp_page_offset is for the left most offset in right node.
	off_t tmp_page_offset = (right_node -> indexes)[0].page_offset;
	// parent_tmp_key and tmp_offset are for the right most index in node I.
	int64_t parent_tmp_key = (parent -> indexes)[v_prime_index].key;
	off_t tmp_offset = right_node -> left_most_offset;

	Index tmp_index[MAX_KEY_IN_INDEX];
	memset(tmp_index, 0, sizeof (Index) * MAX_KEY_IN_INDEX);
	// set the left most offset in right node
	right_node -> left_most_offset = tmp_page_offset;

	mappingInIndex(tmp_index, 0, right_node -> indexes, 1, number_of_keys - 1);
	mappingInIndex(right_node -> indexes, 0, tmp_index, 0, number_of_keys - 1);
	(right_node -> number_of_keys)--;
	// set the right most index in node I.
	number_of_keys = I -> number_of_keys;
	(I -> indexes)[number_of_keys].key =  parent_tmp_key;
	(I -> indexes)[number_of_keys].page_offset = tmp_offset;
	(I -> number_of_keys)++;
	// Change the parent's v prime key to tmp_key.
	(parent -> indexes)[v_prime_index].key = tmp_key;
	writeInFile(parent, off_parent);
	// Change child's offset beacuse of changing it's parent.
	LeafNode* child = getNewLeafNode();
	readFromFile(child, tmp_offset);
	child -> parent_page_offset = off_I;
	writeInFile(child, tmp_offset);
	FREE(child);
	FREE(parent);
}

/**
	This function is used to delete the key and offset in internal node.
	Also, it is recursive function.
	It's process is same as funcion 'delete'.
 **/

void deleteEntryIndex(int index, off_t off_I){

	InternalNode* I = getNewInternalNode();
	readFromFile(I, off_I);

	deleteInIndex(I, index); 

	off_t off_parent = I -> parent_page_offset;
	off_t tmp_page_offset = 0;
	// If it is a root node and it has some keys, then just write in file.
	if (off_parent == 0 && I -> number_of_keys != 0) {
		writeInFile(I, off_I);
		FREE(I);
		return;
		// If it is a root node but it is empty after delete, make the left most child root node.
	} else if (off_parent == 0 && I -> number_of_keys == 0) {
		tmp_page_offset = I -> left_most_offset;

		InternalNode* new_root = getNewInternalNode();
		readFromFile(new_root, tmp_page_offset);
		new_root -> parent_page_offset = 0;
		head -> root_page = tmp_page_offset;
		writeInFile(new_root, tmp_page_offset);
		deletePage(off_I);

		FREE(new_root);
		FREE(I);
		return;
	}

	off_t off_right = -1;
	off_t off_left = -1;

	int pos = getVPrimeIndex(off_I, off_parent, &off_right, &off_left);

	// Definition left and right node to merge or redistribute.
	InternalNode* left_node = getNewInternalNode();
	InternalNode* right_node = getNewInternalNode();

	if (off_left != -1) {
		readFromFile(left_node, off_left);
	}
	if (off_right != -1) {
		readFromFile(right_node, off_right);
	}
	// No need to merge or redistribute.
	if (I -> number_of_keys >= MIN_KEY_IN_INDEX) {
		writeInFile(I, off_I);

		// Merge or Redistribution.
	} else {
		// In case of redistribution.
		if (off_left != -1 && left_node -> number_of_keys > MIN_KEY_IN_INDEX) {
			borrowInLeftInternalNode(I, left_node, off_parent, pos, off_I);
			writeInFile(left_node, off_left);
			writeInFile(I, off_I);

		} else if (off_right != -1 && right_node -> number_of_keys > MIN_KEY_IN_INDEX){
			borrowInRightInternalNode(I, right_node, off_parent, pos + 1, off_I);
			writeInFile(right_node, off_right);
			writeInFile(I, off_I);
			// In case of merge.
		} else {
			InternalNode* parent = getNewInternalNode();
			readFromFile(parent, off_parent);
			int64_t tmp_key = 0;
			off_t tmp_page_offset = 0;
			int number_of_keys = 0;

			if (off_left != -1) {
				tmp_key = (parent -> indexes)[pos].key;
				tmp_page_offset = I -> left_most_offset;
				number_of_keys = (left_node -> number_of_keys) + (I -> number_of_keys) + 1;

				(left_node -> indexes)[left_node -> number_of_keys].key = tmp_key;
				(left_node -> indexes)[left_node -> number_of_keys].page_offset = tmp_page_offset;
				mappingInIndex(left_node -> indexes, (left_node -> number_of_keys) + 1, I -> indexes, 0, I -> number_of_keys);
				// change the children's offset.
				LeafNode* child = getNewLeafNode();
				for (int i = left_node -> number_of_keys; i < number_of_keys; ++i) {
					readFromFile(child, (left_node -> indexes)[i].page_offset);
					child -> parent_page_offset = off_left;
					writeInFile(child, (left_node -> indexes)[i].page_offset);
				}
				FREE(child);

				left_node -> number_of_keys = number_of_keys;

				writeInFile(left_node, off_left);
				deleteEntryIndex(pos, off_parent);
				deletePage(off_I);

			} else if (off_right != -1) {
				tmp_key = (parent -> indexes)[pos + 1].key;
				tmp_page_offset = right_node -> left_most_offset;
				number_of_keys = (I -> number_of_keys) + (right_node -> number_of_keys) + 1;
				(I -> indexes)[I -> number_of_keys].key = tmp_key;
				(I -> indexes)[I -> number_of_keys].page_offset = tmp_page_offset;

				mappingInIndex(I -> indexes, (I -> number_of_keys) + 1, right_node -> indexes, 0, right_node -> number_of_keys);
				// change the children's offset.
				LeafNode* child = getNewLeafNode();
				for (int i = I -> number_of_keys; i < number_of_keys; ++i) {
					readFromFile(child, (I -> indexes)[i].page_offset);
					child -> parent_page_offset = off_I;
					writeInFile(child, (I -> indexes)[i].page_offset);
				}
				FREE(child);

				I -> number_of_keys = number_of_keys;

				writeInFile(I, off_I);
				deleteEntryIndex(pos + 1, off_parent);
				deletePage(off_right);
			}
			FREE(parent);
		}
	}
	FREE(I);
	FREE(left_node);
	FREE(right_node);
}

/**
	Deletion is a bit more complicated than insertion.

	Case 1> Delete the key. If there is no need to merge or redistribution, return.
	Case 2> In case of redistribution, borrow record from sibling. Then return.
	Case 3> In case of merge, Do merge with sibling, then delete key and offset in parent.

 **/

int Delete (int64_t key){
	off_t off_L = 0;
	LeafNode* L = findLeaf(key, &off_L);
	// If it is empty, return NULL.
	if (L == NULL) {
		return -1;
	}
	// If cannot find, return -1.
	if (findPositionInLeaf(key, L -> records, L -> number_of_keys) == -1) {
		FREE(L);
		return -1;
	}

	off_t off_parent = L -> parent_page_offset;
	// If L is a root node.
	if (off_parent == 0) {
		deleteInLeaf(L, key, off_L);
		// If it is empty after delete, delete page and initialize the tree.
		if (L -> number_of_keys == 0) {
			deletePage(off_L);
			headerInit();
			// If not, write in file.
		} else {
			writeInFile(L, off_L);
		}
		writeInFile(head, 0);
		FREE(L);
		return 0;
	}

	LeafNode* left_node = getNewLeafNode();
	LeafNode* right_node = getNewLeafNode();
	off_t off_right = -1;
	off_t off_left = -1;
	// Find the position of v prime key and offsets of siblings.
	int pos = getVPrimeIndex(off_L, L -> parent_page_offset, &off_right, &off_left);

	if (off_left != -1) {
		readFromFile(left_node, off_left);
	}
	if (off_right != -1) {
		readFromFile(right_node, off_right);
	}
	// Delete the key in Leaf L.
	deleteInLeaf(L, key, off_L);

	int number_of_keys = 0;
	off_t tmp_page_offset = 0;

	// No need to merge or redistribute.
	if ((L -> number_of_keys) >= MIN_KEY_IN_LEAF) {
		writeInFile(L, off_L);

	} else {
		// In case of redistribution.
		if (off_left != -1 && left_node -> number_of_keys > MIN_KEY_IN_LEAF) { 
			borrowInLeftLeafNode(L, left_node, off_parent, pos);
			writeInFile(left_node, off_left);
			writeInFile(L, off_L);

		} else if (off_right != -1 && right_node -> number_of_keys > MIN_KEY_IN_LEAF) {
			borrowInRightLeafNode(L, right_node, off_parent, pos + 1);
			writeInFile(L, off_L);
			writeInFile(right_node, off_right);
			// In case of merge, always copy right to left.
		} else {
			if (off_left != -1) {

				mappingInRecord(left_node -> records, left_node -> number_of_keys, L -> records, 0, L -> number_of_keys);
				tmp_page_offset = L -> right_page_offset;
				number_of_keys = (left_node -> number_of_keys) + (L -> number_of_keys);
				left_node -> right_page_offset = tmp_page_offset;
				left_node -> number_of_keys = number_of_keys;

				writeInFile(left_node, off_left);

				deleteEntryIndex(pos, off_parent);
				deletePage(off_L);

			} else if (off_right != -1) {

				mappingInRecord(L -> records, L -> number_of_keys, right_node -> records, 0, right_node -> number_of_keys);
				tmp_page_offset = right_node -> right_page_offset;
				number_of_keys = (L -> number_of_keys) + (right_node -> number_of_keys);
				L -> right_page_offset = tmp_page_offset;
				L -> number_of_keys = number_of_keys;

				writeInFile(L, off_L);

				deleteEntryIndex(pos + 1, off_parent);
				deletePage(off_right);
			}
		}
	}
	writeInFile(head, 0);
	FREE(L);
	FREE(left_node);
	FREE(right_node);
	return 0;
}
