//2013012096_YuJaeSun
#include "BPT.h"
/**
	If we make a new file, we have to initialize our header page.
	This function will help you to do it.
	Make new header page and Write it in file.
	Set free page off set the first page offset.
 **/
void header_init(){
	header_page* HEAD = (header_page*)malloc(sizeof(header_page));
	memset(HEAD, 0, sizeof(HEAD));
	HEAD->free_page = (off_t)4096;
	HEAD->root_page = (off_t)0;
	HEAD->number_of_pages = 0;
	fseek(fp, 0, SEEK_SET);
	fwrite(HEAD, sizeof(header_page), 1, fp);
	free(HEAD);
}
/**
	Before using other operation like insert, delete, find, we must call this function.
	If the file named pathname already exists, It will open it with read & write mode.
	If not, It will make the new file named pathname and open it with write & read mode.
 **/
int open_db(char* pathname){
	if((fp=fopen(pathname, "r+"))==NULL){
		fp=fopen(pathname, "w+");
		header_init();
	}
	if(fp==NULL) return -1;
	return 0;
}
void close_db(){
	if(fp != NULL)
		fclose(fp);
}
/**
	These three functions are needed when we try to get header page, new leaf page and new internal page.
 **/
header_page* getHeader(){
	header_page* tmp=(header_page*)malloc(sizeof(header_page));
	fseek(fp,0,SEEK_SET);
	fread(tmp, sizeof(header_page), 1, fp);
	return tmp;
}
leaf_node_f* getNewLeaf(){
	leaf_node_f* tmp=(leaf_node_f*)malloc(sizeof(leaf_node_f));
	memset(tmp,0,BSZ);
	tmp -> parent_page_offset = 0;
	tmp -> is_leaf = 1;
	tmp -> number_of_keys = 0;
	tmp -> right_page_offset = 0;
	return tmp;
}
internal_node_f* getNewInter(){
	internal_node_f* tmp = (internal_node_f*)malloc(sizeof(internal_node_f));
	memset(tmp,0,BSZ);
	tmp -> parent_page_offset = 0;
	tmp -> is_leaf = 0;
	tmp -> number_of_keys = 0;
	tmp -> left_most_offset = 0;
	return tmp;
}

/**
	이 함수는 리프를 찾을 때 중간 인덱스에서 어느 포인터를 통해 자식으로 내려가는지를 알려 주는 함수이다.
	index_f 에는 key 와 page_offset 이 있고 이 offset 은 그림으로 생각했을때 오른쪽을 가르킨다.
	return 하는 값에 있는 key value 는 현재 key 보다 큰것들중에서 가장 작은것이다.
	따라서 함수 return 을 받고 계산할때 그보다 하나 작은 번째에 있는 offset을 이용해야한다.
 **/
int find_i_in_index(int64_t key, index_f* index, int number_of_keys){
	int start=0;
	int end=number_of_keys-1;
	int mid=(start+end)/2;
	while(start<=end){
		if(index[mid].key==key) return mid+1;
		else if(index[mid].key < key) {
			start=mid+1;
			mid=(start+end)/2;
		}else{
			end=mid-1;
			mid=(start+end)/2;
		}
	}
	return start;
}
/**
	아예 [0]번째보다 작거나 제일큰것 이상이거나 하는 경우는 미리 빼서 계산한다.
	그게 아니면 바로 위에있는 함수를 통해 i 를 구해서 리턴해준다.
	-1 : left_most_offset
	number_of_keys-1 : 가장 끝
 **/
int search_index(int64_t key, index_f* index, int number_of_keys){
	int i;
	if(key < index[0].key) return -1;
	else if (key >= index[number_of_keys-1].key) return number_of_keys;
	else {
		i=find_i_in_index(key, index, number_of_keys);
		return i;
	}
}
/**
	리프에서 값을 찾을때 찾으면 그 i 번째 리턴.
	못찾으면 -1.
 **/
int find_i_in_leaf(int64_t key, record_f* record, int number_of_keys){
	int start = 0;
	int end = number_of_keys-1;
	int mid = (start+end)/2;
	while(start<=end){
		if(record[mid].key == key) return mid;
		else if (record[mid].key < key) {
			start = mid+1;
			mid = (start+end)/2;
		} else {
			end = mid-1;
			mid = (start+end)/2;
		}
	}
	return -1;
}
/**
	리프에서 찾는 번쨰 리턴.
 **/
