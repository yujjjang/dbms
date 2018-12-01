#include "bpt_internal.h"

/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
bool find_leaf(Table* table, int64_t key, LeafPage** out_leaf_node) {
		// unique_lock<mutex> buf_latch(pool.buf_pool_mutex);
		
    int i = 0;
    off_t root_offset = table->dbheader.root_offset;

    if (root_offset == 0) {
        return false;
    }
    
    NodePage* page;
    page = (NodePage*)get_page(table, FILEOFF_TO_PAGENUM(root_offset));

    while (!page->is_leaf) {
        InternalPage* internal_node = (InternalPage*)page;

        i = 0;
        while (i < internal_node->num_keys) {
            if (key >= INTERNAL_KEY(internal_node, i)) i++;
            else break;
        }
        
        NodePage* nextPage = (NodePage*)get_page(table, 
                FILEOFF_TO_PAGENUM(INTERNAL_OFFSET(internal_node, i)));
        if (nextPage->is_leaf == 0 && nextPage->num_keys == 0){
            PANIC("find_leaf infinite loop!\n");
        }
        release_page((Page*)page);
        page = nextPage;
                       
    }

    *out_leaf_node = (LeafPage*)page;

    return true;
}

/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 * Function when executing in transaction.
 */

bool find_leaf(Table* table, int64_t key, LeafPage** out_leaf_node, int* buf_page_i) {
		// unique_lock<mutex> buf_latch(pool.buf_pool_mutex);
		BUF_POOL_MUTEX_ENTER;
    int i = 0;
    off_t root_offset = table->dbheader.root_offset;

    if (root_offset == 0) {
        return false;
    }
    
    NodePage* page;
    page = (NodePage*)get_page(table, FILEOFF_TO_PAGENUM(root_offset), buf_page_i);

    while (!page->is_leaf) {
        InternalPage* internal_node = (InternalPage*)page;

        i = 0;
        while (i < internal_node->num_keys) {
            if (key >= INTERNAL_KEY(internal_node, i)) i++;
            else break;
        }
        
        NodePage* nextPage = (NodePage*)get_page(table, 
                FILEOFF_TO_PAGENUM(INTERNAL_OFFSET(internal_node, i)), buf_page_i);
				
				// Return nullptr means cannot acquire the latch on that page. [BUF_PAGE_MUTEX_ENTER_FAIL].
				if (!nextPage) {
					release_page((Page*)page);
					*buf_page_i = BUF_PAGE_MUTEX_FAIL;
					return false;
				}
        if (nextPage->is_leaf == 0 && nextPage->num_keys == 0){
            PANIC("find_leaf infinite loop!\n");
        }
        release_page((Page*)page);
        page = nextPage;
                       
    }

    *out_leaf_node = (LeafPage*)page;

    return true;
}

/* Finds and returns the value to which
 * a key refers.
 */
char* find_record(Table* table, int64_t key) {
    int i = 0;
    char* out_value;

    LeafPage* leaf_node;
    if (!find_leaf(table, key, &leaf_node)) {
        return NULL;
    }
		
    for (i = 0; i < leaf_node->num_keys; i++) {
        if (LEAF_KEY(leaf_node, i) == key) {
            out_value = (char*)malloc(SIZE_VALUE * sizeof(char));
            memcpy(out_value, LEAF_VALUE(leaf_node, i), SIZE_VALUE);
            release_page((Page*)leaf_node);
            return out_value;
        }
    }

    release_page((Page*)leaf_node);
    return NULL;
}

/* Finds and returns the value to which
 * a key refers.
 * Function when executing in transaction.
 */

char* find_record(Table* table, int64_t key, trx_t* trx) {
    int i = 0;
		int buf_page_i = 0;
    char* out_value;

    LeafPage* leaf_node;
	
		// Find leaf returns false when there is no root node
		// 				or the page latch is already granted by other.
		while (!find_leaf(table, key, &leaf_node, &buf_page_i)) {
			if (buf_page_i != BUF_PAGE_MUTEX_FAIL)
				return NULL;
		}
		
		// Transaction get the buf_page_mutex.
		// Buffer block index  = buf_page_i.

		if (lock_sys->acquire_lock()) {
			// OK
		} else {
			// DEADLOCK -> ABORT !
			return NULL;
		}
		
    for (i = 0; i < leaf_node->num_keys; i++) {
        if (LEAF_KEY(leaf_node, i) == key) {
            out_value = (char*)malloc(SIZE_VALUE * sizeof(char));
            memcpy(out_value, LEAF_VALUE(leaf_node, i), SIZE_VALUE);
            release_page((Page*)leaf_node, &buf_page_i);
            return out_value;
        }
    }

    release_page((Page*)leaf_node, &buf_page_i);
    return NULL;
}

/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int cut( int length ) {
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}
