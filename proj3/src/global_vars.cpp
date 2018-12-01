#include "bpt_internal.h"

/* The order determines the maximum and minimum number of entries (keys and
 * pointers) in any node.  Every node has at most order - 1 keys and at least
 * (roughly speaking) half that number. Every leaf has as many pointers to
 * data as keys, and every internal node has one more pointer to a subtree
 * than the number of keys. This global variable is initialized to the
 * default value.
 */
int order_internal = BPTREE_INTERNAL_ORDER;
int order_leaf = BPTREE_LEAF_ORDER;
