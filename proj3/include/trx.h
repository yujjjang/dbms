// Change pointer to shared_ptr.
#ifndef __TRX_HPP__
#define __TRX_HPP__

#include <list>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "types.h"

#include <unistd.h>
#include <string.h>

class LockManager;
struct lock_t;

struct undo_log {
	undo_log(int table_id, uint64_t page_id, int64_t key, int64_t* old_value) : table_id(table_id), page_id(page_id), key(key) {
		memcpy(undo_value, old_value, 120);
	}
	int table_id;
	uint64_t page_id;
	int64_t key;
	int64_t undo_value[15];
};

class trx_t {

	private:
		int trx_id;
		std::list<lock_t*> acquired_lock;	
		
		std::mutex trx_mutex;
		std::condition_variable_any trx_t_cv;

		std::list<undo_log> undo_log_list;

		State state; // NONE = 0, RUNNING = 1, ABORTED = 2
		LockState lock_state; // SUCCESS = 0, WAIT = 1, DEADLOCK = 2.	
		lock_t* wait_lock;

	public:
		trx_t(trx_id_t t_id) : trx_id(t_id) , state(RUNNING) , wait_lock(nullptr) {
			acquired_lock.clear();
			undo_log_list.clear();
		}

		~trx_t(){ acquired_lock.clear(); undo_log_list.clear(); }

		// For rollback.
		void push_undo_log(int table_id, uint64_t page_id, int64_t key, int64_t* old_value) { 
			undo_log log(table_id, page_id, key, old_value); undo_log_list.push_back(log); }
		undo_log pop_undo_log() { undo_log log = undo_log_list.back(); undo_log_list.pop_back(); return log; }

		// Push or pop the trx's locks.
		void push_acquired_lock(lock_t* lock) { acquired_lock.push_back(lock); }
		void setState(State state) { this -> state = state; }
	
		// For transaction's mutex and condition variable.
		void trx_mutex_enter() { trx_mutex.lock(); }
		void trx_mutex_exit() { trx_mutex.unlock(); }
		void trx_wait_lock() { trx_t_cv.wait(trx_mutex); trx_mutex.unlock(); }
		void trx_wait_release() { trx_t_cv.notify_all();}
		
		// For aborted transaction.
		void undo_update();
		
		void set_wait_lock(lock_t* wait_lock) { this->wait_lock = wait_lock; }
		lock_t* get_wait_lock() { return wait_lock; }

		const std::list<lock_t*> getAcquiredLock () const { return acquired_lock; }
		const State getState() const { return state; }
		const trx_id_t getTransactionId () const { return trx_id; }
		const std::list<undo_log> get_undo_log_list() const { return undo_log_list; }
};


class TransactionManager {

	private:
		std::unordered_map<trx_id_t, trx_t*> active_trx;
		std::atomic<int> trx_id;

	public:
		TransactionManager():trx_id(0) {};	

		~TransactionManager();

		trx_t* makeNewTransaction();
		bool deleteTransaction(trx_id_t);

		std::unordered_map<trx_id_t, trx_t*> getActiveTrx() const { return active_trx; }
		trx_t* getTransaction(trx_id_t trx_id);
};

#endif /* TRX_HPP */