int search_leaf(int64_t key, record_f* record, int number_of_keys){
	return find_i_in_leaf(key, record, number_of_keys);
}
/**
	First, what we have to do is checking its header page to find out whether empty or not and to find the root page offset.

 **/
char* find(int64_t key){
	header_page* HEAD = getHeader();
	if (HEAD->number_of_pages=0){ // If it is empty tree, return NULL
		return NULL;
	}
	char* VALUE = (char*)malloc(sizeof(char)*120);
	memset(VALUE,0,sizeof(char)*120);
	off_t off_root = HEAD->root_page;
	free(HEAD);
	leaf_node_f* L;
	internal_node_f* I=(internal_node_f*)malloc(sizeof(internal_node_f));
	memset(I,0,BSZ);
	fseek(fp,off_root,SEEK_SET);
	fread(I, BSZ,1,fp);  // I에다가 root_page를 읽음. TODO: fseek + fread 이거 함수로 구현. 하고 바꾸기. (void*)
	int index;
	off_t off_tmp;
	if(I->is_leaf == 1){ // 루트 노드가 리프노드일때 !.
		free(I);
		L = (leaf_node_f*)malloc(sizeof(leaf_node_f));
		memset(L,0,BSZ);
		fseek(fp,off_root,SEEK_SET);
		fread(L,BSZ,1,fp);
		index=search_leaf(key, L->records,L->number_of_keys);
		if(index==-1){ // 못찾았을때.
			free(L);
			return NULL;
		} else { // 찾았을때.
			strcpy(VALUE,(L->records)[index].value); 
			free(L);
			return VALUE;
		}
	} else { // 루트노드가 리프노드가 아닐때 -> 아래로 내려가야해 !!
		while(I->is_leaf != 1){ // 내부노드 다음 자식 offset을 첫줄함수로 index 받아서 해결. index-1 쪾으로 가는게 맞음!
			index=search_index(key,I->indexes,I->number_of_keys);
			if(index==-1){ // 제일 왼쪽일때 
				off_tmp=(I->left_most_offset);
			} else { // 그게 아닐때
				off_tmp=(I->indexes)[index-1].page_offset;
			}
			fseek(fp,off_tmp,SEEK_SET);
			fread(I,BSZ,1,fp); // 자식 노드 읽기.
		}
		// 루프 탈출 : I에는 현재 리프노드가 담겨있고 off_tmp 에는 리프노드의 offset 이 담겨있음.
		L = (leaf_node_f*)malloc(sizeof(leaf_node_f));
		memset(L,0,BSZ);
		free(I);
		fseek(fp,off_tmp,SEEK_SET); //off_tmp 에 노드 위치 기록.
		fread(L,BSZ,1,fp);
		// L 에다가 리프 노드 담았엉 !!.
		// 위에 TODO 랑 똑같은 함수 구현하면 데지롱.
		index=search_leaf(key,L->records,L->number_of_keys);
		if(index==-1){
			free(L); // 못찾.
			return NULL;
		} else { // 찾았을때
			strcpy(VALUE,(L->records)[index].value);
			free(L);
			return VALUE;
		}
		return NULL;
	}
}
/**
	getNewPage()
	새로운 페이지 offset 리턴해주기 !
	해더페이지 가져와서 그 해드가 가르키는 free page 에 페이지 만들꺼야.
	그 free page 가 만약에 다른 free page 가르켯을떄와 아닐때로 구분지었다.

	return offset of new page.
 **/
off_t getNewPage(){
	header_page* HEAD = getHeader();
	off_t off_new = HEAD -> free_page;
	int64_t number_of_pages = HEAD -> number_of_pages;

	leaf_node_f* tmp = (leaf_node_f*)malloc(sizeof(leaf_node_f));
	memset(tmp,0,sizeof(leaf_node_f));
	fseek(fp,off_new,SEEK_SET);
	fread(tmp,BSZ,1,fp);
	if(tmp->parent_page_offset == 0) { // 따로 가르키는 free page 없을때.
		HEAD -> free_page = BSZ + BSZ * (++number_of_pages);
		HEAD -> number_of_pages = number_of_pages;
	} else {
		HEAD -> free_page = tmp -> parent_page_offset;
		HEAD -> number_of_pages = (++number_of_pages);
	}
	fseek(fp,0,SEEK_SET);
	fwrite(HEAD,sizeof(header_page),1,fp);
	free(HEAD);
	free(tmp);
	return off_new;
}
/**
	Index 에서 split 할때 부모로 올려줄 key value 를 구해서 리턴해준다 !!.
 **/
