#include "bpt_internal.h"
#include "types.h"


int update_record(Table* table, int64_t key, int64_t* value, trx_t* trx) {
	int i = 0;
	int buf_page_i = 0;
	LeafPage* leaf_node;

	bool get_page_latch;
	int lock_req_ret;

	
	while (true) {
		
		get_page_latch = find_leaf(table, key, &leaf_node, &buf_page_i);
		
		if (!get_page_latch) {
			if (buf_page_i != BUF_PAGE_MUTEX_FAIL)
				return -1;
			else
				continue;
		}

		lock_req_ret = lock_sys.acquire_lock(trx, table->table_id, pool.pages[buf_page_i].pagenum, key, LOCK_S, buf_page_i);

		if (lock_req_ret == LOCK_SUCCESS)
			break;

		else if (lock_req_ret == DEADLOCK) {
			release_page((Page*)leaf_node, &buf_page_i);
			return -1;
		
		} else {
			release_page((Page*)leaf_node, &buf_page_i);
			trx->trx_wait_lock();
		}
	}

	for (i = 0; i < leaf_node->num_keys; i++) {
		if (LEAF_KEY(leaf_node, i) == key) {
			trx -> push_undo_log(table->table_id, key, leaf_node -> pagenum, LEAF_VALUE(leaf_node, i));

			memcpy(LEAF_VALUE(leaf_node, i), value, SIZE_VALUE);
			release_page((Page*)leaf_node, &buf_page_i);
			return 0;
		}
	}

	release_page((Page*)leaf_node, &buf_page_i);
	return -1;
}
