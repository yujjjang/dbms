#include "deadlock.h"

void DLChecker::initialize_tarjan_acyclic() {
	while(!s_tarjan.empty())
		s_tarjan.pop();
	
	dfs_order = -1;
	cycle_flag = false;
	
	for (auto it = dl_graph.begin(); it != dl_graph.end(); ++it) {
		(it -> second).finished = false;
		(it -> second).dfs_order = -1;
	}
}

void DLChecker::initialize_tarjan_cyclic() {
	while (!s_tarjan.empty())
		s_tarjan.pop();
	
	dfs_order = -1;
	cycle_flag = false;
	
	// Delete the latest Vertex <latest_trx_id, tarjan_t>, and the Vertex waiting for the latest one.
	dl_graph.erase(latest_trx_id);
	
	std::vector<int> erase_list;
	for (auto it = dl_graph.begin(); it != dl_graph.end(); ++it) {
		if ((it -> second).waiting_trx_id == latest_trx_id)
			erase_list.push_back(it->first);
		
		else {
			(it -> second).finished = false;
			(it -> second).dfs_order = -1;
		}
	}

	for (auto i : erase_list)
		dl_graph.erase(i);
	
}

int DLChecker::dfs_tarjan(tarjan_t* cur) {
	cur->dfs_order = ++this->dfs_order;
	s_tarjan.push(cur);

	int min_order = cur->dfs_order;
	
	if (dl_graph.count(cur->waiting_trx_id) != 0){
	
		tarjan_t* next = &(dl_graph[cur->waiting_trx_id]);	
		if (next->dfs_order == -1)
			min_order = std::min(min_order, dfs_tarjan(next));
		else if (!next->finished)
			min_order = std::min(min_order, next->dfs_order);
	}

	if (cycle_flag)
		return INT_MAX;
	
	if (min_order == cur->dfs_order) {
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
	* DeadLock detection algorithm using "tarjan's algorithm" which can be used to find SCC.
	*/
bool DLChecker::is_cyclic() {
	for (auto it = dl_graph.begin(); it != dl_graph.end(); ++it) {
		
		if (it->second.dfs_order == -1)
			dfs_tarjan(&(it->second));
		
		if (cycle_flag) {
			initialize_tarjan_cyclic();
			return true;
		}
	}

	initialize_tarjan_acyclic();
	return false;
}

/**
	* Main function in deadlock detection.
	*/
bool DLChecker::deadlock_checking(int trx_id, int wait_for) {
	tarjan_t cur(wait_for);
	
	if (dl_graph.count(trx_id) != 0)
		PANIC("In deadlock checking. The transaction cannot wait for more than one transaction.\n");
	
	dl_graph[trx_id] = cur;
	latest_trx_id = trx_id;

	return is_cyclic();
}

void DLChecker::delete_waiting_for_trx(int trx_id) {
	std::vector<int> erase_list;

	if (dl_graph.count(trx_id) == 0)
		PANIC("In delete waiting for trx. Vertex[trx_id] doesn't exist.\n");

	dl_graph.erase(trx_id);

	for (auto it = dl_graph.begin(); it != dl_graph.end(); ++it) {
		if ((it -> second).waiting_trx_id == trx_id)
			erase_list.push_back(it -> first);
	}

	for (auto i : erase_list)
		dl_graph.erase(i);
}

bool DLChecker::change_waiting_for(int trx_id, int wait_for) {
	if (dl_graph.count(trx_id) == 0)
		PANIC("In change_waiting_for. There is no wait for lock.\n");
	tarjan_t cur(wait_for);
	dl_graph[trx_id] = cur;

	latest_trx_id = trx_id;

	return is_cyclic();
}