int64_t find_V_index(internal_node_f* I, int64_t key){
	int number_of_keys = I -> number_of_keys; //248개가 정상.
	if ( key < (I->indexes)[number_of_keys/2].key){
		if (key < (I->indexes)[number_of_keys/2 -1].key){
			return (I->indexes)[number_of_keys/2-1].key;
		} else {
			return key;
		}
	} else {
		return (I->indexes)[number_of_keys/2].key;
	}
}

/**
	인자로 들어온 off_leaf 에 현재 찾은 리프값의 offset 을 리턴해준다.
	전체 return 은 그 리프의 포인터형.
 **/
leaf_node_f* findLeaf(int64_t key, off_t* off_leaf){
	header_page* HEAD = getHeader();
	if (HEAD->number_of_pages == 0){ // 빈페이지.
		free(HEAD);
		return NULL;
	}
	off_t off_root = HEAD->root_page;
	free(HEAD);
	leaf_node_f* L;
	internal_node_f* I=(internal_node_f*)malloc(sizeof(internal_node_f));
	memset(I,0,BSZ);
	fseek(fp,off_root,SEEK_SET);
	fread(I, BSZ,1,fp);  // L에다가 root_page를 읽음.
	int is_leaf;
	off_t off_tmp;
	int index;
	if(I->is_leaf == 1){ // 얘를 리턴해주면 데자누.
		free(I);
		L=(leaf_node_f*)malloc(sizeof(leaf_node_f));
		memset(L,0,BSZ);
		fseek(fp,off_root,SEEK_SET);
		fread(L,BSZ,1,fp);
		*off_leaf = off_root;
		return L;
	} else {
		while(I->is_leaf != 1){ // 내부노드 다음 자식 offset을 첫줄함수로 index 받아서 해결. index-1 쪾으로 가는게 맞음!
			index = search_index(key,I->indexes,I->number_of_keys);
			if(index == -1){
				off_tmp = (I->left_most_offset);
			} else {
				off_tmp = (I->indexes)[index-1].page_offset;
			}
			fseek(fp,off_tmp,SEEK_SET);
			fread(I,BSZ,1,fp); // 자식 노드 읽기.
		}
		L = (leaf_node_f*)malloc(sizeof(leaf_node_f));
		memset(L,0,BSZ);
		free(I);
		fseek(fp,off_tmp,SEEK_SET); //off_tmp 에 노드 위치 기록.
		fread(L,BSZ,1,fp);
		*off_leaf = off_tmp;
		return L;
	}
	return NULL;
}
/**
	리프에다가 넣는거.
 **/
int insert_in_leaf(leaf_node_f* L, int64_t key, char* value){
	int number_of_keys = L -> number_of_keys;
	if (L->number_of_keys == 0){
		(L->records)[0].key = key;
		strcpy((L->records)[0].value, value);
		L->number_of_keys = number_of_keys+1;
		return 0;
	}
	int start = 0;
	int end = number_of_keys-1;
	int mid = (start+end)/2;
	record_f tmp_records[31];
	memset(tmp_records,0,sizeof(tmp_records));
	if( key < (L->records)[0].key){
		memcpy(tmp_records, L->records, sizeof(record_f)*number_of_keys);
		(L->records)[0].key = key;
		strcpy((L->records)[0].value,value);
		memcpy((L->records)+1, tmp_records, sizeof(record_f)*number_of_keys);
		L->number_of_keys = number_of_keys + 1;
		return 0;
	} else if (key > (L->records)[end].key){
		(L->records)[number_of_keys].key=key;
		strcpy((L->records)[number_of_keys].value,value);
		L->number_of_keys = number_of_keys + 1;
		return 0;
	} else {
		while(start<=end){
			if((L->records)[mid].key > key){
				end=mid-1;
				mid=(start+end)/2;
			} else {
				start=mid+1;
				mid=(start+end)/2;
			}
		}
	}

	//start 는 key 보다 큰것들중 가장 작은거의 i 를 가지고 있음.
	int number_of_copy = number_of_keys - start;
	memcpy(tmp_records, (L->records)+start, sizeof(record_f) * number_of_copy);
	(L->records)[start].key = key;
	strcpy((L->records)[start].value, value);
	memcpy((L->records)+start+1, tmp_records, sizeof(record_f) * number_of_copy);
	L -> number_of_keys = number_of_keys + 1;
	return 0;
}

