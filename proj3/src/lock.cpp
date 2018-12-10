#include "lock.h"

LockManager lock_sys;

/**
	* lock_rec_has_lock(trx_t*, table_id, page_id, key, mode, front_lock_ptr)
	*
 	* @return (bool) : If enough lock request is already granted return true. Other wise return false
	*/

bool LockManager::lock_rec_has_lock(trx_t* trx, int table_id, pagenum_t page_id, 
		int64_t key, LockMode mode, lock_t** front_lock_ptr) {
		
	for (auto it = lock_table[page_id].lock_list.begin(); it != lock_table[page_id].lock_list.end(); ++it) {
		
		if (it -> key == key && it -> table_id == table_id) {

			*front_lock_ptr = &(*it);
			
			for (lock_t* lock = *front_lock_ptr; (lock && lock -> acquired);) {
				if (lock -> trx -> getTransactionId() == trx -> getTransactionId() &&  lock_mode_stronger_or_eq(lock -> lock_mode, mode)) {
						return true;
				}
				lock = lock -> next;
			}
		}
	}
	return false;
}

/**
	* lock_rec_create(trx_t*, table_id, page_id, key, mode, tail_lock_ptr)
	*
  * @return (lock_t*) : return current lock request pointer.
	*/
lock_t* LockManager::lock_rec_create(trx_t* trx, int table_id, pagenum_t page_id, 
		int64_t key, LockMode mode, lock_t* tail_lock_ptr) {
	lock_t lock_req(table_id, trx, page_id, key, mode);
	lock_table[page_id].lock_list.push_back(lock_req);
	lock_t* lock_ptr = &(lock_table[page_id].lock_list.back());

	if (tail_lock_ptr) {
		lock_ptr -> prev = tail_lock_ptr;
		tail_lock_ptr -> next = lock_ptr;
	}
	return lock_ptr;
}

/**
	* lock_wait_rec_add_to_queue(trx_t*, table_id, page_id, key, mode, front_lock_ptr, wait_lock)
	*
	* @return (void).
	*
	* Make new waiting lock request and push in list.
	*/
void LockManager::lock_wait_rec_add_to_queue(trx_t* trx, int table_id, pagenum_t page_id, int64_t key, LockMode mode,
		lock_t* front_lock_ptr, lock_t* wait_lock) {

	trx->trx_mutex_enter();
	lock_t* tail_lock_ptr = nullptr;
	
	for (lock_t* lock = front_lock_ptr; lock != nullptr;){
		tail_lock_ptr = lock;
		lock = lock -> next;
	}

	lock_t* lock_ptr = lock_rec_create(trx, table_id, page_id, key, mode, tail_lock_ptr);
	lock_ptr -> acquired = false;
	trx -> set_wait_lock(wait_lock);
}

/**
	* lock_granted_rec_add_to_queue(trx_t*, table_id, page_id, key, mode, front_lock_ptr)
	*
	*@return (void).
	*
	* Make new granted lock request and push in list. 
	*/
void LockManager::lock_granted_rec_add_to_queue(trx_t* trx, int table_id, pagenum_t page_id, int64_t key, 
		LockMode mode, lock_t* front_lock_ptr) {

	lock_t* tail_lock_ptr = nullptr;
	for (lock_t* lock = front_lock_ptr; lock != nullptr;) {
		tail_lock_ptr = lock;
		lock = lock -> next;
	}

	lock_t* lock_ptr = lock_rec_create(trx, table_id, page_id, key, mode, tail_lock_ptr);
	lock_ptr -> acquired = true;
	trx -> push_acquired_lock(lock_ptr);
}

/**
	* lock_rec_has_conflict(trx_t*, table_id, page_id, key, mode, front_lock_ptr, wait_lock)
	* 
	*@return (bool) : If there is conflict, set wait lock and return true. Otherwise return false.
	*/
bool LockManager::lock_rec_has_conflict(trx_t* trx, int table_id, pagenum_t page_id, int64_t key, LockMode mode,
		lock_t* front_lock_ptr, lock_t** wait_lock) {
	
	if (!front_lock_ptr)
		return false;
	
	for (lock_t* lock = front_lock_ptr; lock != nullptr;) {
		if (lock -> trx -> getTransactionId() != trx -> getTransactionId() &&
				!lock_mode_compatible(lock -> lock_mode, mode)) {
			*wait_lock = lock;
		}
		lock = lock -> next;
	}

	if (*wait_lock){
		return true;
	}
	return false;
}

/**
	* lock_wait_for(trx_t*, lock_t*, lock_t*,  mode)
	*
	*@return (vector<int>)  : All conflict transactions' id.
	*/
