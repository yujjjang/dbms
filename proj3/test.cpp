#include <mutex>
#include <iostream>
using namespace std;

int main(){
	mutex mut;
	unique_lock<mutex> l_mut(mut);
	if(l_mut.owns_lock())
		cout << "MY" << endl;
	else {
		cout << "RR" << endl;
		return -1;
	}
	l_mut.unlock();
	return 0;
}

