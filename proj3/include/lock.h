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
struct lock_t {
	lock_t(int table_id, trx_t* trx, pagenum_t page_id, int64_t key, LockMode lock_mode) :
		table_id(table_id), trx(trx), page_id(page_id), key(key), lock_mode(lock_mode), prev(nullptr), next(nullptr) {};
	int table_id;
	trx_t* trx;
	pagenum_t page_id;
	int64_t key;
	bool acquired;
	LockMode lock_mode;

	lock_t* prev;
	lock_t* next;
};	

class LockManager {
	
	struct lock_page_t {
		std::list<lock_t> lock_list;
	};

	private:

		const bool lock_compatibility_matrix[2][2] = {{true, false}, {false, false}};
		const bool lock_strength_matrix[2][2] = {{true, false}, {true, true}};

		DLChecker dl_checker;
		std::mutex lock_sys_mutex;
		
		// The hash table keyed on "page number (id)"
		std::unordered_map<pagenum_t, lock_page_t> lock_table;
	
		bool lock_rec_has_lock(trx_t*, int, pagenum_t, int64_t, LockMode, lock_t**);	
		bool lock_rec_has_conflict(trx_t*, int, pagenum_t, int64_t, LockMode, lock_t*, lock_t**);
		
		void lock_granted_rec_add_to_queue(trx_t*, int, pagenum_t, int64_t, LockMode, lock_t*);
		void lock_wait_rec_add_to_queue(trx_t*, int, pagenum_t, int64_t, LockMode, lock_t*, lock_t*);
		
		lock_t* lock_rec_create(trx_t*, int, pagenum_t, int64_t, LockMode, lock_t*);
		
		std::vector<int> lock_wait_for(trx_t*, lock_t*, lock_t*, LockMode);

		void rollback_data(trx_t*);
		
		void lock_grant(lock_t*);
		bool still_lock_wait(lock_t*);

		const bool lock_mode_compatible(LockMode, LockMode);
		const bool lock_mode_stronger_or_eq(LockMode, LockMode);	

		void release_lock_aborted_low(trx_t*, lock_t*);
		bool release_lock_aborted(trx_t*);
		void release_lock_low(trx_t*, lock_t*);

	public:
		
		LockManager() : dl_checker() {};
		~LockManager(){};
		
		int acquire_lock(trx_t*, int table_id, pagenum_t, int64_t key, LockMode lock_mode);
		bool release_lock(trx_t*); 

};

#define LOCK_SYS_MUTEX_ENTER \
	std::unique_lock<std::mutex> l_mutex(lock_sys_mutex);

#endif /* LOCK_HPP */