std::vector<int> LockManager::lock_wait_for(trx_t* trx, lock_t* front_lock_ptr, lock_t* wait_lock, LockMode mode) {
	std::vector<int> wait_for;
	
	for (lock_t* lock = front_lock_ptr; lock != wait_lock;) {
		if (lock -> trx -> getTransactionId() != trx -> getTransactionId() &&
				!lock_mode_compatible(lock -> lock_mode, mode))
			wait_for.push_back(lock -> trx -> getTransactionId());
		lock = lock -> next;
	}
	
	wait_for.push_back(wait_lock -> trx -> getTransactionId());
	return wait_for;
}

/**
 *	acquire_lock(trx_t*, table_id, page_id, key, lock mode)
 *	trx_t* 				: current transaction
 *  table_id 			: table number
 *  page_id 			: page number of record
 *  key						: key on record
 *  mode 					: LOCK_S or LOCK_X (find : LOCK_S , update : LOCK_X)
 *	@return (int) : LOCK_SUCCESS. LOCK_WAITING, DEADLOCK.
 */
int LockManager::acquire_lock(trx_t* trx, int table_id, pagenum_t page_id, int64_t key, LockMode mode) {

	LOCK_SYS_MUTEX_ENTER;
	Table* table = get_table(table_id);

	if (trx->getState() != RUNNING)
		PANIC("Cannot acquire lock in other mode.\n");

	int waiting_for_trx_id = -1;
	lock_t* front_lock_ptr = nullptr;
	lock_t* wait_lock = nullptr;
	lock_t* lock_ptr = nullptr;
	
	/**
		* If the lock request (equal or powerful) is already granted, return LOCK_SUCCESS immediately.
		*/
	if (lock_rec_has_lock(trx, table_id, page_id, key, mode, &front_lock_ptr))
		return LOCK_SUCCESS;
	
	/**
		* If the lock has no conflict with other lock requests, push lock request in list.
		* Then return LOCK_SUCCESS.
		*/
	else if (!lock_rec_has_conflict(trx, table_id, page_id, key, mode, front_lock_ptr, &wait_lock)){
		lock_granted_rec_add_to_queue(trx, table_id, page_id, key, mode, front_lock_ptr);
		return LOCK_SUCCESS;
	
	} else {
		/**
			* If there is conflict, check deadlock first.
			* If deadlock occurs, aborted current transaction. Then return DEADLOCK.
			* Otherwise, push current lock request in list then return LOCK_WAIT.
			*/
		if (dl_checker.deadlock_checking(trx -> getTransactionId(), lock_wait_for(trx, front_lock_ptr, wait_lock, mode))) {
			trx -> setState(ABORTED);
			release_lock_aborted(trx);
			return DEADLOCK;
		}

		lock_wait_rec_add_to_queue(trx, table_id, page_id, key, mode, front_lock_ptr, wait_lock);
		return LOCK_WAIT;
	}

	PANIC("In acquire lock. Unknown event.\n");
}

/**
	* lock_mode_compatible(lock1, lock2)      ----------------------     
  * (row , col) = (lock1 , lock2)           |  LOCK_S  | LOCK_X  |
  *															   ---------|----------|---------|
	*																 | LOCK_S |   True   |  False  |
	* 															 |--------|----------|---------|
	*																 | LOCK_X |   False  |  False  |
	*                                |--------|----------|---------|
	*/
const bool LockManager::lock_mode_compatible(LockMode lock_mode1, LockMode lock_mode2) {
	return (lock_compatibility_matrix[lock_mode1][lock_mode2]);
}

/**
	* lock_mode_stronger_or_eq(lock1, lock2)  ----------------------     
  * (row , col) = (lock1 , lock2)           |  LOCK_S  | LOCK_X  |
  *															   ---------|----------|---------|
	*																 | LOCK_S |   True   |  False  |
	* 															 |--------|----------|---------|
	*																 | LOCK_X |   True   |  True   |
	*                                |--------|----------|---------|
	*/

const bool LockManager::lock_mode_stronger_or_eq(LockMode lock_mode1, LockMode lock_mode2) {
	return (lock_strength_matrix[lock_mode1][lock_mode2]);
}

/**
	* lock_grant(lock_t*)
	*
	*@return (void).
	*
	* Acquire transaction mutex and release wait condition.
	* Push current lock request in active lock list. Then mutex exit.
	*/
void LockManager::lock_grant(lock_t* lock_ptr) {
	lock_ptr -> trx -> trx_mutex_enter();
	
	lock_ptr -> acquired = true;
	lock_ptr -> trx -> push_acquired_lock(lock_ptr);
	
	lock_ptr -> trx -> trx_wait_release();
	lock_ptr -> trx -> trx_mutex_exit();
}

/**
	* still_lock_wait(lock_t*)
	*
	*@return (bool) : If the lock request's transaction can be granted, return true. Otherwise return false.
	*/