int insert_in_index(internal_node_f* I, int64_t key, off_t page_offset){
	int number_of_keys = I -> number_of_keys;
	if (number_of_keys == 0){
		(I->indexes)[0].key=key;
		(I->indexes)[0].page_offset = page_offset;
		I->number_of_keys = number_of_keys + 1;
		return 0;
	}
	int start = 0;
	int end = number_of_keys - 1;
	int mid = (start+end)/2;
	index_f tmp_indexes[248];
	memset(tmp_indexes, 0, sizeof(tmp_indexes));
	if (key < (I->indexes)[0].key){
		memcpy(tmp_indexes, I->indexes, sizeof(index_f) * number_of_keys);
		(I->indexes)[0].key = key;
		(I->indexes)[0].page_offset = page_offset;
		memcpy((I->indexes)+1, tmp_indexes, sizeof(index_f) * number_of_keys);
		I->number_of_keys = number_of_keys + 1;
		return 0;
	} else if (key > (I->indexes)[end].key){
		(I->indexes)[number_of_keys].key = key;
		(I->indexes)[number_of_keys].page_offset = page_offset;
		I->number_of_keys = number_of_keys + 1;
		return 0;
	} else {
		while(start<=end){
			if((I->indexes)[mid].key > key){
				end = mid-1;
				mid = (start+end)/2;
			} else {
				start = mid + 1;
				mid = (start+end)/2;
			}
		}
	}
	int number_of_copy = number_of_keys - start;
	memcpy(tmp_indexes, (I->indexes)+start, sizeof(index_f) * number_of_copy);
	(I->indexes)[start].key = key;
	(I->indexes)[start].page_offset = page_offset;
	memcpy((I->indexes)+start+1, tmp_indexes, sizeof(index_f) * number_of_copy);
	I -> number_of_keys = number_of_keys + 1;
	return 0;
}
/**
	이거 r_L 은 세팅 끝내서 파일에 써놓고 L 도 세팅 다 끝냈음. L 을 쓰는건 밖에서 쓰는걸로 하고
	리턴 값은 올라갈 r_L 에서 가장 왼쪽에 있는 값임.
**/
int64_t split_in_leaf(leaf_node_f* L, int64_t key, char* value,off_t off_parent){
	off_t new_page = getNewPage(); // r_L 의 offset.
	leaf_node_f* r_L = getNewLeaf();
	r_L->right_page_offset = L->right_page_offset;
	L->right_page_offset = new_page;

	int64_t r_V = (L->records)[15].key;
	r_L->parent_page_offset = off_parent;
	L->parent_page_offset = off_parent;
	r_L -> number_of_keys =16;
	L->number_of_keys = 16;
	if(key > r_V){
		memcpy(r_L->records,(L->records)+16,sizeof(record_f)*15);
		memset((L->records)+16,0,sizeof(record_f)*15);
		r_L->number_of_keys = 15;
		insert_in_leaf(r_L,key,value);
	} else {
		memcpy(r_L->records,(L->records)+15,sizeof(record_f)*16);
		memset((L->records)+15,0,sizeof(record_f)*16);
		L->number_of_keys=15;
		insert_in_leaf(L,key,value);
	}
	fseek(fp,new_page,SEEK_SET);
	fwrite(r_L,BSZ,1,fp);
	r_V = (r_L->records)[0].key;
	free(r_L);
	return r_V; // 부모로 올려야할 key value 리턴.
}

