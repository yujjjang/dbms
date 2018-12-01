#define NONE 					0
#define RUNNING				1
#define ABORTED				2

#define LOCK_S				0
#define LOCK_X				1

typedef int trx_id_t;
typedef int State;
typedef int LockMode;
