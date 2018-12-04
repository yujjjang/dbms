#ifndef __LOCK_HPP__
#define __LOCK_HPP__

#include <unordered_map>
#include <list>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <unordered_set>

#include "page.h"
#include "table.h"
#include "trx.h"
#include "types.h"
#include "panic.h"
#include "deadlock.h"
#include "buf.h"

extern TransactionManager trx_sys;
extern BufPool pool;

/**
	* Lock request structure.
	*/
typedef struct lock_t {
	lock_t(int table_id, trx_id_t trx_id, pagenum_t page_id, int64_t key, LockMode lock_mode, int buf_page_i, lock_t* wait_lock) :
		table_id(table_id), trx_id(trx_id), page_id(page_id), key(key), lock_mode(lock_mode), buf_page_i(buf_page_i), 
		prev(nullptr), next(nullptr) ,wait_lock(wait_lock) {};
	int table_id;
	trx_id_t trx_id;
	pagenum_t page_id;
	int64_t key;
	bool acquired;
	LockMode lock_mode;
	int buf_page_i;

	lock_t* prev;
	lock_t* next;

	lock_t* wait_lock;

} lock_t;
	
typedef struct lock_page_t {
	std::list<lock_t> lock_list;
} lock_page_t;

class LockManager {
	typedef enum {LOCK_WAITING, LOCK_GRANTED_PUSH, LOCK_GRANTED_NO_PUSH} LOCK_REQ_STATE;
	
	private:
		DLChecker dl_checker;
		std::mutex lock_sys_mutex;
		
		// The hash table keyed on "page number (id)"
		std::unordered_map<pagenum_t, lock_page_t> lock_table;
		
		void rollback_data(int, uint64_t, trx_t*, undo_log&);
		void release_lock_aborted_low(trx_t*, lock_t*);
		bool release_lock_aborted(trx_t*);

		inline void release_page_latch(int buf_page_i);
		inline void acquire_page_latch(int buf_page_i);


	public:
		
		LockManager() : dl_checker() {};
		~LockManager(){};
		
		bool acquire_lock(trx_t*, int table_id, pagenum_t, int64_t key, LockMode lock_mode, int buf_page_i);
		void release_lock_low(trx_t*, lock_t*);
		bool release_lock(trx_t*); 
};

#define LOCK_SYS_MUTEX_ENTER \
	std::unique_lock<std::mutex> l_mutex(lock_sys_mutex, std::defer_lock);\
	bool lock_sys_mutex_acquired = l_mutex.try_lock();

#define MAKE_LOCK_REQUEST_WAIT \
	lock_t lock_req{table_id, trx->getTransactionId(), page_id, key, mode, buf_page_i, wait_lock};\
	lock_table[page_id].lock_list.push_back(lock_req);\
	lock_ptr = &(lock_table[page_id].lock_list.back());\
	if(prev_lock_ptr) {\
		lock_ptr -> prev = prev_lock_ptr;\
		prev_lock_ptr -> next = lock_ptr;\
	}

#define MAKE_LOCK_REQUEST_GRANTED \
	lock_t lock_req{table_id, trx->getTransactionId(), page_id, key, mode, buf_page_i, nullptr};\
	lock_table[page_id].lock_list.push_back(lock_req);\
	lock_ptr = &(lock_table[page_id].lock_list.back());\
	if(prev_lock_ptr) {\
		lock_ptr -> prev = prev_lock_ptr;\
		prev_lock_ptr -> next = lock_ptr;\
	}

#endif /* LOCK_HPP */
