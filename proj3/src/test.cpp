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
	if (*result == 0)
		cout << "Aborted Now" << endl;
	cout << "-----------" << endl;
}

void exclusive_lock(int table_id, int key, int64_t* input_value, int trx_id, int* result) {
	int ret = update(table_id, key, input_value, trx_id, result);
	cout << "EXCLUSIVE TRX ID : " << trx_id << "write" << endl;
	if (*result == 0)
		cout << "Aborted Now" << endl;
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
		f_update_value[i] = 5000+i;
		s_update_value[i] = 100+i;
	}

	int table_id;
	init_db(1000);
	int f_result = 0;
	int s_result = 0;
	int t_result = 0;
	int fo_result = 0;

	table_id = open_table("test1_10.db", 15);
	auto test1 = [&]() {
		cout << ">> First Schedule : Record 1 : R(t1) W(t2) W(t1)" << endl;
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

		cout << ">> Second Schedule : Record 1 : W(t1) W(t2) R(t1) R(t2)" << endl;
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
		cout << ">> Third Schedule : Record 1 : W(t1)       R(t2)" << endl;
		cout << ">>                  Record 2 :       W(t2)       W(t1)"    << endl;
		int f_trx_id = begin_tx();
		int s_trx_id = begin_tx();
		thread first_x(exclusive_lock, table_id, 1, f_update_value, f_trx_id, &f_result);
		first_x.join();
		thread second_x(exclusive_lock, table_id, 2, s_update_value, s_trx_id, &s_result);
		second_x.join();	
		thread second_s_s2(shared_lock, table_id, 1,s_trx_id, &s_result);
		usleep(5000);
		thread first_x_2(exclusive_lock, table_id, 2, f_update_value, f_trx_id, &f_result);
		first_x_2.join();
		thread first_e(end_trx, f_trx_id);
		first_e.join();
		second_s_s2.join();
		thread second_s(shared_lock, table_id, 1, s_trx_id, &s_result);
		second_s.join();
		thread second_s_2(shared_lock, table_id, 2, s_trx_id, &s_result);
		second_s_2.join();
		thread second_e(end_trx, s_trx_id);
		second_e.join();
		cout << "<< Third schedule done" << endl;
	};

	auto test4 = [&]() {
		cout << ">> Foruth Schedule : Record 1 : S(t1) S(t2) S(t3) W(t4)" << endl;
		int fi_trx_id = begin_tx();
		int se_trx_id = begin_tx();
		int th_trx_id = begin_tx();
		int fo_trx_id = begin_tx();
		thread fi_s(shared_lock, table_id, 1, fi_trx_id, &f_result);
		
		fi_s.join();
		thread se_s(shared_lock, table_id, 1, se_trx_id, &s_result);
		
		se_s.join();
		thread th_s(shared_lock, table_id, 1, th_trx_id, &t_result);

		th_s.join();
		thread fo_x(exclusive_lock, table_id, 1, s_update_value, fo_trx_id, &fo_result);

		thread f_c(end_trx, fi_trx_id);
		
		f_c.join();
		thread s_c(end_trx, se_trx_id);
		
		s_c.join();
		thread t_c(end_trx, th_trx_id);

		t_c.join();
		fo_x.join();

		thread fo_c(end_trx, fo_trx_id);
		fo_c.join();
		cout << "<< Fourth schedule done" << endl;
	};	


	auto test5 = [&]() {
		cout << ">> Fifth Schedule : Record 1 : W(t1) S(t2) S(t3) S(t4)" << endl;
		int fi_trx_id = begin_tx();
		int se_trx_id = begin_tx();
		int th_trx_id = begin_tx();
		int fo_trx_id = begin_tx();
		thread fi_s(exclusive_lock, table_id, 1, f_update_value, fi_trx_id, &f_result);
		
		fi_s.join();
		
		thread se_s(shared_lock, table_id, 1, se_trx_id, &s_result);
		thread th_s(shared_lock, table_id, 1, th_trx_id, &t_result);
		thread fo_x(shared_lock, table_id, 1, fo_trx_id, &fo_result);

		thread f_c(end_trx, fi_trx_id);
		f_c.join();

		se_s.join();
		th_s.join();
		fo_x.join();
		thread s_c(end_trx, se_trx_id);
		s_c.join();
		thread t_c(end_trx, th_trx_id);
		t_c.join();
		thread fo_c(end_trx, fo_trx_id);
		fo_c.join();
		cout << "<< Fifth schedule done" << endl;
	};	




	auto dl_test1 = [&]() {
		cout << ">> First DeadLock checker" << endl;
		int fi = begin_tx();
		int se = begin_tx();


	}
	auto dl_test2 = [&]() {
	}
	auto dl_test3 = [&]() {
	}
	auto dl_test4 = [&]() {
	}
	auto dl_test5 = [&]() {
	}






	test1();
	cout << endl;
	test2();
	cout << endl;
	test3();
	cout << endl;
	test4();
	cout << endl;
	test5();
	cout << endl;
	close_table(table_id);
	shutdown_db();
	return 0;
}
