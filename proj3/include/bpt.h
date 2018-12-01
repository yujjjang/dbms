#ifndef __BPT_H__
#define __BPT_H__


int init_db(int num_buf); // TODO
int shutdown_db();

int open_table(char* filename, int num_column);
int close_table(int table_id);

// Add trx_id in parameter.
int64_t* find(int table_id, int64_t key);
int insert(int table_id, int64_t key, int64_t* value);
int remove(int table_id, int64_t key);

// API for transaction.
int64_t* find(int table_id, int64_t key, int trx_id, int* result);
int update(int table_id, int64_t key, int64_t* value, int trx_id, int* result);

int begin_tx();
bool end_tx(int trx_id);

void print_tree(int table_id);
void find_and_print(int table_id, int64_t key); 
void license_notice( void );
void usage_1( void );
void usage_2( void );
#endif // __BPT_H__
