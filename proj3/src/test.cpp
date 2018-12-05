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
#include "bpt.h"
#include "file.h"

using namespace std;
void Result(int re){
	if (re != 1) {
		cout << "GOD NO" << endl;
		exit(0);
	}
}

void shared_lock(int table_id, int key, int trx_id, int* result) {
	int64_t* ret = find(table_id, key, trx_id, result);
}

void exclusive_lock(int table_id, int key, int64_t* input_value, int trx_id, int* result) {
	int ret = update(table_id, key, input_value, trx_id, result);
}

void end_trx(int trx_id){
	end_tx(trx_id);
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
    int64_t input_key;
		int result = 0;
    int64_t input_value[SIZE_COLUMN];
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
		//first_s.join();
		cout << "FS" << endl;
		thread second_x(exclusive_lock, table_id, 1, s_update_value, s_trx_id, &s_result);
		first_s.join();
		cout << "SX" << endl;
		thread first_x(exclusive_lock, table_id, 1, f_update_value, f_trx_id, &f_result);
		cout << "FX" << endl;
		first_x.join();
		cout << "FCSTART " << endl;
		thread first_e(end_trx, f_trx_id);
		cout << "FCDONE" << endl;
		first_e.join();
		second_x.join();
		thread second_e(end_trx, s_trx_id);
		cout << "SC" << endl;
		second_e.join();

		
		close_table(table_id);
		shutdown_db();
		return 0;
}
