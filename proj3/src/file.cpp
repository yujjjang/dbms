#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "file.h"
#include "panic.h"

int dbfile;

// Puts free page to the free list
void file_free_page(Table* table, pagenum_t pagenum) {
    FreePage freepage;
    memset(&freepage, 0, PAGE_SIZE);

    freepage.next = table->dbheader.freelist;
    file_write_page(table, pagenum, &freepage);
    
    table->dbheader.freelist = PAGENUM_TO_FILEOFF(pagenum);
}

// Expands file pages and prepends them to the free list
void expand_file(Table* table, size_t cnt_page_to_expand) {
    off_t offset = table->dbheader.num_pages * PAGE_SIZE;

    if (table->dbheader.num_pages > 1024 * 1024) {
        // Test code: do not expand over than 4GB
        PANIC("Test: you are already having a DB file over than 4GB");
    }
    
    int i;
    for (i = 0; i < cnt_page_to_expand; i++) {
        file_free_page(table, FILEOFF_TO_PAGENUM(offset));
        table->dbheader.num_pages++;
        offset += PAGE_SIZE;
    }
}

void file_read_page(Table* table, pagenum_t pagenum, void* page) {
    lseek(table->dbfile, PAGENUM_TO_FILEOFF(pagenum), SEEK_SET);
    read(table->dbfile, page, PAGE_SIZE);
}

void file_write_page(Table* table, pagenum_t pagenum, void* page) {
    lseek(table->dbfile, PAGENUM_TO_FILEOFF(pagenum), SEEK_SET);
    write(table->dbfile, page, PAGE_SIZE);
}

