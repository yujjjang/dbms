#ifndef __FILE_H__
#define __FILE_H__
#include "table.h"
#include "page.h"

// Expand the file to manage more pages
void expand_file(Table *table, size_t cnt_page_to_expand);

// Load file page into the in-memory page
void file_read_page(Table *table, pagenum_t pagenum, void* page);

// Flush page into the file
void file_write_page(Table *table, pagenum_t pagenum, void* page);

#endif /* __FILE_H__ */
