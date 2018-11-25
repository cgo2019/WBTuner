#ifndef _VOTING_STG_H_
#define _VOTING_STG_H_

#include "DpLib.h"

typedef enum {
	A_USER_CB = 0,
	A_AVG,
	A_MJ_VOTE,
	A_MAX,
	A_MIN,
	A_AVG_MEM,
	A_MJ_VOTE_MEM,
	A_MAX_MEM,
	A_MIN_MEM,
} AStg;

// Set target shm is ready and the result index
#define ASTG_SHM_SET_READY(_shmIdx, _resultIdx)				\
	gDPContext.pSharedTab->aggShmReady[(_shmIdx)] =		\
		((0x80000000) | (_resultIdx))

// Set target shm is not ready
#define ASTG_SHM_SET_UNREADY(_shmIdx)									\
	gDPContext.pSharedTab->aggShmReady[(_shmIdx)] = 0

// Check target shm ready or not
#define ASTG_SHM_READY(_shmIdx)														\
	(gDPContext.pSharedTab->aggShmReady[(_shmIdx)] & 0x80000000)

// Get the result index of target shm
#define ASTG_SHM_GET_RESULT_IDX(_shmIdx)									\
	(gDPContext.pSharedTab->aggShmReady[(_shmIdx)] & 0x7FFFFFFF)

typedef void (*astgFuncPtr)(int, void *, ...);  

extern astgFuncPtr astgFuncTab[];

void astgAggShmConfig(int opt, int shmIdx);

void astgAggShm();

int astgGetShmSize(int stg, int tSize, int nmemb);

double astgGetShmScore(int shmIdx, void *trg);

#endif