/**
	인덱스 에서 스플릿 날때임.
	r_I 의 offset 을 파라미터로 넘겨주고, 부모에 넣어야할 값 V 를 리턴해줌.
**/
int64_t split_in_index(internal_node_f* I, int64_t V, int64_t key, off_t page_offset, off_t parent_offset, off_t* off_r_I){ //TODO::10/25
	internal_node_f* r_I = getNewInter();
	off_t off_new = getNewPage();
	*off_r_I = off_new;
	// 이 branch 에서 I 노드와 r_I 노드를 다 바꾸어 주었어.
	I->number_of_keys = 124;
	r_I->number_of_keys = 124;
	if (key == V){
		r_I -> left_most_offset = page_offset;
		//memcpy((I->indexes)+124, r_I->indexes, sizeof(index_f)*124);
		memcpy(r_I->indexes, (I->indexes)+124, sizeof(index_f)*124);
		memset((I->indexes)+124,0,sizeof(index_f)*124);
	} else if (key > (I->indexes)[I->number_of_keys/2].key){ // 125 번째가 올라가는경우
		r_I -> left_most_offset = (I->indexes)[124].page_offset;
		//memcpy((I->indexes)+125, r_I->indexes, sizeof(index_f)*123);
		memcpy(r_I->indexes, (I->indexes)+125, sizeof(index_f)*123);
		memset((I->indexes)+124,0,sizeof(index_f)*124);
		r_I->number_of_keys = 123;
		insert_in_index(r_I,key,page_offset);
	} else { // 124 번째가 올라가는 경우. key 가 124번째보다 작을때.
		r_I -> left_most_offset = (I->indexes)[123].page_offset;
		//memcpy((I->indexes)+124, r_I->indexes, sizeof(index_f)*124);
		memcpy(r_I->indexes, (I->indexes)+124, sizeof(index_f)*124);
		memset((I->indexes)+123, 0, sizeof(index_f)*125);
		I->number_of_keys = 123;
		insert_in_index(I, key, page_offset);
	}
	
	r_I->parent_page_offset = parent_offset;

	// 새로 만들어진 node로 옮겨진 자식들이 가르키는 그 자식들의 부모 노드 수정해주기.
	internal_node_f* child = (internal_node_f*)malloc(sizeof(internal_node_f));
	memset(child,0,BSZ);
	off_t off_child = r_I -> left_most_offset;
	fseek(fp,off_child,SEEK_SET);
	fread(child,BSZ,1,fp);
	child->parent_page_offset = off_new;
	fseek(fp,off_child,SEEK_SET);
	fwrite(child,BSZ,1,fp);
	for(int i=0; i<r_I->number_of_keys;++i){
		off_child = (r_I->indexes)[i].page_offset;
		fseek(fp,off_child,SEEK_SET);
		fread(child,BSZ,1,fp);
		child->parent_page_offset=off_new;
		fseek(fp,off_child,SEEK_SET);
		fwrite(child,BSZ,1,fp);
	}
	
	fseek(fp,off_new,SEEK_SET);
	fwrite(r_I,BSZ,1,fp);
	free(r_I);
	return V;
}

//Leaf insert 에서 split 상황에 현재의 L 이 부모가 있을때 이제 그 부모를 나눌때의 함수를 재귀로 구현.
int insert_entry_index(internal_node_f* I,off_t I_offset, int64_t key, off_t page_offset){
	//int64_t V= find_V_index(I,key);
	off_t off_parent = I->parent_page_offset;
	off_t off_r_I;
	if(I->number_of_keys < 248){ // 나눌필요가 없을떄 넣고 쓰고 끝냄.
		insert_in_index(I,key,page_offset);
		fseek(fp,I_offset,SEEK_SET);
		fwrite(I,BSZ,1,fp);
		free(I);
		return 0;
	} else { //mSPLIT 상황 //TODO:: 10/26 에러.
		if (I->parent_page_offset == 0){
			off_parent = getNewPage();
		}
		int64_t V = find_V_index(I,key);
		split_in_index(I, V, key, page_offset, off_parent, &off_r_I);

		if (I->parent_page_offset == 0){ // 루트 일때는 부모노드 만들어서 V` 값 올리고 끝 !.
			I->parent_page_offset = off_parent;
			internal_node_f* root = getNewInter();
			header_page* HEAD = getHeader();
			HEAD -> root_page = off_parent;
			fseek(fp,0,SEEK_SET);
			fwrite(HEAD,BSZ,1,fp);
			free(HEAD);
			root->left_most_offset = I_offset;
			insert_in_index(root, V, off_r_I);
			fseek(fp,off_parent,SEEK_SET);
			fwrite(root,BSZ,1,fp);
			fseek(fp,I_offset,SEEK_SET);
			fwrite(I,BSZ,1,fp);
			free(root);
			free(I);
			return 0;
		}else { // 루트아닐때는 V` 부모에 삽입 재귀적으로 구현.
			internal_node_f* parent = getNewInter();
			fseek(fp,off_parent,SEEK_SET);
			fread(parent, BSZ,1, fp);
			fseek(fp,I_offset,SEEK_SET);
			fwrite(I,BSZ,1,fp);
			insert_entry_index(parent, off_parent, V, off_r_I);
			free(I);
			return 0;
		}
	}
	return 0;
}

