#include "trx.h"

TransactionManager trx_sys;

TransactionManager::~TransactionManager() {
	for (auto it = active_trx.begin(); it != active_trx.end(); ++it) {
		if (it -> second)
			delete it->second;
	}
	active_trx.clear();
}

trx_t* TransactionManager::makeNewTransaction() {
	trx_t* new_trx = new trx_t(trx_id++);
	active_trx[new_trx->getTransactionId()] = new_trx;
	return new_trx;
}

trx_t* TransactionManager::getTransaction(trx_id_t trx_id) {
	if (active_trx.count(trx_id) != 0)
			return active_trx[trx_id];
	else
		return nullptr;
}

bool TransactionManager::deleteTransaction(trx_id_t trx_id) {
	trx_t* trx = getTransaction(trx_id);
	if (!trx)
		return false;
	
	trx_t* trx_ptr = nullptr;
	
	for (auto it = active_trx.begin(); it != active_trx.end(); ++it) {
		if (it -> first == trx_id){
			trx_ptr = it -> second;
			active_trx.erase(trx_id);
			delete trx_ptr;
			return true;
		}
	}

	return false;
}