bool LockManager::still_lock_wait(lock_t* lock) {
	int wait_for_trx_id = dl_checker.get_wait_lock_trx_id(lock -> trx -> getTransactionId());
	
	if (wait_for_trx_id == NO_WAIT_LOCK) {
		lock -> trx -> set_wait_lock(nullptr);
		return false;
	
	} else {
		for (lock_t* prev = lock -> prev; prev != nullptr;) {
			if (prev -> trx -> getTransactionId() == wait_for_trx_id) {
				lock -> trx -> set_wait_lock(prev);
				return true;
			}
		}
	}
	PANIC("In still lock wait.\n");
}

/**
	* rollback_data(trx_t*)
	*
	*@return (void).
	*
	* The all lock requests going to rollback are already only granted one.
	* So only buffer pool latch is acquired in this function.
	* Other transaction can acquire the page latch but cannot acquire lock sys mutex.
	*/
void LockManager::rollback_data(trx_t* trx) {

	BUF_POOL_MUTEX_ENTER;

	LeafPage* leaf_node = nullptr;
	Table* table = nullptr;
	std::list<undo_log> undo_log_list = trx -> get_undo_log_list();

	bool flag;
	
	for (auto rit = undo_log_list.rbegin(); rit != undo_log_list.rend(); ++rit) {
		flag = false;
		leaf_node = nullptr;
		table = get_table(rit -> table_id);

		leaf_node = (LeafPage*)get_page(table, rit -> page_id);

		for (int i = 0; i < leaf_node -> num_keys; i++) {
			if (LEAF_KEY(leaf_node, i) == rit -> key) {
				memcpy(LEAF_VALUE(leaf_node, i), rit -> undo_value, SIZE_VALUE);
				release_page((Page*)leaf_node);
				flag = true;
				break;
			}
		}	
		
		if (!flag)
			PANIC("In rollback_data. Cannot find the given key.\n");
	}

	BUF_POOL_MUTEX_EXIT;
}

/**
	* release_lock_aborted_low(trx_t*, lock_t*)
	*
	*@return (void).
	*
	* low-level function when release the lock request.
	* release current lock request and release possible other transactions waiting 
	* because of this lock request.
	*/
void LockManager::release_lock_aborted_low(trx_t* trx, lock_t* lock_ptr) {
	int page_id = lock_ptr -> page_id;		
	if (lock_ptr -> prev)
		lock_ptr -> prev -> next = lock_ptr -> next;
	if (lock_ptr -> next)
		lock_ptr -> next -> prev = lock_ptr -> prev;

	for (lock_t* lock = lock_ptr -> next; lock != nullptr;) {
		if (lock -> trx -> get_wait_lock() == lock_ptr && !still_lock_wait(lock)) {
			lock_grant(lock);
		}
		lock = lock -> next;
	}
	bool flag = true;
	
	for (auto it = lock_table[page_id].lock_list.begin(); it != lock_table[page_id].lock_list.end(); ++it) {
		if (&(*it) == lock_ptr) {
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
 * releases_lock_aborted(trx_t*)
 * trx_t*				: current transaction
 *@return (bool) : true.
 *
 * This function is called when deadlock occurs by given transaction.
 */
bool LockManager::release_lock_aborted(trx_t* trx) {
	rollback_data(trx);
	const std::list<lock_t*> acquired_lock = trx -> getAcquiredLock();
	
	for (auto rit = acquired_lock.rbegin(); rit != acquired_lock.rend(); ++rit) {
		release_lock_aborted_low(trx, *rit);
	}

	return true;
}
/**
	* release_lock_low(trx_t*, lock_t*)
	*
	*@return (void).
	*
	* low-level function when release the lock request.
  * release current lock request and release possible other transactions waiting	
  * because of this lock request.
  */
void LockManager::release_lock_low(trx_t* trx, lock_t* lock_ptr) {
	int page_id = lock_ptr -> page_id;
	if (lock_ptr -> prev)
		lock_ptr -> prev -> next = lock_ptr -> next;
	if (lock_ptr -> next)
		lock_ptr -> next -> prev = lock_ptr -> prev;

	for (lock_t* lock = lock_ptr -> next; lock != nullptr;) {
		if (lock -> trx -> get_wait_lock() == lock_ptr && !still_lock_wait(lock)) {
			lock_grant(lock);
		}
		lock = lock -> next;
	}
	bool flag = true;
	
	for (auto it = lock_table[page_id].lock_list.begin(); it != lock_table[page_id].lock_list.end(); ++it) {
		if (&(*it) == lock_ptr) {	
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
 * This function is called in end_tx().
 * release_lock(trx_t*)
 * trx_t*				: current transaction
 * return (bool) : true.
 */
bool LockManager::release_lock(trx_t* trx) {
	LOCK_SYS_MUTEX_ENTER;
	
	dl_checker.delete_waiting_for_trx(trx->getTransactionId());

	const std::list<lock_t*> acquired_lock = trx->getAcquiredLock();
	for (auto it = acquired_lock.begin(); it != acquired_lock.end(); ++it) {
		release_lock_low(trx, *it);
	}
	return true;
}
