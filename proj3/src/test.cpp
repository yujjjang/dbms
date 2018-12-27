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

void shared_lock(int table_id, int key, int trx_id, int64_t* ret, int* result) {
	ret = find(table_id, key, trx_id, result);
}

void exclusive_lock(int table_id, int key, int64_t* input_value, int trx_id, int* result) {
	int ret = update(table_id, key, input_value, trx_id, result);
}

void end_trx(int trx_id, int* i){
	*i = end_tx(trx_id);
}

int main( int argc, char ** argv ) {
	ios_base::sync_with_stdio(false);
	int64_t input_key;
	int result = 0;
	for (int i = 0; i < SIZE_COLUMN; ++i){
		f_update_value[i] = 5000+i;
		s_update_value[i] = 100+i;
	}

	int table_id;
	init_db(1000);
	int f_result = 0;
	int s_result = 0;
	int t_result = 0;

	int64_t f_ret[120];
	int64_t s_ret[120];
	int64_t t_ret[120];

	int f_i = 0;
	int s_i = 0;
	int t_i = 0;
	table_id = open_table("test1_10.db", 15);

	auto TEST1 = [&]() {
		int t1 = begin_tx();
		int t2 = begin_tx();

		thread t1_x(exclusive_lock, table_id, 1, f_update_value, t1, &f_result);
		t1_x.join();
		if (f_result == 0){
			cout << "ERR" << endl;
		}

		thread t2_x(exclusive_lock, table_id, 1, s_update_value, t2, &s_result);

		thread t1_c(end_trx, t1, &f_i);
		t1_c.join();
		t2_x.join();
		if (s_result == 0){
			cout << "ERE" << endl;
		}

		thread t2_c(end_trx, t2, &s_i);
		t2_c.join();
	};

	auto TEST2 = [&]() {
		int t1 = begin_tx();
		int t2 = begin_tx();

		thread t1_s(shared_lock, table_id, 1, t1, f_ret,&f_result);
		thread t2_s(shared_lock, table_id, 1, t2, s_ret,&s_result);

		t1_s.join();
		t2_s.join();

		if (!(f_result && s_result)){
			cout << "Err" << endl;
		}

		thread t1_c(end_trx, t1, &f_i);
		thread t2_c(end_trx, t2, &s_i);
		t1_c.join();
		t2_c.join();
	};

	auto TEST3 = [&]() {
		int t1 = begin_tx();
		int t2 = begin_tx();
		int t3 = begin_tx();

		thread t1_s(shared_lock, table_id, 1, t1, f_ret, &f_result);
		thread t2_s(shared_lock, table_id, 1, t2, s_ret, &s_result);
		
		t1_s.join();
		t2_s.join();
		
		if (!(f_result && s_result)){
			cout << "Err" << endl;
		}

		thread t3_x(exclusive_lock, table_id, 1, f_update_value, t3, &t_result);
		
		usleep(1000000);
		
		thread t1_c(end_trx, t1, &f_i);
		t1_c.join();
		cout << " Fin" << endl;
		thread t2_c(end_trx, t2, &f_i);
		t2_c.join();

		t3_x.join();
		if(t_result == 0)
			cout << "ERERE" << endl;
		thread t3_c(end_trx, t3, &s_i);
		t3_c.join();
	};
	
	auto TEST4 = [&]() {
		int t1 = begin_tx();
		int t2 = begin_tx();

		thread t1_x(exclusive_lock, table_id, 1, f_update_value, t1, &f_result);
		thread t2_x(exclusive_lock, table_id, 2, s_update_value, t2, &s_result);

		t1_x.join();
		t2_x.join();

		if (!(f_result && s_result)){
			cout << "Err" << endl;
		}

		thread t1_c(end_trx, t1, &f_i);
		thread t2_c(end_trx, t2, &s_i);
		t1_c.join();
		t2_c.join();
	};

	auto DL_TEST1 = [&]() { // R1 W2 W1.
		int t1 = begin_tx();
		int t2 = begin_tx();

		thread t1_s(shared_lock, table_id, 1, t1, f_ret, &f_result);
		t1_s.join();
	  thread t2_x(exclusive_lock, table_id, 1, s_update_value, t2, &s_result);

		usleep(1000000);

		thread t1_x(exclusive_lock, table_id, 1, f_update_value, t1, &f_result);
		t1_x.join();
		t2_x.join();
		if (f_result == 1 || s_result == 0)
			cout << "Error" << endl;

		thread t1_c(end_trx, t1, &f_i);
		thread t2_c(end_trx, t2, &s_i);

		t1_c.join();
		t2_c.join();

		if (f_i != -1 || s_i != 0)
			cout << "ERR" << endl;
	};
	auto DL_TEST2 = [&]() { 
		/**  R1: W1   R2 
			*  R2:    W2   R1
			*/
		int t1 = begin_tx();
		int t2 = begin_tx();

		thread t1_x(exclusive_lock, table_id, 1, f_update_value, t1, &f_result);
		thread t2_x(exclusive_lock, table_id, 2, s_update_value, t2, &s_result);

		t1_x.join();
		t2_x.join();
		if (f_result != 1 || s_result != 1)
			cout << "Err" << endl;

		thread t2_s(shared_lock, table_id, 1, t2, s_ret, &s_result);

		usleep(1000000);

		thread t1_s(shared_lock, table_id, 2, t1, f_ret, &f_result);
		t1_s.join();
		t2_s.join();

		if (f_result != 0 || s_result != 1)
			cout << "Err" << endl;

		thread t1_c(end_trx, t1, &f_i);
		thread t2_c(end_trx, t2, &s_i);
		t1_c.join();
		t2_c.join();

		if (f_i != -1 || s_i != 0)
			cout << "Err" << endl;
	};
	auto DL_TEST3 = [&]() {
		/**  R1 : S1       W3
			*  R2 : S2  W1
			*  R3 : S3     W2
			*/
		int t1 = begin_tx();
		int t2 = begin_tx();
		int t3 = begin_tx();

		thread t1_s(shared_lock, table_id, 1, t1, f_ret, &f_result);
		thread t2_s(shared_lock, table_id, 2, t2, s_ret, &s_result);
		thread t3_s(shared_lock, table_id, 3, t3, t_ret, &t_result);

		t1_s.join(); t2_s.join(); t3_s.join();
		if (f_result != 1 || s_result != 1 || t_result != 1)
			cout << "Err" << endl;

		thread t1_x(exclusive_lock, table_id, 2, f_update_value, t1, &f_result);
		usleep(1000000);
		thread t2_x(exclusive_lock, table_id, 3, f_update_value, t2, &s_result);
		usleep(1000000);
		thread t3_x(exclusive_lock, table_id, 1, s_update_value, t3, &t_result);

		t3_x.join();
		t2_x.join();

		if (t_result != 0 || s_result != 1)
			cout << "Err" << endl;

		thread t3_c(end_trx, t3, &t_i);
		thread t2_c(end_trx, t2, &s_i);
		t3_c.join();
		t2_c.join();
		if (t_i != -1 || s_i != 0)
			cout << "Err" << endl;
		t1_x.join();

		thread t1_c(end_trx, t1, &f_i);
		t1_c.join();

		if (f_i != 0)
			cout << "Err" << endl;
	};
	
	/*TEST1();
	TEST2();*/
	TEST3();
	/*TEST4();
	DL_TEST1();
	DL_TEST2();
	DL_TEST3();*/

	close_table(table_id);
	shutdown_db();
	return 0;
}
