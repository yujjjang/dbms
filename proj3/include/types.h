#ifndef __TYPES_H__
#define __TYPES_H__

#define NONE 					0
#define RUNNING				1
#define ABORTED				2

#define LOCK_S				0
#define LOCK_X				1

#define LOCK_SUCCESS  0
#define LOCK_WAIT     1
#define DEADLOCK      2

typedef int trx_id_t;
typedef int State;
typedef int LockState;
typedef int LockMode;


#endif
