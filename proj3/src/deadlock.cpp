#include "deadlock.h"

void DLChecker::initialize_tarjan_acyclic() {
	s_tarjan.clear();
	dfs_order = -1;
	cycle_flag = false;
	for (auto it = dl_graph.begin(); it != dl_graph.end(); ++it) {
		(it -> second).finished = false;
		(it -> second).dfs_order = -1;
	}
}

void DLChecker::initialize_tarjan_cyclic() {
	s_tarjan.clear();
	dfs_order = -1;
	cycle_flag = false;
	dl_graph.erase(latest_trx_id);
	vector<int> erase_list;
	
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
	cur->dfs_order = this->dfs_order++;
	s_tarjan.push_back(cur);

	int min_order = cur->dfs_order;
	
	tarjan_t* next = &(dl_graph[cur->waiting_trx_id]);

	if (next->dfs_order == -1)
		min_order = std::min(min_order, dfs_tarjan(next));
	else if (!next->finished)
		min_order = std::min(min_order, next->dfs_order);

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

bool DLChecker::is_cyclic() {
	// Tarjan's algorithm.
	for (auto it = dl_graph.begin(); it != dl_graph.end() ++it) {
		
		if (it->second.dfs_order == -1)
			dfs_tarjan(&(it->second));
		
		if (cycle_flag) {
			initialize_tarjan_cycle();
			return true;
		}
	}

	initialize_tarjan_acycle();
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
