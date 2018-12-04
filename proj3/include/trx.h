// Change pointer to shared_ptr.
#ifndef __TRX_HPP__
#define __TRX_HPP__

#include <list>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "types.h"

class LockManager;
struct lock_t;

struct undo_log {
	undo_log(int64_t key, int64_t* old_value) : key(key) {
		memcpy(undo_value, old_value, 120);
	}
	int64_t key;
	int64_t undo_value[15];
};

class trx_t {

	private:
		int trx_id;
		std::list<lock_t*> acquired_lock;	
		std::condition_variable trx_t_cv;

		std::list<undo_log> undo_log_list;

		State state; // NONE = 0, RUNNING = 1, ABORTED = 2
	
	public:
		trx_t(trx_id_t t_id) : trx_id(t_id) , state(RUNNING) {};
		~trx_t(){ acquired_lock.clear(); undo_log_list.clear(); }

		// For rollback.
		void push_undo_log(int64_t key, int64_t* old_value) { undo_log log(key, old_value); undo_log_list.push_back(log); }
		undo_log pop_undo_log() { undo_log log = undo_log_list.front(); undo_log_list.pop_front(); return log; }

		// Push or pop the trx's locks.
		void push_acquired_lock(lock_t* lock) { acquired_lock.push_back(lock); }
		void setState(State state) { this->state = state; }

		const std::list<lock_t*> getAcquiredLock () const { return acquired_lock; }
		const State getState() const { return state; }
		const trx_id_t getTransactionId () const { return trx_id; }
		std::condition_variable* getCV() { return &trx_t_cv; }
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
