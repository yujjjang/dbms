#include <unordered_map>
#include <iostream>

#include <vector>
#include <stack>
#include <algorithm>
using namespace std;
	struct tarjan_t {
		tarjan_t() : waiting_trx_id(-1), finished(false), dfs_order(-1) {};
		int waiting_trx_id;
		bool finished;
		int dfs_order;
	};

class DLChecker {
	public:
	std::stack<tarjan_t*> s_tarjan;
	int dfs_order = -1;
	int dfs_tarjan(tarjan_t* cur) {
		cur->dfs_order = this->dfs_order++;
		s_tarjan.push(cur);

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
	};
	bool cycle_flag = false;
	std::unordered_map<int, tarjan_t>  dl_graph;
	
	bool is_cyclic (){
		for (auto it = dl_graph.begin(); it != dl_graph.end(); ++it) {
			if (it->second.dfs_order == -1)
				dfs_tarjan(&(it->second));
			if (cycle_flag) {
				// 정리
				return true;
			}
		}
	}
	bool change_wating_list(int trx_id, int o_wait_for, int n_wait_for);
	bool deadlock_checking(int trx_id, int wait_for);
};

int main(){
	DLChecker dl;
	tarjan_t a;
	a.waiting_trx_id = 2;
	tarjan_t b;
	b.waiting_trx_id = 1;
	dl.dl_graph[1] = a;
	dl.dl_graph[2] = b;

	bool ret = dl.is_cyclic();
	cout << ret << endl;
	return 0;
}

