#include "lock.h"

LockManager lock_sys;

bool LockManager::acquire_lock(trx_t* trx, int table_id, pagenum_t page_id, int64_t key, LockMode mode, int buf_page_i) {

	LOCK_SYS_MUTEX_ENTER;

	Table* table = get_table(table_id);
	TransactionManager* tm = &trx_sys;


	if (trx->getState() != RUNNING)
		PANIC("Cannot acquire lock in other mode.\n");

	LOCK_REQ_STATE lock_state;
	lock_t* lock_ptr = nullptr;

	int waiting_for_trx_id = -1;
	lock_t* prev_or_own_lock_ptr = nullptr;
	lock_t* wait_lock = nullptr;

	/**
	 *	Exclusive Lock request state.
	 * 
	 * 1. If conflict exists, lock_state = LOCK_WAITING.
	 *
	 * 2. If not, lock_state = LOCK_GRANTED_PUSH or LOCK_GRANTED_NO_PUSH
	 *		 If the trx has exclusive lock request already, No need to push.
	 *    Otherwise, push current lock request & granted.
	 *
	 * Shared Lock request state.
	 *
	 * 1. If conflict exists, lock_state = LOCK_WAITING.
	 *
	 * 2. If not, lock_state = LOCK_GRANTED_PUSH or LOCK_GRANTED_NO_PUSH
	 *		 If the trx has any lock requests already, No need to push.
	 *		 Otherwise, push current lock request & granted.
	 *
	 */
	if (mode == LOCK_X) {

		for (auto rit = lock_table[page_id].lock_list.rbegin(); rit != lock_table[page_id].lock_list.rend(); ++rit) {

			lock_state = LOCK_GRANTED_PUSH;

			if (rit -> key == key && rit -> table_id == table_id) {

				prev_or_own_lock_ptr = &(*rit);

				for (lock_t* lock_t_ptr = prev_or_own_lock_ptr; lock_t_ptr != nullptr;) {

					if (lock_t_ptr -> trx_id != trx->getTransactionId()) {
						lock_state = LOCK_WAITING;
						waiting_for_trx_id = lock_t_ptr -> trx_id;
						wait_lock = lock_t_ptr;
						break;

					} else if (lock_t_ptr -> lock_mode == LOCK_X){
						lock_state = LOCK_GRANTED_NO_PUSH;
					}
				}
				break;
			}
		}

	} else {

		for (auto rit = lock_table[page_id].lock_list.rbegin(); rit != lock_table[page_id].lock_list.rend(); ++rit) {

			lock_state = LOCK_GRANTED_PUSH;

			if (rit -> key == key && rit -> table_id == table_id) {

				prev_or_own_lock_ptr = &(*rit);

				for (lock_t* lock_t_ptr = prev_or_own_lock_ptr; lock_t_ptr != nullptr;) {

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
				}
				break;
			}
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

	if (dl_checker.deadlock_checking(trx->getTransactionId(), waiting_for_trx_id)) {
		release_lock_aborted(trx);
		trx->setState(ABORTED);
		return false;
	}

	MAKE_LOCK_REQUEST_WAIT;

	trx_t* prev_trx = nullptr;

	if (mode == LOCK_S) {

		prev_trx = tm -> getTransaction(waiting_for_trx_id);
		prev_trx -> getCV() -> wait(l_mutex);

	} else {
		prev_trx = tm -> getTransaction(waiting_for_trx_id);
		prev_trx -> getCV() -> wait(l_mutex);

		// After wake up, *** It must check previous lock request one more time. ***
		while (true) {
			bool still_conflict = false;
			// If there is no previous lock req, the lock request is granted.
			if (!lock_ptr -> prev)
				break;

			for (auto lock_t_ptr = lock_ptr -> prev; lock_t_ptr != nullptr;) {
				// If there is conflict, the transaction should wait for it.
				if (lock_t_ptr -> trx_id != trx -> getTransactionId()) {

					still_conflict = true;
					waiting_for_trx_id = lock_t_ptr -> trx_id;
					lock_ptr -> wait_lock = lock_t_ptr;

					if (dl_checker.change_waiting_for(trx->getTransactionId(), waiting_for_trx_id)) {
						// ABORT;
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

/**
	* This function is called in acquried_lock().
	*/
bool LockManager::release_lock_aborted(trx_t* trx) {
	const std::list<lock_t*> acquired_lock = trx->getAcquiredLock();
	for (auto it = acquired_lock.begin(); it != acquired_lock.end(); ++it) {
		release_lock_low(trx, *it);
	}
	trx->getCV()->notify_all();
	trx->pop_all_acquired_lock();
	return true;
}

/**
	* This function is called in end_tx().
	*/
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
