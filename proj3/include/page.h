#ifndef __PAGE_H__
#define __PAGE_H__
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

#define BPTREE_INTERNAL_ORDER       249
#define BPTREE_LEAF_ORDER           32

#define PAGE_SIZE                   4096

#define SIZE_KEY                    8
#define SIZE_COLUMN  								15
#define SIZE_VALUE                  120
#define SIZE_RECORD                 (SIZE_KEY + SIZE_VALUE)

#define BPTREE_MAX_NODE             (1024 * 1024) // for queue

typedef uint64_t pagenum_t;
#define PAGENUM_TO_FILEOFF(pgnum)   ((pgnum) * PAGE_SIZE)
#define FILEOFF_TO_PAGENUM(off)     ((pagenum_t)((off) / PAGE_SIZE))

/* Type representing the record
 * to which a given key refers.
 * In a real B+ tree system, the
 * record would hold data (in a database)
 * or a file (in an operating system)
 * or some other information.
 * Users can rewrite this part of the code
 * to change the type and content
 * of the value field.
 */
typedef struct _Record {
    int64_t key;
    int64_t value[SIZE_COLUMN];
} Record;

typedef struct _InternalRecord {
    int64_t key;
    off_t offset;
} InternalRecord;

#define INMEM_PAGE \
    int table_id; \
    pagenum_t pagenum; \
    bool is_dirty; \
    int pincnt; \
    struct _Page* lru_next; \
    struct _Page* lru_prev

typedef struct _Page {
    char bytes[PAGE_SIZE];
    
    // in-memory data
    INMEM_PAGE;
} Page;

typedef struct _FreePage {
    off_t next;
    char reserved[PAGE_SIZE - 8];

    // in-memory data
    INMEM_PAGE;
} FreePage;

typedef struct _HeaderPage {
    off_t freelist;
    off_t root_offset;
    uint64_t num_pages;
		off_t num_column;
    char reserved[PAGE_SIZE - 32];

    // in-memory data
    INMEM_PAGE;
} HeaderPage;

#define INTERNAL_KEY(n, i)    ((n)->irecords[(i)+1].key)
#define INTERNAL_OFFSET(n, i) ((n)->irecords[(i)].offset)
typedef struct _InternalPage {
    union {
        struct {
            off_t parent;
            int is_leaf;
            int num_keys;
            char reserved[112 - 16];
            InternalRecord irecords[BPTREE_INTERNAL_ORDER];
        };
        char space[PAGE_SIZE];
    };
    // in-memory data
    INMEM_PAGE;
} InternalPage;

#define LEAF_KEY(n, i)      ((n)->records[(i)].key)
#define LEAF_VALUE(n, i)    ((n)->records[(i)].value)
typedef struct _LeafPage {
    union {
        struct {
            off_t parent;
            int is_leaf;
            int num_keys;
            char reserved[120 - 16];
            off_t sibling;
            Record records[BPTREE_LEAF_ORDER-1];
        };
        char space[PAGE_SIZE];
    };

    // in-memory data
    INMEM_PAGE;
} LeafPage;

typedef struct _NodePage {
    union {
        struct {
            off_t parent;
            int is_leaf;
            int num_keys;
        };
        char space[PAGE_SIZE];
    };

    // in-memory data
    INMEM_PAGE;
} NodePage;

#endif /* __PAGE_H__ */
