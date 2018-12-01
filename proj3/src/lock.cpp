#include "lock.h"
#include "panic.h"

static LockManager lock_sys;

/**
	*	acquire_lock(trx_t*, int table_id, pagenum_t, int64_t key, LockMode). 
	* 
	* trx_t* 	 : 	current transaction structure.
	* table_id :  current table id.
	* pagenum_t:  current page id.
	* key      :  key on record.
	* LockMode :  Shared or Exclusive
	*
	* @return  : If true, the lock request is granted.
	*            Otherwise, abort transation.
	**/
bool LockManager::acquire_lock(trx_t* trx, int table_id, pagenum_t page_id, int64_t key, LockMode mode, int buf_page_i) {
		
	LOCK_SYS_MUTEX_ENTER;
	// Holding lock_sys_mutex & pool.pages[buf_page_i].mutex.
	Table* table = get_table(table_id);

	if (trx->getState() != RUNNING)
		PANIC("Cannot acquire lock in other mode.\n");
	
	lock_t* prev_or_own_lock_ptr = nullptr;
	bool only_for_cur_trx = true;
	bool lock_req_by_cur_trx = false;
	/**
		* Check whether there is the lock_t request by other trx in list or not.
		* If not, check whether there is the only lock_t request by current trx in list or not.
		*/
	for (auto rit = lock_table[page_id].lock_list.rbegin(); it != lock_table[page_id].lock_list.rend(); ++rit) {
		if (rit -> key == key) {
			if (!prev_or_own_lock_ptr)
				prev_or_own_lock_ptr = &(*rit);
			if (rit -> trx_id != trx->getTransactionId()){
				only_for_cur_trx = false;
				break;
			} else {
				lock_req_by_cur_trx = true;
			}
		}
	}

	// No other transcations' lock requests are in list.
	// And *** Current transaction has already lock request (shared / exclusive) ***.
	if (only_for_cur_trx && lock_req_by_cur_trx) {
		if (mode > prev_or_own_lock_ptr -> lock_mode)
			prev_or_own_lock_ptr -> lock_mode = mode;
		return true;		
	}

	lock_t lock_req{table_id, trx->getTransactionId(), page_id, key, mode};
	
	// TODO : BEFORE PUSH, SHOULD DO DEADLOCK CHECK.
	lock_table[page_id].lock_list.push_back(lock_req);
	lock_t* lock_ptr = &(lock_table[page_id].lock_list.back());
	
	lock_ptr -> prev = prev_or_own_lock_ptr;
	prev_or_own_lock_ptr -> next = lock_ptr;
	/**
		* Shared lock request is granted only the case that there is no exclusive lock request previous current lock req
		*			and other previous shared lock is already granted (acquired).
		*/
	trx_t* prev_trx = nullptr;
	
	if (mode == LOCK_S) {
		bool flag = true;
		lock_t* prev_lock_t = lock_ptr->prev;
		
		while (prev_lock_t) {
			if (prev_lock_t -> lock_mode != LOCK_S || prev_lock_t -> acquired == false) {
				flag = false;
				break;
			}
			prev_lock_t = prev_lock_t->prev;
		}

		// If there is LOCK_X lock req or not granted lock req previous current lock req.
		if (!flag) {
			prev_trx = table->trx_sys->getTransaction(lock_ptr->prev->trx_id);
			prev_trx->getCV()->wait(l_mutex);
			
			// After acquire l_mutex, shared lock should wake up next shared lock req.
			if (lock_ptr -> next && lock_ptr -> next -> lock_mode == LOCK_S)
				trx->getCV()->notify_all();
		}
	
	/**
		* Exclusive lock request is granted only it is first lock request on record.
		* After wake up, it should check whether there is previous lock or not.
	 	*/
	} else {
		while (lock_ptr->prev) {
			if (lock_ptr->prev->trx_id == lock_ptr -> trx_id) {

			}
			prev_trx = table->trx_sys->getTransaction(lock_ptr->prev->trx_id);
			if (!prev_trx)
				PANIC("In exclusive lock list.\n");
			prev_trx->getCV()->wait(l_mutex);
		}
	}
	
	lock_ptr->acquired = true;
	trx -> push_acquired_lock(lock_ptr);

	return true;
}

void LockManager::release_lock_low(trx_t* trx, lock_t* lock_ptr) {
	bool flag = true;	
	
	if (lock_ptr->lock_mode == LOCK_X) {
		for (auto it = lock_table[page_id].lock_list.begin(); it != lock_table[page_id].lock_list.end(); ++it) {
			if (it -> trx_id == trx->getTransactionId() && it -> key == key && 
					it -> acquired && it -> lock_mode == lock_ptr -> lock_mode) {
				flag = false;		
				
				if (lock_ptr -> next)
					lock_ptr->next->prev = nullptr;
				
				trx->getCV()->notify_all();
				lock_table[page_id].lock_list.erase(it);
				break;
			}
		}
	} else {
		bool list_head = true;
		for (auto it = lock_table[page_id].lock_list.begin(); it != lock_table[page_id].lock_list.end(); ++it) {
			if (it -> trx_id == trx->getTransactionId() && it -> key == key &&
					it -> acquired && it -> lock_mode == lock_ptr -> lock_mode) {
				flag = false;	
				
				if (lock_ptr -> next)
					lock_ptr->next->prev = lock_ptr -> prev;
				if (lock_ptr -> prev)
					lock_ptr->prev->next = lock_ptr -> next;
			
				trx->getCV()->notify_all();
				lock_table[page_id].lock_list.erase(it);
				break;
			}
		}
	}
	if (flag)
		PANIC("Cannot find the given lock request in list.\n");
}

/**
	*	release_lock (trx_t*)
	* trx_t* : current transaction pointer.
	* 
	* Release all the locks given transaction held.
	* @return :
	**/
bool LockManager::release_lock(trx_t* trx) {
	LOCK_SYS_MUTEX_ENTER;
	// Get the shared and exclusive locks from given transaction.
	const std::list<lock_t*> acquired_lock = trx->getAcquiredLock();
	// Release all shared locks first.
	for (auto it = acquired_lock.begin(); it != acquired_lock.end(); ++it) {
		release_lock_low(trx, *it);
	}
	trx->pop_all_acquired_lock();

	return true;
}

bool LockManager::deadlock_detection() {





	return true;
}
















