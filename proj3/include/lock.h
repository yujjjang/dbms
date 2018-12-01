#ifndef __LOCK_HPP__
#define __LOCK_HPP__

#include <unordered_map>
#include <list>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <unordered_set>

#include "page.h"
#include "trx.h"
#include "types.h"
#include "panic.h"
#include "deadlock.h"
class trx_t;


typedef uint64_t trx_id_t;

typedef enum {LOCK_S=0, LOCK_X} LockMode;

typedef struct lock_t {
	lock_t(int table_id, trx_id_t trx_id, pagenum_t page_id, int64_t key, LockMode lock_mode, int buf_page_i) :
		table_id(table_id), trx_id(trx_id), page_id(page_id), key(key), lock_mode(lock_mode), buf_page_i(buf_page_i), prev(nullptr), next(nullptr) {};
	table_id table_id;
	trx_id_t trx_id;
	pagenum_t page_id;
	int64_t key;
	bool acquired;
	LockMode lock_mode;
	int buf_page_i;

	lock_t* prev; // Waiting for prev.
	lock_t* next;	// Waiting for cur.
} lock_t;
	
typedef struct lock_page_t {
	std::list<lock_t> lock_list;
} lock_page_t;

class LockManager {
	
	private:
		std::mutex lock_sys_mutex;
		std::condition_variable lock_sys_cv;
		// The hash table keyed on "page number (id) "
		// Each bucket hash page-level locking structure.
		// Lock_t has table_id.
		std::unordered_map<pagenum_t, lock_page_t> lock_table;

	public:
		
		LockManager(){};
		LockManager(int bucket_size) { this->lock_table.reserve(bucket_size); }
		~LockManager(){};
		
		// Called in init_db.
		void setBucketSize(int bucket_size) { this->lock_table.reserve(bucket_size); }

		/**
			* You should implement these functions below.
			*/
		bool acquire_lock(trx_t*, int table_id, pagenum_t, int64_t key, LockMode lock_mode);
		void release_lock_low(trx_t*, pagenum_t, int64_t key, LockMode lock_mode);
		bool release_lock(trx_t*); 
};

#define LOCK_SYS_MUTEX_ENTER do {\
	std::unique_lock<std::mutex> l_mutex(lock_sys_mutex);\
} while(0);

#endif /* LOCK_HPP */
