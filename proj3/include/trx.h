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

class trx_t {
	private:
		// Unique transaction id.
		trx_id_t trx_id;
		// List of locks : For release locks.
		
		std::list<lock_t*> shared_lock;
		std::list<lock_t*> exclusive_lock;
		std::condition_variable<std::mutex> trx_t_cv;
		// HMM..
		std::list<lock_t*> acquired_lock;
		// Current trx's state.
		State state;
	
	public:
		trx_t(trx_id_t t_id) : trx_id(t_id) , state(RUNNING) {};
		~trx_t(){ shared_lock.clear(); exclusive_lock.clear(); }
		
		// Push or pop the trx's locks.
		void push_acquired_lock(lock_t* lock) { acquired_lock.push_back(lock); }
		void pop_all_acquired_lock() { acquired_lock.clear(); }			
		void setState(State state) { this->state = state; }

		const std::list<lock_t*> getAcquiredLock () const { return acquired_lock; }
		const State getState() const { return state; }
		const trx_id_t getTransactionId () const { return trx_id; }
		std::contidion_variable<std::mutex>* getCV() const { return &trx_t_cv; }
};


class TransactionManager {
	
	private:
		std::unordered_map<trx_id_t, trx_t*> active_trx;
		std::atomic<trx_id_t> trx_id;
	
	public:
		TransactionManager():trx_id(0) {};	
		
		~TransactionManager();

		trx_id_t makeNewTransaction();
		bool deleteTransaction(trx_id_t);

		std::unordered_map<trx_id_t, trx_t*> getActiveTrx() const { return active_trx; }
		trx_t* getTransaction(trx_id_t trx_id) const { return active_trx[trx_id]; }
};

#endif /* TRX_HPP */
