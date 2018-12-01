#ifndef __BPT_H__
#define __BPT_H__


int init_db(int num_buf); // TODO
int shutdown_db();

int open_table(char* filename);
int close_table(int table_id);

// Add trx_id in parameter.
char* find(int table_id, int64_t key);
int insert(int table_id, int64_t key, const char* value);
int remove(int table_id, int64_t key);
int update(int table_id, int64_t key, const char* value);

// API for transaction.
char* find(int table_id, int64_t key, int trx_id);

int begin_transaction(int table_id);
bool commit_transaction(int table_id, int trx_id);
bool abort_transaction(int table_id, int trx_id);

void print_tree(int table_id);
void find_and_print(int table_id, int64_t key); 
void license_notice( void );
void usage_1( void );
void usage_2( void );
#endif // __BPT_H__
