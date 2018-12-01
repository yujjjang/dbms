#include "bpt_internal.h"


int init_db(int num_buf) {
    return init_buf_pool(num_buf);
}

int shutdown_db() {
    return destroy_buf_pool();
}

// Opens a db file. Creates a file if not exist.
int open_table(char* filename, int num_column) {
    return open_or_create_table_file(filename, num_column);
}

// Closes the db file.
int close_table(int table_id) {
    Table* table = get_table(table_id);
    flush_table(table);
    return close_table_file(table);
}


// TODO
/* Inserts a new record.
 */
int insert(int table_id, int64_t key, int64_t* value) {
    Table* table = get_table(table_id);
    return insert_record(table, key, value);
}

/* Deletes a record.
 */
int remove(int table_id, int64_t key) {
    Table* table = get_table(table_id);
    return delete_record(table, key);
}

/* Finds a value from key in the b+ tree.
 */
int64_t* find(int table_id, int64_t key) {
    Table* table = get_table(table_id);
    
		return find_record(table, key);
}

int64_t* find(int table_id, int64_t key, int trx_id, int* result) {
	Table* table = get_table(table_id);
	TransactionManager* tm = &trx_sys;
	trx_t* trx = tm -> getTransaction(trx_id);
	
	if (trx -> getState() == ABORTED) {
		*result = 0;
		return NULL;
	}
	*result = 1;
	return find_record(table, key, trx);
}

/* Update a record old value to new value.
 */
int update(int table_id, int64_t key, int64_t* value, int trx_id, int* result) {
	Table* table = get_table(table_id);
	TransactionManager* tm = &trx_sys;
	
	trx_t* trx = tm -> getTransaction(trx_id);
	
	if (trx -> getState() == ABORTED) {
		*result = 0;
		return -1;
	}
	*result = 1;
	return update_record(table, key, value, trx);
}

/* Begin a new transaction.
 * Allocate transaction id from Transaction Manager. 
 * @return trx*.
 **/
int begin_tx() {
	TransactionManager* tm = &trx_sys;
	trx_t* new_t = tm -> makeNewTransaction();

	return new_t -> getTransactionId();
}

/**
	* End the transaction whose trx_id is given transaction id.
	* 1. Commit : If state is not ABORTED, Release all locks &  delete transaction object.
	* 2. Abort :  If state is ABORTED, just delete transaction object.
	*	
	*/
int end_tx(int trx_id) {
	TransactionManager* tm = &trx_sys;
	LockManager* lm = &lock_sys;

	trx_t* end_t = tm -> getTransaction(trx_id);
	bool ret;
	if (end_t -> getState() == ABORTED) {
		ret = tm->deleteTransaction(trx_id);
	} else {
		end_t -> setState(NONE);
		ret = lm->release_lock(end_t) && tm -> deleteTransaction(trx_id);
	}
	if (!ret)
		PANIC("end_tx.\n");

	return 0;
}

/* Prints the B+ tree in the command
 * line in level (rank) order, with the 
 * keys in each node and the '|' symbol
 * to separate nodes.
 * With the verbose_output flag set.
 * the values of the pointers corresponding
 * to the keys also appear next to their respective
 * keys, in hexadecimal notation.
 */
