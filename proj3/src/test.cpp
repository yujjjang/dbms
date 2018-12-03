#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>

#include <iostream>
#include <vector>
#include "bpt.h"
#include "file.h"

using std::vector;
using std::cout;
using std::endl;

// MAIN
void work() {
	int trx_id = begin_tx();
	int table_id = open_table("test1_10.db", 15);

}

int main( int argc, char ** argv ) {
    int64_t input_key;
		int result = 0;
    int64_t input_value[SIZE_COLUMN];
		for (int i = 0; i < SIZE_COLUMN; ++i)
			input_value[i] = i;
		
    int table_id;
    init_db(1000);
		int trx_id_1 = begin_tx();
		int trx_id_2 = begin_tx();
		
    table_id = open_table("test1_10.db", 15);

		vector<int64_t> key_list;
		for (int64_t i = 0; i < 10000; ++i)
			key_list.push_back(i);

		for (auto i = 0; i < 10000; ++i) {
			int64_t* ret = find(table_id, key_list[i], trx_id, &result);
			if (result != 1){
				cout << "GOD NO" << endl;
				break;
			}
		}

		
		for (auto i = 0; i < 10000; ++i) {
			int ret = update(table_id, key_list[i], input_value, trx_id, &result);
	
			if (result != 1)
				cout << "GOD NO " << endl;
				break;
		}
		
		end_tx(trx_id);
		close_table(table_id);
		shutdown_db();
		return 0;
}
