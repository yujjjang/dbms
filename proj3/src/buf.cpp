#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "page.h"
#include "file.h"
#include "buf.h"
#include "panic.h"

BufPool pool;

/* Flush the page.
 */
void flush_page(Page* page) {
    file_write_page(get_table(page->table_id), page->pagenum, page);
    page->is_dirty = false;
}

/* Pop the page from lru.
 */
void pop_from_lru(Page* page){
    if (page == pool.lru_head || page == pool.lru_tail){
        if (page == pool.lru_head){
            pool.lru_head = page->lru_next;
            if (page->lru_next)
                page->lru_next->lru_prev = NULL;
        }
        if (page == pool.lru_tail){
            pool.lru_tail = page->lru_prev;
            if (page->lru_prev)
                page->lru_prev->lru_next = NULL;
        }
    }
    else{
        page->lru_prev->lru_next = page->lru_next;
        page->lru_next->lru_prev = page->lru_prev;
    }

    page->lru_prev = page->lru_next = NULL;

}

/* Push the page to lru list.
 */
void push_to_lru(Page* page){
    page->lru_next = pool.lru_head;
    if (page->lru_next){
        page->lru_next->lru_prev = page;
    }
    page->lru_prev = NULL;
    pool.lru_head = page;
    if (pool.lru_tail == NULL){
        pool.lru_tail = page;
    }
}

/* Evict the page from the lru list.
 */
Page* evict_page(){
    Page* page = pool.lru_tail;
    while(page != NULL){
        if (page->pincnt != 0){
            page = page->lru_prev;
            continue;
        }
        pop_from_lru(page);
        if (page->is_dirty)
            flush_page(page);
        page->lru_next = page->lru_prev = NULL;
        return page;
    }
    PANIC("evict_page");
    exit(1);
}

/* Pop and push the page in the lru list.
 */
void update_lru(Page* page){
    pop_from_lru(page);
    push_to_lru(page);
}

/* Initialize header page.
 */
void init_new_header_page(HeaderPage* header, int table_id, int num_column) {
    memset(header, 0, sizeof(HeaderPage));

    header->table_id = table_id;
    header->pagenum = 0;
    header->freelist = 0;
    header->root_offset = 0;
		header->num_column = num_column;
    header->num_pages = 1;
    set_dirty_page((Page*)header);
    header->pincnt++;
    pool.tot_pincnt++;
}

/* Load header page.
 */
void load_header_page(Table* table) {
    file_read_page(table, 0, &table->dbheader);
    table->dbheader.table_id = table->table_id;
    table->dbheader.pagenum = 0;
    table->dbheader.is_dirty = false;
    table->dbheader.pincnt = 1;
    table->dbheader.lru_next = NULL;
    table->dbheader.lru_prev = NULL;
    pool.tot_pincnt++;
}

/* Allocate new page.
 */
Page* alloc_page(Table* table) {
    pagenum_t pagenum = FILEOFF_TO_PAGENUM(table->dbheader.freelist);
    FreePage* freepage;
    if (pagenum == 0){
        expand_file(table, table->dbheader.num_pages);
        pagenum = FILEOFF_TO_PAGENUM(table->dbheader.freelist);
    }
    freepage = (FreePage*)get_page(table, pagenum); 
    table->dbheader.freelist = freepage->next;

    set_dirty_page((Page*)&table->dbheader);
    memset(freepage, 0, PAGE_SIZE);

    set_dirty_page((Page*)freepage);
    return (Page*)freepage;
}

/* Basic function to access the page.
 */
Page* get_page(Table* table, pagenum_t pagenum) {
    Page* new_page = NULL;
    int i;
    for (i = 0; i < pool.num_buf; i++){
        if (!new_page && pool.pages[i].table_id == 0){
            new_page = &pool.pages[i];
        }
        if (pool.pages[i].table_id == table->table_id &&
                pool.pages[i].pagenum == pagenum){
						pool.pages[i].pincnt++;
            pool.tot_pincnt++;
            return &pool.pages[i];
        }
    }
    if (!new_page){
        if ((new_page = evict_page()) == NULL){
            PANIC("Not enough buffer\n");
        }
        if (new_page->is_dirty){
            flush_page(new_page);
        }
    }
    file_read_page(table, pagenum, new_page);
    new_page->table_id = table->table_id;
    new_page->pagenum = pagenum;
    new_page->pincnt = 1;
    pool.tot_pincnt++;
    push_to_lru(new_page);
    return new_page;
}