/**
	INSERT 함수.
 **/
int insert(int64_t key, char* value){
	if (find(key) != NULL) return -1; // no dup.
	off_t L_offset = 0;
	leaf_node_f* L = findLeaf(key, &L_offset);
	internal_node_f* I;
	header_page* HEAD;
	if(L == NULL) { // Empty tree : 새로만들고 그거 루트로 넣고 종료.
		off_t off_new = getNewPage();
		free(L);
		L = getNewLeaf();
		insert_in_leaf(L,key,value); // 리프에다가 넣기.
		fseek(fp,off_new,SEEK_SET);
		fwrite(L,BSZ,1,fp);
		HEAD = getHeader();
		HEAD -> root_page = off_new; // 빈트리에 넣을때 루트페이지 어딘지 정해주기
		fseek(fp,0,SEEK_SET);
		fwrite(HEAD,BSZ,1,fp);
		free(L);
		free(HEAD);
		return 0;
	}
	off_t off_parent = L->parent_page_offset;
	off_t off_right;
	int64_t r_V = 0;
	if (L->number_of_keys < 31){ // 그냥 넣기만 하면 되는 경우.
		insert_in_leaf(L, key, value);
		fseek(fp,L_offset,SEEK_SET);
		fwrite(L,BSZ,1,fp);
		free(L);
		return 0;
	} else { // Split. Leaf 가 root 인 경우 / root 가 아닌경우
		if(L->parent_page_offset == 0) { // L 이 루트인경우 !! 새로운 부모를 만들고 이녀석을 root 로 만들어버리.
			off_parent=getNewPage();
			r_V = split_in_leaf(L,key,value,off_parent);
			off_right = L->right_page_offset;
			I = getNewInter(); // 이 I 가 부모노드임.
			I -> left_most_offset =  L_offset;// L's offset.
			insert_in_index(I,r_V,off_right); // 부모노드에다가 값 넣기.
			HEAD = getHeader();
			HEAD->root_page = off_parent;
			fseek(fp,0,SEEK_SET);
			fwrite(HEAD,sizeof(header_page),1,fp);
			fseek(fp,off_parent,SEEK_SET);
			fwrite(I,BSZ,1,fp);
			fseek(fp,L_offset,SEEK_SET);
			fwrite(L,BSZ,1,fp);
			free(I);
			free(L);
			free(HEAD);
			return 0;
		} else { // L이 root 가 아닐때의 경우. 흠.
			r_V = split_in_leaf(L,key,value,off_parent);
			off_right = L->right_page_offset;
			fseek(fp,L_offset,SEEK_SET);
			fwrite(L,BSZ,1,fp);
			I = (internal_node_f*)malloc(sizeof(internal_node_f));
			memset(I,0,BSZ);
			fseek(fp,L->parent_page_offset,SEEK_SET);
			fread(I,BSZ,1,fp);
			insert_entry_index(I,L->parent_page_offset,r_V,off_right);
			free(L);			
		}
	}
	return 0;
}

// DELETION 시작 지점.

