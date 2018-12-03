#include "bpt_internal.h"


int update_record(Table* table, int64_t key, int64_t* value, trx_t* trx) {
	int i = 0;
	int buf_page_i = 0;
	LeafPage* leaf_node;

	while (!find_leaf(table, key, &leaf_node, &buf_page_i)) {
		if (buf_page_i != BUF_PAGE_MUTEX_FAIL)
			return -1;
	}
	
	if (!lock_sys.acquire_lock(trx, table->table_id, pool.pages[buf_page_i].pagenum, key, LOCK_X, buf_page_i)) {
		release_page((Page*)leaf_node, &buf_page_i);
		trx->setState(ABORTED);
		return -1;
	}
	
	for (i = 0; i < leaf_node->num_keys; i++) {
		if (LEAF_KEY(leaf_node, i) == key) {
			memcpy(LEAF_VALUE(leaf_node, i), value, SIZE_VALUE);
			release_page((Page*)leaf_node, &buf_page_i);
			return 0;
		}
	}
	
	release_page((Page*)leaf_node, &buf_page_i);
	return -1;
}
