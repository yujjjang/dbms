#include "deadlock.h"

/**
	* initialize_tarjan_acyclic()
	* Initialize all variables used in detection algorithm.
	*/
void DLChecker::initialize_tarjan_acyclic() {
	while (!s_tarjan.empty())
		s_tarjan.pop();
	
	dfs_order = -1;
	cycle_flag = false;
	
	for (auto it = dl_graph.begin(); it != dl_graph.end(); ++it) {
		(it -> second).finished = false;
		(it -> second).dfs_order = -1;
	}
}

/**
	* initialize_tarjan_cyclic()
	* Erase latest transaction in dl_graph and the vertex waiting for the latest one.
 	* Initialize all variables used in detection algorithm.
	*/
void DLChecker::initialize_tarjan_cyclic() {
	while (!s_tarjan.empty())
		s_tarjan.pop();
	
	dfs_order = -1;
	cycle_flag = false;
	
	dl_graph.erase(latest_trx_id);

	std::vector<int> erase_list;

	tarjan_t* tarjan = nullptr;
	
	for (auto it = dl_graph.begin(); it != dl_graph.end(); ++it) {
		tarjan = &(it -> second);
		
		for (auto local_it = tarjan -> waiting_trx_id.begin(); local_it != tarjan -> waiting_trx_id.end(); ++local_it) {
			if (*local_it == latest_trx_id) {
				tarjan -> waiting_trx_id.erase(local_it);
				--local_it;
			}
		}
		tarjan -> finished = false;
		tarjan -> dfs_order = -1;

		if (tarjan -> waiting_trx_id.size() == 0) {
			erase_list.push_back(it -> first);
		}
	}
	
	for (auto i : erase_list)
		dl_graph.erase(i);
}

/**
	* dfs_tarjan(tarjan_t*)
	* tarjan_t*				: current Vertex
	*	@return (int)		: the mininum order
	*/
int DLChecker::dfs_tarjan(tarjan_t* cur) {
	cur -> dfs_order = ++(this -> dfs_order);
	s_tarjan.push(cur);

	int min_order = cur -> dfs_order;
	
	for (auto it = cur -> waiting_trx_id.begin(); it != cur -> waiting_trx_id.end(); ++it){
		if (dl_graph.count(*it) != 0){
			tarjan_t* next = &(dl_graph[*it]);	
			
			if (next -> dfs_order == -1)
				min_order = std::min(min_order, dfs_tarjan(next));
			else if (!next -> finished)
				min_order = std::min(min_order, next -> dfs_order);

			if (cycle_flag)
				return INT_MAX;
		}
	}

	if (cycle_flag)
		return INT_MAX;
	
	if (min_order == cur -> dfs_order) {
		tarjan_t* tmp = nullptr;
		int cnt_size = 0;
		while (1) {
			if (cnt_size == 1)
				cycle_flag = true;
			cnt_size++;
			tmp = s_tarjan.top();
			s_tarjan.pop();

			if (cur == tmp)
				break;
		}
	}

	cur -> finished = true;
	return min_order;	
}

/**
	* is_cyclic()
	*
	*@return (bool) : cyclic (true) or not.
	* Main Function in DLChecker.
	* DeadLock detection algorithm using "tarjan's" which is used to find SCC.
	*/
bool DLChecker::is_cyclic() {
	for (auto it = dl_graph.begin(); it != dl_graph.end(); ++it) {
		
		if (it -> second.dfs_order == -1)
			dfs_tarjan(&(it -> second));
		
		if (cycle_flag) {
			initialize_tarjan_cyclic();
			return true;
		}
	}
	initialize_tarjan_acyclic();
	return false;
}

/**
	* deadlock_checking(trx_id, wait_for)
	* trx_id				: current transaction's id
	* wait_for			: the transaction's id which current transaction is waiting for.
	*@return (bool) : cyclic or acyclic.
	*
	* This function is called when pushing lock request in list first time.
	*/
bool DLChecker::deadlock_checking(int trx_id, std::vector<int> wait_for) {
	
	if (dl_graph.count(trx_id) != 0)
		PANIC("In deadlock checking. The transaction cannot wait for more than one transaction.\n");
	
	tarjan_t cur(wait_for);
	latest_trx_id = trx_id;
	
	dl_graph[trx_id] = cur;

	return is_cyclic();
}

/**
	* delete_waiting_for_trx(trx_id)
	* trx_id				: current transaction's id
	*@return (void).
	*
	* This function is called when the transaction is committed.
	*/
void DLChecker::delete_waiting_for_trx(int trx_id) {
	
	if (dl_graph.count(trx_id) != 0)
		PANIC("In delete waiting for trx. Committing transaction cannot wait for some other one.\n");
  
	std::vector<int> erase_list;
	tarjan_t* tarjan = nullptr;
	
	for (auto it = dl_graph.begin(); it != dl_graph.end(); ++it) {
		tarjan = &(it -> second);
		
		for (auto local_it = tarjan -> waiting_trx_id.begin(); local_it != tarjan -> waiting_trx_id.end(); ++local_it) {
			if (*local_it == trx_id) {
				tarjan -> waiting_trx_id.erase(local_it);
				--local_it;
			}
		}
		if (tarjan -> waiting_trx_id.size() == 0) {
			erase_list.push_back(it -> first);
		}
	}
	
	for (auto i : erase_list)
		dl_graph.erase(i);
}

/**
	* get_wait_lock_trx_id(trx_id)
	*
	*@return (int) : If there is no wait lock whose transaction id is same as given trx_id return NO_WAIT_LOCK.
	*								 Otherwise return recent transaction id.
	*/
int DLChecker::get_wait_lock_trx_id(int trx_id) {
	if (dl_graph.count(trx_id) == 0)
		return NO_WAIT_LOCK;
	else
		return dl_graph[trx_id].waiting_trx_id.back();
}