void print_tree(int table_id) {
    off_t* queue;
    int i;
    int front = 0;
    int rear = 0;

    Table* table = get_table(table_id);

    print_buf_pool();
    if (table->dbheader.root_offset == 0) {
        printf("Empty tree.\n");
        return;
    }
    queue = (off_t*)malloc(sizeof(off_t) * BPTREE_MAX_NODE);

    queue[rear] = table->dbheader.root_offset;
    rear++;
    queue[rear] = 0;
    rear++;
    while (front < rear) {
        off_t page_offset = queue[front];
        front++;

        if (page_offset == 0) {
            printf("\n");
            
            if (front == rear) break;

            // next tree level
            queue[rear] = 0;
            rear++;
            continue;
        }
        
        NodePage* node_page;
        node_page = (NodePage*)get_page(table, FILEOFF_TO_PAGENUM(page_offset));
        if (node_page->is_leaf == 1) {
            // leaf node
            LeafPage* leaf_node = (LeafPage*)node_page;
            for (i = 0; i < leaf_node->num_keys; i++) {
                printf("%" PRIu64 " ", LEAF_KEY(leaf_node, i));
            }
            printf("| ");
        } else {
            // internal node
            InternalPage* internal_node = (InternalPage*)node_page;
            for (i = 0; i < internal_node->num_keys; i++) {
                printf("%" PRIu64 " ", INTERNAL_KEY(internal_node, i));
                queue[rear] = INTERNAL_OFFSET(internal_node, i);
                NodePage* child_node = (NodePage*) get_page(table,
                        FILEOFF_TO_PAGENUM(INTERNAL_OFFSET(internal_node, i)));
                if (child_node->parent != page_offset){
                    PANIC("btree structure crashed");
                }
                release_page((Page*)child_node);
                rear++;
            }
            queue[rear] = INTERNAL_OFFSET(internal_node, i);
            rear++;
            printf("| ");
        }
        release_page((Page*)node_page);
    }
    free(queue);
}

/* Finds the record under a given key and prints an
 * appropriate message to stdout.
 */
void find_and_print(int table_id, int64_t key) {
    int64_t* value_found = NULL;
    value_found = find(table_id, key);
    if (value_found == NULL) {
        printf("Record not found under key %" PRIu64 ".\n", key);
    }
    else {
        printf("key %" PRIu64 ", value [%s].\n", key, value_found);
        free(value_found);
    }
}

/* Copyright and license notice to user at startup. 
 */
void license_notice( void ) {
    printf("bpt version %s -- Copyright (C) 2010  Amittai Aviram "
            "http://www.amittai.com\n", Version);
    printf("This program comes with ABSOLUTELY NO WARRANTY; for details "
            "type `show w'.\n"
            "This is free software, and you are welcome to redistribute it\n"
            "under certain conditions; type `show c' for details.\n\n");
}

/* Routine to print portion of GPL license to stdout.
 */
void print_license( int license_part ) {
    int start, end, line;
    FILE * fp;
    char buffer[0x100];

    switch(license_part) {
    case LICENSE_WARRANTEE:
        start = LICENSE_WARRANTEE_START;
        end = LICENSE_WARRANTEE_END;
        break;
    case LICENSE_CONDITIONS:
        start = LICENSE_CONDITIONS_START;
        end = LICENSE_CONDITIONS_END;
        break;
    default:
        return;
    }

    fp = fopen(LICENSE_FILE, "r");
    if (fp == NULL) {
        perror("print_license: fopen");
        exit(EXIT_FAILURE);
    }
    for (line = 0; line < start; line++)
        fgets(buffer, sizeof(buffer), fp);
    for ( ; line < end; line++) {
        fgets(buffer, sizeof(buffer), fp);
        printf("%s", buffer);
    }
    fclose(fp);
}

/* First message to the user.
 */
void usage_1( void ) {
    printf("B+ Tree of Order %d(Internal).\n", order_internal);
    printf("Following Silberschatz, Korth, Sidarshan, Database Concepts, "
           "5th ed.\n\n");
}

/* Second message to the user.
 */
void usage_2( void ) {
    printf("Enter any of the following commands after the prompt > :\n"
    "\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
    "\tf <k>  -- Find the value under key <k>.\n"
    "\tp <k> -- Print the path from the root to key k and its associated "
           "value.\n"
    "\td <k>  -- Delete key <k> and its associated value.\n"
    "\tt -- Print the B+ tree.\n"
    "\tq -- Quit. (Or use Ctl-D.)\n"
    "\t? -- Print this help message.\n");
}

