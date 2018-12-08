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

int64_t f_update_value[SIZE_COLUMN];
int64_t s_update_value[SIZE_COLUMN];

void shared_lock(int table_id, int key, int trx_id, int* result) {
	int64_t* ret = find(table_id, key, trx_id, result);
	cout << "SHARED TRX ID : " << trx_id << " READ" << endl;
	for (int i = 0; i < 15; ++i)
		cout << ret[i] << ' ';
	cout << endl;
	cout << "Result : " << *result << endl;
	cout << "----------------" << endl;
}

void exclusive_lock(int table_id, int key, int64_t* input_value, int trx_id, int* result) {
	int ret = update(table_id, key, input_value, trx_id, result);
	cout << "EXCLUSIVE TRX ID : " << trx_id << "write" << endl;
	cout << "Result : " << *result << endl;
	cout << "----------------" << endl;
}

void end_trx(int trx_id){
	int i = end_tx(trx_id);
	cout << "END TRX ID : " << trx_id;
	if (i == -1)
		cout << " ABORTED";
	else
		cout << " COMMITTED";
	cout << "----------------" << endl;
}

int main( int argc, char ** argv ) {
	ios_base::sync_with_stdio(false);
	int64_t input_key;
	int result = 0;
	for (int i = 0; i < SIZE_COLUMN; ++i){
		f_update_value[i] = i;
		s_update_value[i] = 100+i;
	}

	int table_id;
	init_db(1000);
	int f_result = 0;
	int s_result = 0;

	table_id = open_table("test1_10.db", 15);
	auto test1 = [&]() {
		cout << ">> First Schedule : R(t1) W(t2) W(t1)" << endl;
		int f_trx_id = begin_tx();
		int s_trx_id = begin_tx();

		thread first_s(shared_lock, table_id, 1, f_trx_id, &f_result);
		first_s.join();
		thread second_x(exclusive_lock, table_id, 1, s_update_value, s_trx_id, &s_result);		
		thread first_x(exclusive_lock, table_id, 1, f_update_value, f_trx_id, &f_result);
		first_x.join();

		second_x.join();
		thread first_e(end_trx, f_trx_id);
		first_e.join();
		thread second_e(end_trx, s_trx_id);
		second_e.join();

		cout << "<< First Schedule done" << endl;
	};

	auto test2 = [&]() {

		cout << ">> Second Schedule : W(t1) W(t2) R(t1) R(t2)" << endl;
		int f_trx_id = begin_tx();
		int s_trx_id = begin_tx();
		thread first_x(exclusive_lock, table_id, 1, f_update_value, f_trx_id, &f_result);
		first_x.join();
		thread second_x(exclusive_lock, table_id, 1, s_update_value, s_trx_id, &s_result);
		thread first_s(shared_lock, table_id, 1, f_trx_id, &f_result);
		first_s.join();
		thread first_e(end_trx, f_trx_id);
		first_e.join();
		second_x.join();
		thread second_s(shared_lock, table_id, 1, s_trx_id, &s_result);
		second_s.join();
		thread second_e(end_trx, s_trx_id);
		second_e.join();
		cout << "<< Second schedule done" << endl;
	};

	auto test3 = [&](){
	

	};

	test1();
	cout << endl;
	test2();
	cout << endl;
	test3();
	cout << endl;

	close_table(table_id);
	shutdown_db();
	return 0;
}
