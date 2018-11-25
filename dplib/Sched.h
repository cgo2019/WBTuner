#ifndef _SCHED_H_
#define _SCHED_H_

//#define NO_SCHED 1

#define MAX_POOL_SIZE 4096
#define MAX_PARENT_NUM 8192

typedef enum {
	S_SPAWN_SAMPLING = 0,
	S_SPAWN_TUNING,
	S_EXIT,
} SEvent;

void schSchedInit();
void schParentInit();
void __attribute__ ((noinline, optimize("O0"))) 
	schSched(SEvent event, int todo);

void __attribute__ ((noinline))schDebug(int event);
#endif
