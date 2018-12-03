#include "lock.h"

LockManager lock_sys;

/**
	*	acquire_lock(trx_t*, table_id, page_id, key, lock mode, buf_page_i)
	*	trx_t* 				: current transaction
	* table_id 			: table number
	* page_id 			: page number of record
	* key						: key on record
	* mode 					: LOCK_S or LOCK_X (find : LOCK_S , update : LOCK_X)
	* buf_page_i		: buffer page index for buffer page latch
	*	@return (bool) : granted or aborted.
	*/
bool LockManager::acquire_lock(trx_t* trx, int table_id, pagenum_t page_id, int64_t key, LockMode mode, int buf_page_i) {

	LOCK_SYS_MUTEX_ENTER;

	Table* table = get_table(table_id);
	TransactionManager* tm = &trx_sys;


	if (trx->getState() != RUNNING)
		PANIC("Cannot acquire lock in other mode.\n");

	int waiting_for_trx_id = -1;
	lock_t* prev_lock_ptr = nullptr;
	lock_t* wait_lock = nullptr;

	LOCK_REQ_STATE lock_state = LOCK_GRANTED_PUSH;
	lock_t* lock_ptr = nullptr;
	/**
	 *	Exclusive Lock request state.
	 * 
	 * 1. If conflict exists, lock_state = LOCK_WAITING.
	 *
	 * 2. If not, lock_state = LOCK_GRANTED_PUSH or LOCK_GRANTED_NO_PUSH
	 *		If the trx has exclusive lock request already, No need to push.
	 *    Otherwise, push current lock request & granted.
	 *
	 * Shared Lock request state.
	 *
	 * 1. If conflict exists, lock_state = LOCK_WAITING.
	 *
	 * 2. If not, lock_state = LOCK_GRANTED_PUSH or LOCK_GRANTED_NO_PUSH
	 *		If the trx has any lock requests already, No need to push.
	 *		Otherwise, push current lock request & granted.
	 *
	 */
	
	if (mode == LOCK_X) {

		for (auto rit = lock_table[page_id].lock_list.rbegin(); rit != lock_table[page_id].lock_list.rend(); ++rit) {

			lock_state = LOCK_GRANTED_PUSH;

			if (rit -> key == key && rit -> table_id == table_id) {

				prev_lock_ptr = &(*rit);

				for (lock_t* lock_t_ptr = prev_lock_ptr; lock_t_ptr != nullptr;) {
					
					// Conflict case : Other lock request is already in list.
					if (lock_t_ptr -> trx_id != trx->getTransactionId()) {
						lock_state = LOCK_WAITING;
						waiting_for_trx_id = lock_t_ptr -> trx_id;
						wait_lock = lock_t_ptr;
						break;

					} else if (lock_t_ptr -> lock_mode == LOCK_X){
						lock_state = LOCK_GRANTED_NO_PUSH;
					}
					lock_t_ptr = lock_t_ptr -> prev;
				}
			}
			break;
		}

	} else {

		for (auto rit = lock_table[page_id].lock_list.rbegin(); rit != lock_table[page_id].lock_list.rend(); ++rit) {

			lock_state = LOCK_GRANTED_PUSH;

			if (rit -> key == key && rit -> table_id == table_id) {

				prev_lock_ptr = &(*rit);

				for (lock_t* lock_t_ptr = prev_lock_ptr; lock_t_ptr != nullptr;) {

					// Conflict case : Other exclusive lock request is already in list.
					if (lock_t_ptr -> trx_id != trx->getTransactionId()) {
						if (lock_t_ptr -> lock_mode == LOCK_X) {
							lock_state = LOCK_WAITING;
							waiting_for_trx_id = lock_t_ptr -> trx_id;
							wait_lock = lock_t_ptr;
							break;
						}

					} else {
						lock_state = LOCK_GRANTED_NO_PUSH;
					}
					lock_t_ptr = lock_t_ptr -> prev;
				}
			}
			break;
		}
	}

	
	if (lock_state == LOCK_GRANTED_NO_PUSH) {
		return true;
	}

	if (lock_state == LOCK_GRANTED_PUSH) {
		MAKE_LOCK_REQUEST_GRANTED;
		lock_ptr -> acquired = true;
		trx -> push_acquired_lock(lock_ptr);
		return true;
	}
	
	/**
		*	Before push lock request in lock list, Check whether it causes deadlock or not.
		* If deadlock is detected release current transaction's all lock request and change state RUNNING to ABORTED.
		* Should unlock page latch.
	 	*/
	if (dl_checker.deadlock_checking(trx->getTransactionId(), waiting_for_trx_id)) {
		release_page_latch(buf_page_i);
		release_lock_aborted(trx);
		trx->setState(ABORTED);
		return false;
	}

	MAKE_LOCK_REQUEST_WAIT;

	trx_t* prev_trx = nullptr;
	/**
		* Waiting for other conflict lock request.
		* 
		* Shared Lock request.
		*	Sleep in conflict transaction's cond_var.
		* If conflict one is commit or aboted current lock request is granted.
		*
		* Exclusive Lock request.
		* Sleep in conflict transaction's cond_var.
		* If conflict one is commit or aborted current lock request should check previous lock request one more time.
		* Then change the conflict transaction and wait for that transaction. Otherwise, the current lock request is granted.
		*	
		*/

	release_page_latch(buf_page_i);
	
	if (mode == LOCK_S) {
		prev_trx = tm -> getTransaction(waiting_for_trx_id);
		prev_trx -> getCV() -> wait(l_mutex);

	} else {

		prev_trx = tm -> getTransaction(waiting_for_trx_id);
		prev_trx -> getCV() -> wait(l_mutex);

		while (true) {
			bool still_conflict = false;
			if (!lock_ptr -> prev)
				break;

			for (auto lock_t_ptr = lock_ptr -> prev; lock_t_ptr != nullptr;) {
			
				if (lock_t_ptr -> trx_id != trx -> getTransactionId()) {
					still_conflict = true;
					waiting_for_trx_id = lock_t_ptr -> trx_id;
					lock_ptr -> wait_lock = lock_t_ptr;
					
					if (dl_checker.change_waiting_for(trx->getTransactionId(), waiting_for_trx_id)) {
						release_lock_aborted(trx);					
						trx->setState(ABORTED);
						return false;
					}

					prev_trx = tm -> getTransaction(waiting_for_trx_id);
					prev_trx -> getCV() -> wait(l_mutex);
					break;
				
				} else {
					lock_t_ptr = lock_t_ptr -> prev;
				}
			}
			// If there is no conflict, the lock request is granted.
			if (!still_conflict)
				break;
		}
	}

	acquire_page_latch(buf_page_i);
	lock_ptr -> acquired = true;
	trx -> push_acquired_lock(lock_ptr);

	return true;
}

