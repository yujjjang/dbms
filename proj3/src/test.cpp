#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include <thread>
#include <stdlib.h>

#include <iostream>
#include <vector>
#include <iostream>
#include "bpt.h"
#include "file.h"

using namespace std;

void shared_lock(int table_id, int key, int trx_id, int* result) {
	int64_t* ret = find(table_id, key, trx_id, result);
	cout << "@@@@TRX ID : " << trx_id << " READ" << endl;
	for (int i = 0; i < 15; ++i)
		cout << ret[i] << ' ';
	cout << endl;
	cout << "Result : " << *result << endl;
}

void exclusive_lock(int table_id, int key, int64_t* input_value, int trx_id, int* result) {
	int ret = update(table_id, key, input_value, trx_id, result);
	cout << "@@@@TRX ID : " << trx_id << "write" << endl;
	cout << "Return : " << ret << endl;
	cout << "Result : " << *result << endl;
}

void end_trx(int trx_id){
	int i = end_tx(trx_id);
	cout << "@@@@TRX ID : " << trx_id;
	if (i == -1)
		cout << " ABORTED";
	else
		cout << " COMMITTED";
	cout << endl;
}

void first_schedule_RRW(int table_id){
}
void second_schedule_RWR(int table_id){
}
void third_schedule_WRR(int table_id){
}
void fourth_schdule_WRR(int table_id){
}


int main( int argc, char ** argv ) {
	ios_base::sync_with_stdio(false);
		int64_t input_key;
		int result = 0;
		int64_t f_update_value[SIZE_COLUMN];
		int64_t s_update_value[SIZE_COLUMN];
		for (int i = 0; i < SIZE_COLUMN; ++i){
			f_update_value[i] = i;
			s_update_value[i] = 100+i;
		}
		
    int table_id;
    init_db(1000);
		int f_trx_id = begin_tx();
		int s_trx_id = begin_tx();
		int f_result = 0;
		int s_result = 0;
		
    table_id = open_table("test1_10.db", 15);

		thread first_s(shared_lock, table_id, 1, f_trx_id, &f_result);
		first_s.join();
		thread second_x(exclusive_lock, table_id, 1, s_update_value, s_trx_id, &s_result);

		usleep(3000);
		thread first_x(exclusive_lock, table_id, 1, f_update_value, f_trx_id, &f_result);
		first_x.join();
		thread first_e(end_trx, f_trx_id);
		first_e.join();
		second_x.join();
		thread second_e(end_trx, s_trx_id);
		second_e.join();

		
		close_table(table_id);
		shutdown_db();
		return 0;
}
