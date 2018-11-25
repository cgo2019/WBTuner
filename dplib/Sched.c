#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <execinfo.h>
#include <signal.h>
#include "Sched.h"
#include "DpLib.h"

typedef struct SchPQNode {
	pid_t pid;
	SEvent event;
	int todo;
} SchPQNode;

pthread_mutex_t *schMtx = NULL;
pthread_mutex_t *schMtx2 = NULL;
int *schPQNum = NULL;
int *schWakePID = NULL;
SchPQNode *schPQ;
char *schD;
int *schPoolSize = NULL;
int debugPID = 0;

static void __attribute__ ((noinline)) schSignalHandler(int signum)
{
	//printf("Caught signal %d\n",signum);

	// Cleanup and close up stuff here
	return;
}

static void __attribute__ ((noinline)) schDebugHandler(int signum)
{
	//printf("Caught signal %d\n",signum);

	// Cleanup and close up stuff here
	printf("QQ:%d\n", getpid());
	if (getpid() == debugPID) {
		int i, j;
		for (i=0, j=0; i<1048576; i++) {
			if (schD[i] != 0) {
				j++;
				printf("(%d)", i);
				fflush(stdout);
			}
		}
		printf("\nTotal=%d\n", j);
	}

	_exit(0);
	return;
}

void handler(int sig) {
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	exit(1);
}

static int schPrioCmp(SchPQNode *pNode1, SchPQNode *pNode2)
{
	if (pNode1->event == S_SPAWN_SAMPLING) {
		if (pNode2->event == S_SPAWN_SAMPLING) {
			return pNode1->todo - pNode2->todo;
		} else
			return 1;
	} else
		return 0;
}

static void schSwap(SchPQNode *pNode1, SchPQNode *pNode2)
{
	SchPQNode tmpNode;

	memcpy(&tmpNode, pNode1, sizeof(SchPQNode));
	memcpy(pNode1, pNode2, sizeof(SchPQNode));
	memcpy(pNode2, &tmpNode, sizeof(SchPQNode));
}

static int schPushPQ(pid_t pid, SEvent event, int todo)
{
	if (*schPQNum == MAX_PARENT_NUM) {
		printf("Error<schPushPQ>: Priority queue full\n");
		return -1;
	}
	
	schPQ[*schPQNum].pid = pid;
	schPQ[*schPQNum].event = event;
	schPQ[*schPQNum].todo = todo;

	int itemIdx = *schPQNum;

	while (itemIdx > 0) {
		int parentItemIdx = (itemIdx - 1) / 2;

		if (schPrioCmp(&schPQ[itemIdx], &schPQ[parentItemIdx]) > 0) {
			schSwap(&schPQ[itemIdx], &schPQ[parentItemIdx]);
			itemIdx = parentItemIdx;
		} else {
			break;
		}
	}

	*schPQNum = *schPQNum + 1;

	return 0;
}

static int schPopPQ()
{
	*schPQNum = *schPQNum - 1;
	memcpy(&schPQ[0], &schPQ[*schPQNum], sizeof(SchPQNode));

	int itemIdx = 0;
	while (1) {
		int lChildIdx = (itemIdx << 1) + 1;
		int rChildIdx = lChildIdx + 1;
		
		if (*schPQNum > rChildIdx) {
			int lChildCmpResult = schPrioCmp(&schPQ[itemIdx], &schPQ[lChildIdx]);
			int rChildCmpResult = schPrioCmp(&schPQ[itemIdx], &schPQ[rChildIdx]);

			// Both child nodes have higher priority
			if (lChildCmpResult <= 0 && rChildCmpResult <= 0) {
				// Left child node has higher priority than right
				if (schPrioCmp(&schPQ[lChildIdx], &schPQ[rChildIdx]) > 0) {
					schSwap(&schPQ[itemIdx], &schPQ[lChildIdx]);
					itemIdx = lChildIdx;
				} else {
					schSwap(&schPQ[itemIdx], &schPQ[rChildIdx]);
					itemIdx = rChildIdx;
				}
			} else if (lChildCmpResult <= 0) {
				schSwap(&schPQ[itemIdx], &schPQ[lChildIdx]);
				itemIdx = lChildIdx;
			} else if (rChildCmpResult <= 0) {
				schSwap(&schPQ[itemIdx], &schPQ[rChildIdx]);
				itemIdx = rChildIdx;
			} else
				break;
		} else if (*schPQNum > lChildIdx) {
			int lChildCmpResult = schPrioCmp(&schPQ[itemIdx], &schPQ[lChildIdx]);
			
			if (lChildCmpResult <= 0) {
				schSwap(&schPQ[itemIdx], &schPQ[lChildIdx]);
				itemIdx = lChildIdx;
			} else
				break;
		} else
			break;
	}

	return 0;
}

