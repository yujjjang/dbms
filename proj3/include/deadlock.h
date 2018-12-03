#ifndef __DEADLOCK__H__
#define __DEADLOCK__H__

#include <unordered_map>
#include <vector>
#include <stack>
#include <algorithm>
#include <climits>
#include <panic.h>

/**
	* DLChecker : Deadlock detection class.
	* dl_graph (directed graph) :  <trx_id , wait for>.
	*/

class DLChecker {
	struct tarjan_t {
		tarjan_t(){};
		tarjan_t(int wait_for) : waiting_trx_id(wait_for), finished(false), dfs_order(-1) {};
		int waiting_trx_id;
		bool finished;
		int dfs_order;
	};

	private:
		std::stack<tarjan_t*> s_tarjan;
		int dfs_order;
		bool cycle_flag;
		int latest_trx_id;

		int dfs_tarjan(tarjan_t* st);
		void initialize_tarjan_acyclic();
		void initialize_tarjan_cyclic();
		
		std::unordered_map<int, tarjan_t>  dl_graph;
		bool is_cyclic ();
	
	public:
		DLChecker(): dfs_order(-1), cycle_flag(false), latest_trx_id(0) {};
		~DLChecker(){};
		
		bool change_waiting_for(int trx_id, int wait_for);
		bool deadlock_checking(int trx_id, int wait_for);
		void delete_waiting_for_trx(int trx_id);
};

#endif
