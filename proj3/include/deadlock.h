#include <unordered_map>
#include <vector>
#include <stack>
#include <algorithm>
#include <panic.h>

class DLChecker {
	struct tarjan_t {
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
		
		// Cannot wait more than one transaction !.
		std::unordered_map<int, tarjan_t>  dl_graph;
		bool is_cyclic ();
	
	public:
		DLChecker(): dfs_order(-1), cycle_flag(false), latest_trx_id(0) {};
		~DLChecker(){};
		
		bool change_wating_list(int trx_id, int o_wait_for, int n_wait_for);
		bool deadlock_checking(int trx_id, int wait_for);
};
