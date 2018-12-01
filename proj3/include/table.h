#ifndef __TABLE_H__
#define __TABLE_H__

#include "page.h"

#include "trx.h"
#include "lock.h"

typedef struct _Table {
  HeaderPage dbheader;
  int dbfile;
  int table_id;		

	LockManager* lock_sys;         // Each table has LM & TM.
	TransactionManager* trx_sys;		// TODO
} Table;

#define MAX_NUM_TABLE 10

// DB initialization.
int open_or_create_table_file(const char* filename);

int close_table_file(Table *table);
// Should add function for LM & TM.

Table *get_table(int table_id);

#endif /* __TABLE_H__ */