/**
	* release_lock_low(trx_t*, lock_t*)
	*	trx_t*				: current transaction
	* lock_t*				: lock request by given transaction
	* @return (void).
	*/
void LockManager::release_lock_low(trx_t* trx, lock_t* lock_ptr) {
	int page_id = lock_ptr -> page_id;
	/**
		* If wait lock pointer is same as current lock_ptr, make it nullptr.
		*/
	for (auto it = lock_ptr -> next; it != nullptr;) {
		if (it -> wait_lock == lock_ptr)
			it -> wait_lock = nullptr;
		it = it -> next;
	}

	if (lock_ptr -> prev)
		lock_ptr -> prev -> next = lock_ptr -> next;
	if (lock_ptr -> next)
		lock_ptr -> next -> prev = lock_ptr -> prev;

	bool flag = true;
	
	/** Erase current lock_t in lock list.
		*/
	for (auto it = lock_table[page_id].lock_list.begin(); it != lock_table[page_id].lock_list.end(); ++it) {
		if (it -> trx_id == trx->getTransactionId() && it -> key == lock_ptr -> key && it -> acquired &&
				it -> lock_mode == lock_ptr -> lock_mode && it -> table_id == lock_ptr -> table_id){
			lock_table[page_id].lock_list.erase(it);
			flag = false;
			break;
		}
	}

	if (flag)
		PANIC("In release_lock_low. Cannot erase lock_t in list).\n");
	return;
}

/**
	* This function is called in the deadlock.
	* releases_lock_aborted(trx_t*)
	* trx_t*				: current transaction
	* return (bool) : true.
	*/
bool LockManager::release_lock_aborted(trx_t* trx) {
	
	const std::list<lock_t*> acquired_lock = trx->getAcquiredLock();
	for (auto it = acquired_lock.begin(); it != acquired_lock.end(); ++it) {
		release_lock_low(trx, *it);
	}
	
	trx->getCV()->notify_all();
	return true;
}

/**
	* This function is called in end_tx().
	* release_lock(trx_t*)
	* trx_t*				: current transaction
	* return (bool) : true.
	*/
bool LockManager::release_lock(trx_t* trx) {
	LOCK_SYS_MUTEX_ENTER;
	
	/** Delete the current transaction's Vertex and the Vertex whose edge is current transaction in dl_graph.
		*/
	dl_checker.delete_waiting_for_trx(trx->getTransactionId());
	
	const std::list<lock_t*> acquired_lock = trx->getAcquiredLock();
	for (auto it = acquired_lock.begin(); it != acquired_lock.end(); ++it) {
		release_lock_low(trx, *it);
	}

	trx->getCV()->notify_all();
	return true;
}

/**
	* buf pool latch & buffer page latch.
	*/
inline void LockManager::release_page_latch(int buf_page_i) {
	pool.buf_page_mutex[buf_page_i].unlock();
	pool.tot_pincnt--;
	pool.pages[buf_page_i].pincnt--;
}

inline void LockManager::acquire_page_latch(int buf_page_i) {
	while (1) {
		BUF_POOL_MUTEX_ENTER;
		BUF_PAGE_MUTEX_ENTER(buf_page_i);
		if (ret)
			break;
	}
}
