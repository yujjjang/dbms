#include "bpt_internal.h"
#include "lock.h"
#include "trx.h"

static Table tables[MAX_NUM_TABLE];

/**
	*  alloc_table()
	*  find the index to create new table.
	*	 Make the LockManager and Transaction Manager in it.
	**/
int alloc_table() {
    for (int i = 0; i < MAX_NUM_TABLE; i++){
        if (tables[i].table_id == 0){
            tables[i].table_id = i+1;
            return i+1;
        }
    }
    PANIC("Alloc table limit.");
    return -1;
}

Table* get_table(int table_id) {
    if (table_id <= 0){
        PANIC("get_table");
    }
    return &tables[table_id-1];
}

// Opens a db file or creates a new file if not exist.
int open_or_create_table_file(const char* filename, int num_column) {
    int table_id = alloc_table();
    Table* table;

    if (table_id <= 0){
        return -1;
    }
    table = get_table(table_id);
    table->dbfile = open(filename, O_RDWR);
    if (table->dbfile < 0) {
        // Creates a new db file
        table->dbfile = open(filename, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
        if (table->dbfile < 0) {
            PANIC("failed to create new db file");
        }
        init_new_header_page(&table->dbheader, table_id, num_column);
    } else {
        // DB file exists. Loads header info
        load_header_page(table);
    }
    return table_id;
}

// Closes the db file.
int close_table_file(Table* table) {
    release_header_page(table);
    close(table->dbfile);
    table->dbfile = 0;
    table->table_id = 0;
		return 0;
}
