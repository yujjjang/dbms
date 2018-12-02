#include "lock.h"

LockManager lock_sys;

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
	TransactionManager* tm = &trx_sys;

	if (trx->getState() != RUNNING)
		PANIC("Cannot acquire lock in other mode.\n");
	
	lock_t* prev_or_own_lock_ptr = nullptr;
	lock_t* wait_lock = nullptr;
	bool only_for_cur_trx = true;
	bool lock_req_by_cur_trx = false;
	
	int waiting_for_trx_id = -1;
	
	// If true push or skip.
	bool lock_push = true;

	/**
		* Check whether there is the lock_t request by other trx in list or not.
		* If it exists, get the conflicting trx's transaction id for dl_check.
		TODO*/
	for (auto rit = lock_table[page_id].lock_list.rbegin(); rit != lock_table[page_id].lock_list.rend(); ++rit) {
		// Only search the lock request which has same key & table_id in list.
		if (rit -> key == key && rit -> table_id == table_id) {
			prev_or_own_lock_ptr = &(*rit);
			for (lock_t* local_rit = &(*rit); local_rit != nullptr;;) {
				/**
					* LOCK_X : 
					*
					*
					*/
				if (mode == LOCK_X) {
					if (local_rit -> trx_id == trx -> getTransactionId()) {
						
						if (local_rit -> lock_mode == LOCK_X)
							lock_push = false;

					} else {
						waiting_for_trx_id = local_rit -> trx_id;
						wait_lock = (*local_rit);
						only_for_cur_trx = false;
						break;
					}

				} else {



				}
			}
			break;
		}
	}

	for (auto rit = lock_table[page_id].lock_list.rbegin(); rit != lock_table[page_id].lock_list.rend(); ++rit) {
		
		if (rit -> key == key && rit -> table_id == table_id) {
			// Set the prev pointer.
			if (!prev_or_own_lock_ptr)
				prev_or_own_lock_ptr = &(*rit);
			
			if (rit -> trx_id != trx->getTransactionId()){
				only_for_cur_trx = false;
				// LOCK_S wait for LOCK_X by other trx's.
				if (mode == LOCK_S) {	
					if (rit -> lock_mode == LOCK_X) {
						waiting_for_trx_id = rit -> trx_id;
						wait_lock = &(*rit);
						break;
					}
				// LOCK_X wait for any LOCK by other trx's.
				} else {
					waiting_for_trx_id = rit -> trx_id;
					wait_lock = &(*rit);
					break;
				}
			}
		}
	}


	
	// If the transaction should wait for some transaction, DeadLock should be checked.
	if (waiting_for_trx_id != -1) {
		bool ret = dl_checker.deadlock_checking(trx->getTransactionId(), waiting_for_trx_id);
		if (!ret) {
			trx->setState(ABORTED);
			// DEADLOCK OCCUR !
		}
	}

	lock_t lock_req{table_id, trx->getTransactionId(), page_id, key, mode, buf_page_i, wait_lock}; 

	lock_table[page_id].lock_list.push_back(lock_req);
	lock_t* lock_ptr = &(lock_table[page_id].lock_list.back());
	
	if (prev_or_own_lock_ptr) {
		lock_ptr -> prev = prev_or_own_lock_ptr;
		prev_or_own_lock_ptr -> next = lock_ptr;
	}
	
	// If there is no conflict, the lock request is granted immediately.
	if (!wait_lock && waiting_for_trx_id == -1) {
		lock_ptr -> acquired = true;
		trx -> push_acquired_lock(lock_ptr);
		return true;
	}

	/**
		* Shared lock request is granted only the case that there is no exclusive lock request previous current lock req
		*			and other previous shared lock is already granted (acquired).
		*/

	trx_t* prev_trx = nullptr;
	
	if (mode == LOCK_S) {
		prev_trx = tm -> getTransaction(waiting_for_trx_id);
		prev_trx -> getCV() -> wait(l_mutex);
	
	/**
		* Exclusive lock request is granted only it is first lock request on record.
		* After wake up, it should check whether there is previous lock or not.
	 	*/
	} else {
			// Sleep (wait) in the transaction wating for.
			prev_trx = tm -> getTransaction(waiting_for_trx_id);
			prev_trx -> getCV() -> wait(l_mutex);

			// After wake up, *** It must check previous lock request one more time. ***
			while (true) {
				bool still_conflict = false;
				// If there is no previous lock req, the lock request is granted.
				if (!lock_ptr -> prev)
					break;
				
				for (auto it = lock_ptr -> prev; it != nullptr;) {
					// If there is conflict, the transaction should wait for it.
					if (it -> trx_id != trx -> getTransactionId()) {
						still_conflict = true;
						waiting_for_trx_id = it -> trx_id;
						lock_ptr -> wait_lock = it;
						
						bool ret = dl_checker.change_waiting_list(trx->getTransactionId(), waiting_for_trx_id);
						if (!ret) {
							// ABORT;
							trx->setState(ABORTED);

						} else {
							prev_trx = tm -> getTransaction(waiting_for_trx_id);
							prev_trx -> getCV() -> wait(l_mutex);
							break;
						}
					
					} else {
						it = it -> prev;
					}
				}
				// If there is no conflict, the lock request is granted.
				if (!still_conflict)
					break;
			}
	}
	
	lock_ptr -> acquired = true;
	trx -> push_acquired_lock(lock_ptr);

	return true;
}

void LockManager::release_lock_low(trx_t* trx, lock_t* lock_ptr) {
	// First, make wait_lock pointer of waiting for trx nullptr.
	int page_id = lock_ptr -> page_id;
	for (auto it = lock_ptr -> next; it != nullptr;) {
		if (!it)
			break;
		if (it->wait_lock == lock_ptr)
			it->wait_lock = nullptr;
	}
	
	if (lock_ptr -> prev)
		lock_ptr -> prev -> next = lock_ptr -> next;
	if (lock_ptr -> next)
		lock_ptr -> next -> prev = lock_ptr -> prev;
	bool flag = true;
	for (auto it = lock_table[page_id].lock_list.begin(); it != lock_table[page_id].lock_list.end(); ++it) {
		if (it -> trx_id == trx->getTransactionId() && it -> key == lock_ptr -> key && it -> acquired &&
				it -> lock_mode == lock_ptr -> lock_mode && it -> table_id == lock_ptr -> table_id){
			lock_table[page_id].lock_list.erase(it);
			flag = false;
			break;
		}
	}
	if (flag)
		PANIC("Release_lock_low. Cannot erase lock_t in list).\n");
	return;
}

bool LockManager::release_lock(trx_t* trx) {
	LOCK_SYS_MUTEX_ENTER;
	
	dl_checker.delete_waiting_for_trx(trx->getTransactionId());
	// Get the shared and exclusive locks from given transaction.
	const std::list<lock_t*> acquired_lock = trx->getAcquiredLock();
	// Release all shared locks first.
	for (auto it = acquired_lock.begin(); it != acquired_lock.end(); ++it) {
		release_lock_low(trx, *it);
	}
	
	trx->getCV()->notify_all();
	trx->pop_all_acquired_lock();

	return true;
}