/**
페이지 삭제.
페이지 개수가 0이라면 그냥 초기화 시키듯 해버려줌.
그거 아니라면 해드가 가르키던걸 tmp 에 넣고 해드가 tmp를 가르키게함.
전체 페이지 개수 1개 줄임. 
그리고 지워지는 녀석하고 해드 파일에다가 씀!!.
HEAD 전역으로 선언해서 하는게 더 편하겠네ㅔㅔㅔㅔㅔㅔㅔㅔㅔㅔㅔㅔㅔㅔㅔㅔ
**/
void deletePage(off_t off_delete){
	header_page* HEAD = getHeader();
	leaf_node_f* tmp = getNewLeaf();
	tmp->parent_page_offset = HEAD -> free_page;
	HEAD -> free_page = off_delete;
	(HEAD -> number_of_pages)--;
	if (HEAD -> number_of_pages == 0){
		HEAD -> root_page = 0;
		HEAD -> free_page = 4096;
	}
	fseek(fp,0,SEEK_SET);
	fwrite(HEAD, sizeof(header_page), 1, fp);
	fseek(fp, off_delete, SEEK_SET);
	fwrite(tmp, BSZ, 1, fp);
	free(HEAD);
	free(tmp);
	return 0;
}
int find_i_index_V(internal_node_f* parent, off_t off_L){
	index_f tmp[248];
	int number_of_keys = parent -> number_of_keys;
	memset(tmp, 0, sizeof(tmp));
	memcpy(tmp, parent -> indexes, sizeof(index_f) * number_of_keys);
	// 아 이거는 페이지 뒤죽박죽이라 순회밖에 안되네 
	for (int i = 0; i < number_of_keys; ++i){
		if(off_L == tmp[i].page_offset) return i;
	}
	return -1; // 없을경우 근데 이거 리턴되면 에러임.
}
void findValue(leaf_node_f* L,off_t off_L, off_t off_parent, off_t* off_right, off_t* off_left, int64_t* key_R, int64_t* key_L){
	internal_node_f* parent = getNewInter();
	fseek(fp, off_parent, SEEK_SET);
	fread(parent, BSZ, 1, fp);
	int i = find_i_index_V(parent, off_L);
	
	if(i==0){
		*off_left = parent -> left_most_offset;
		*key_L = (parent->indexes)[0];
		*off_right = (parent->indexes)[1].page_offset;
		*key_R = (parent->indexes)[1].key;
	} else if (i == number_of_keys - 1){
		*off_left = (parent->indexes)[i-1].page_offset;
		*key_L = (parent->indexes)[i].key;
	} else {
		*off_left = (parent -> indexes)[i-1].page_offset;
		*key_L = (parent->indexes)[i].key;
		*off_right = (parent->indexes)[i+1].page_offset;
		*key_R = (parent->indexes)[i].key;
	}
	free(parent);
	return;
}
/**
리프노드에서 그냥 지워주기만 할때 병합이나 재분배 필요없이 
지우면서 종료시켜준다. 기록은 밖에서함.
못찾을 경우 -1 리턴. 잘지우면 0 리턴.
**/
int delete_in_leaf(leaf_node_f* L, int64_t key, off_t L_offset){
	int i = search_leaf(key, L->records, L->number_of_keys);
	if(i==-1) return -1;
	int number_of_copy = L->number_of_keys - i - 1;
	record_f tmp_records[31];
	memset(tmp_records,0,sizeof(tmp_records));
	memcpy(tmp_records, (L->records)+i+1, sizeof(record_f) * number_of_copy);
	memset((L->records)+i,0,sizeof(record_f) * (number_of_copy+1));
	memcpy((L->records)+i, tmp_records, sizeof(record_f) * number_of_copy);
	(L->number_of_keys)--;
	return 0;
}

// DELETE 
int delete(int64_t key){
	off_t L_offset = 0;
	leaf_node_f* L = findLeaf(key, &L_offset);
	internal_node_f* I;
	header_page* HEAD;
	if(L == NULL) { // 빈노드일때
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
			fwrite(L,BSZ,1,fp);
		}
		free(L);
		return 0;
	}
	off_t off_right=0;
	off_t off_left=0;
	int64_t key_R = 0; // 이거 부모 양쪾포인터 사이에있는거.
	int64_t key_L = 0;
	findValue(L,L_offset, L->parent_page_offset, &off_right, &off_left, &key_R, &key_L);
	leaf_node_f* left_node;
	leaf_node_f* right_node;
	left_node = getNewInter();
	fseek(fp, off_left, SEEK_SET);
	fread(left_node, BSZ, 1, fp);
	if(off_right != 0){
		right_node = getNewInter();
		fseek(fp, off_right, SEEK_SET);
		fread(right_node, BSZ, 1, fp);
	}
	// 왼쪽 녀석 오른쪽 녀석( 오른쪽은 없을수도 있어서 이래함.

	if ((L->number_of_keys) > floor(31)){ //No need to merge or redistribute. JUST DELETE !!.
		if(delete_in_leaf(L, key, L_offset) == -1) return -1;
		fseek(fp,L_offset,SEEK_SET);
		fwrite(L,BSZ,1,fp);
		free(L);
		return 0;
	} else { // 병합하거나 재분배해야하는 경우 까다로움. 그 재분배부터. 
		



	}
	return 0;
}