static int schGetThreshold(SEvent event)
{
	if (event == S_SPAWN_SAMPLING) {
		return 0;
	} else {
		return MAX_POOL_SIZE - (MAX_POOL_SIZE >> 4);
	}
}

void schSchedInit()
{
	static pthread_mutexattr_t mtxAttr;
	
	schMtx = (pthread_mutex_t *) mmap(0, sizeof(pthread_mutex_t),
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);

	schMtx2 = (pthread_mutex_t *) mmap(0, sizeof(pthread_mutex_t),
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
	
	// Initialize shared mutex
	pthread_mutexattr_init(&mtxAttr);
	pthread_mutexattr_setpshared(&mtxAttr, PTHREAD_PROCESS_SHARED);
	
	pthread_mutex_init(schMtx, &mtxAttr);
	pthread_mutex_init(schMtx2, &mtxAttr);

	pthread_mutexattr_destroy(&mtxAttr);
	
	schPQNum = mmap(0, sizeof(int), 
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
	*schPQNum = 0;
	
	schWakePID = mmap(0, sizeof(int), 
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
	*schWakePID = -1;
	
	schPQ =  mmap(0, sizeof(SchPQNode) * MAX_PARENT_NUM,
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
	
	schD =  mmap(0, sizeof(char) * 1048576,
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
	memset(schD, 0, sizeof(char) * 1048576);
	
	schPoolSize = mmap(0, sizeof(int), 
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
	*schPoolSize = MAX_POOL_SIZE - 1;
	
	signal(SIGQUIT, schDebugHandler);
	debugPID = getpid();

}

void schParentInit()
{
	signal(SIGCONT, schSignalHandler);
}

void __attribute__ ((noinline, optimize("O0"))) 
	schSched(SEvent event, int todo)
{
#ifdef NO_SCHED
	return;
#endif

	pthread_mutex_lock(schMtx);

	if (event == S_SPAWN_SAMPLING || 
			event == S_SPAWN_TUNING)
	{
		int threshold = schGetThreshold(event);

		while (*schPoolSize <= threshold) {
			pid_t pid = getpid();

			schPushPQ(pid, event, todo);

			(*schPoolSize) = (*schPoolSize) + 1;
			
			pthread_mutex_unlock(schMtx);
			pause();
			
			pthread_mutex_lock(schMtx2);
			*schWakePID = pid;
			pthread_mutex_unlock(schMtx2);
			
			pthread_mutex_lock(schMtx);
			(*schPoolSize) = (*schPoolSize) - 1;
		}
		(*schPoolSize) = (*schPoolSize) - 1;

	} else {
		(*schPoolSize) = (*schPoolSize) + 1;

		if (*schPQNum != 0) {
			SchPQNode *pPQHead = &schPQ[0];
			int threshold = schGetThreshold(pPQHead->event);
		
			if (*schPoolSize <= threshold) {
				pthread_mutex_unlock(schMtx);
				return;
			}

			*schWakePID = -1;
			
			while (1) {
				pthread_mutex_lock(schMtx2);

				if (*schWakePID == pPQHead->pid) {
//					printf("%d signal %d done\n", getpid(), *schWakePID);
					schPopPQ();
					pthread_mutex_unlock(schMtx2);
					break;
				}
//				printf("%d signal\n", getpid());
				
				kill(pPQHead->pid, SIGCONT);
				pthread_mutex_unlock(schMtx2);
				sched_yield();
			}
		}
	}

//	printf("poolSize=%d %d END\n", *schPoolSize, *schPoolSize2);

	pthread_mutex_unlock(schMtx);
}

/*
void __attribute__ ((noinline, optimize("O0"))) 
	schSched(SEvent event, int todo)
{
#ifdef NO_SCHED
	return;
#endif

	pthread_mutex_lock(schMtx);
	printf("poolSize=%d poolSize2=%d\n", *schPoolSize, *schPoolSize2);
	
	if (event == S_SPAWN_SAMPLING || 
			event == S_SPAWN_TUNING)
	{
		(*schPoolSize2) = (*schPoolSize2) + 1;
		(*schPoolSize) = (*schPoolSize) - 1;
	} else {
		(*schPoolSize) = (*schPoolSize) + 1;
		(*schPoolSize2) = (*schPoolSize2) - 1;
	}
	printf("poolSize=%d poolSize2=%d\n", *schPoolSize, *schPoolSize2);
	pthread_mutex_unlock(schMtx);
}
*/
void __attribute__ ((noinline, optimize("O0"))) schDebug(int event)
{
	schD[getpid()] += event;
}
