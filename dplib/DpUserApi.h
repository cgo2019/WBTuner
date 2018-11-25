#ifndef _DP_USER_API_
#define _DP_USER_API_

#include "DpLib.h"
#include "Sched.h"

void dpuAvgInc(int tID, int nmemb, int denom,
	double *pAvgResult, void *pSrc);
void dpuAvgAll(int tID, int nmemb, int denom, 
	double *pAvgResult, void *pSrc);
void dpuAvgCopy(int tID, int nmemb, void *pTrg, double *pAvgResult);
void dpuAvg(int aggIdx, void *trg);
void dpuMjVote(int tID, int nmemb, int *pCnt, void *pMjVal, void *pSrc);
int dpuReadResultParms(int resultIdx);
void dpuDumpParms(int resultIdx, FILE *file);
int dpuGetParms(int resultIdx, DPVar *pRandVar);
int dpuSetParmLo2Result(int resultIdx, int parmNum, ...);
int dpuSetParmHi2Result(int resultIdx, int parmNum, ...);
int dpuSetParm2Result(int resultIdx, int parmNum, ...);
int dpuSetParmLo(int parmNum, ...);
int dpuSetParmHi(int parmNum, ...);
int dpuSetParm(int parmNum, ...);
int dpuSetParmLoWithType(int parmNum, ...);
int dpuSetParmHiWithType(int parmNum, ...);

int dpuSetParmWithProb(int parmNum, ...);
void dpuResetParm();

int dpuRandSetParmWithProb(int parmNum, ...);

int dpuReadResultBin(int resultIdx);
void *dpuReadResultById(int resultIdx, int aggIdx);
void *dpuReadResult(int resultIdx, void *trg);
void *dpuReadCurResult(void *trg);

void *dpuReadSync(int syncIdx);

int dpuGetNmemb(int resultIdx, int aggIdx);

int dpuGetResultNum();

void dpuSplitSync(int resultIdx);

void dpuFreeResult(int resultIdx);
void dpuFreeResultAll();

void dpuRmResult(int resultIdx);

double dpuReadCVResult(int resultIdx);

#define dpuSplit(...) VFUNC(split, __VA_ARGS__)

#define split0()														\
	do {																			\
		schSched(S_SPAWN_TUNING, 0);						\
		fflush(stdout);													\
		fflush(stderr);													\
		if (fork() == 0) {											\
			fprintf(stderr, "%d\n", getpid());		\
			gDPContext.execStatus =								\
				(DP_PARENT | DP_FORKED_PARENT);			\
			dpParentInit();												\
			return;																\
		}																				\
	} while (0)

#define split1(_ret)												\
	do {																			\
		schSched(S_SPAWN_TUNING, 0);						\
		fflush(stdout);													\
		fflush(stderr);													\
		if (fork() == 0) {											\
			fprintf(stderr, "%d\n", getpid());		\
			gDPContext.execStatus =								\
				(DP_PARENT | DP_FORKED_PARENT);			\
			dpParentInit();												\
			return _ret;													\
		}																				\
	} while (0)

#endif
