#include "bpt_internal.h"


int update_record(Table* table, int64_t key, const char* value) {
	int i = 0;
	
	LeafPage* leaf_node;
	if (!find_leaf(table, key, &leaf_node)) {
		return -1;
	}
	
	for (i = 0; i < leaf_node->num_keys; i++) {
		if (LEAF_KEY(leaf_node, i) == key) {
			memcpy(LEAF_VALUE(leaf_node, i), value, SIZE_VALUE);
			release_page((Page*)leaf_node);
			return 0;
		}
	}

	return -1;
}
