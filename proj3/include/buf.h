#ifndef __BUF_H__
#define __BUF_H__
#include <mutex>

#include "table.h"
#include "page.h"

typedef struct _BufPool {
    Page* pages;
		// Buffer Pool Mutex.
		std::mutex buf_pool_mutex;	// Buffer Pool Mutex.

		// Buffer page Mutex.
		std::mutex* buf_page_mutex; 	

    Page* lru_head;
    Page* lru_tail;
    int num_buf;
    int tot_pincnt;
} BufPool;

void init_new_header_page(HeaderPage *header_page, int table_id, int num_column);

void load_header_page(Table *table);

Page* alloc_page(Table *table);

Page* get_page(Table *table, pagenum_t pagenum);

Page* get_page(Table *table, pagenum_t pagenum, int* buf_page_i);

void release_header_page(Table *table);

void release_page(Page* page);

void release_page(Page* page, int* buf_page_i);

void set_dirty_page(Page* page);

void free_page(Table *table, Page* page);

void flush_table(Table *table);

int init_buf_pool(int num_buf);

int destroy_buf_pool();

void print_buf_pool();

#define BUF_PAGE_MUTEX_FAIL -930209

#define BUF_POOL_MUTEX_ENTER \
	pool.buf_pool_mutex.lock();

#define BUF_POOL_MUTEX_EXIT \
	pool.buf_pool_mutex.unlock();

#define BUF_PAGE_MUTEX_ENTER(i)\
	bool ret = pool.buf_page_mutex[i].try_lock();


#endif /* __BUF_H__ */