/* Basic function to access the page.
 * Function when executing in transcation.
 * @buf_page_i : index of page in buffer pool.
 */
Page* get_page(Table* table, pagenum_t pagenum, int* buf_page_i) {
    Page* new_page = NULL;
    int i;
		int new_page_i;
    for (i = 0; i < pool.num_buf; i++){
        if (!new_page && pool.pages[i].table_id == 0){
            new_page = &pool.pages[i];
						new_page_i = i;
        }
        if (pool.pages[i].table_id == table->table_id &&
                pool.pages[i].pagenum == pagenum){
					// Only Leaf page latch exists.
					new_page = &pool.pages[i];
					if (((LeafPage*)new_page) -> is_leaf) {
						BUF_PAGE_MUTEX_ENTER(i);
						// Success to acquire latch.
						if (ret) {
							*buf_page_i = i;
							pool.pages[i].pincnt++;
       		    pool.tot_pincnt++;
       	 	   return &pool.pages[i];	
						// Fail to acquire latch.
						} else {
							return nullptr;
						}
					} else {
						pool.pages[i].pincnt++;
						pool.tot_pincnt++;
						return &pool.pages[i];
					}
        }
    }

    if (!new_page){
        if ((new_page = evict_page()) == NULL){
            PANIC("Not enough buffer\n");
        }
        if (new_page->is_dirty){
            flush_page(new_page);
        }
    }

		BUF_PAGE_MUTEX_ENTER(new_page_i);
    
		if (!ret)
			PANIC("Evicted page hold the latch\n");

		file_read_page(table, pagenum, new_page);
    new_page->table_id = table->table_id;
    new_page->pagenum = pagenum;
    new_page->pincnt = 1;
    pool.tot_pincnt++;
		*buf_page_i = new_page_i;
    push_to_lru(new_page);
    return new_page;
}


/* Release header page.
 */
void release_header_page(Table* table) {
    if (table->dbheader.is_dirty){
        file_write_page(table, 0, &table->dbheader);
        table->dbheader.is_dirty = false;
    }
    memset(&table->dbheader, 0, sizeof(HeaderPage));
    pool.tot_pincnt--;
}

/* Basic function to release the page.
 */
void release_page(Page* page) {
    pool.tot_pincnt--;
    page->pincnt--;
}

/**
	* Basic function to release the page " In transaction ".
	*/
void release_page(Page* page, int* buf_page_i) {
	pool.tot_pincnt--;
	page->pincnt--;
	pool.buf_page_mutex[*buf_page_i].unlock();
}

/* Mark dirty flag.
 */
void set_dirty_page(Page* page) {
    page->is_dirty = true;
}

/* Push the page to the free list.
 */
void free_page(Table* table, Page* page) {
    FreePage* freepage = (FreePage*)page;
    memset(freepage, 0, PAGE_SIZE);
    freepage->next = table->dbheader.freelist;
    set_dirty_page(page);
    table->dbheader.freelist = PAGENUM_TO_FILEOFF(freepage->pagenum);
    set_dirty_page((Page*)&table->dbheader);
}

/* Flush all pages of the table in the buffer.
 */
void flush_table(Table* table) {
    int i;
    for (i = 0; i < pool.num_buf; i++){
        if (pool.pages[i].table_id == table->table_id &&
                pool.pages[i].is_dirty){
            flush_page(&pool.pages[i]);
        }
    }
}

/* Init function for buffer pool.
 */
int init_buf_pool(int num_buf) {
    pool.pages = (Page*) calloc(num_buf, sizeof(Page));
		pool.buf_page_mutex = new std::mutex[num_buf];
    
		pool.lru_head = NULL;
    pool.lru_tail = NULL;
    pool.num_buf = num_buf;
    pool.tot_pincnt = 0;
    return 0;
}

/* Destroy buffer pool.
 */
int destroy_buf_pool() {
    free(pool.pages);
		delete[] pool.buf_page_mutex;
    
		pool.pages = NULL;
    pool.lru_head = NULL;
    pool.lru_tail = NULL;
    pool.num_buf = 0;
    pool.tot_pincnt = 0;
    return 0;
}

/* Print the status of buffer pool.
 */
void print_buf_pool() {
    printf("tot pincnt = %d\n", pool.tot_pincnt);
}
