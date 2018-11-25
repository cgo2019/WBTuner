#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "AggregateStg.h"
#include "DpLib.h"
#include "DpUserApi.h"
#include "SamplingIO.h"

#define ASTG_MAX_RANK_SIZE 10

typedef struct AStgRankNode {
	// Node's score
	double score;
	// Child PID produce this node
	pid_t pid;
	// Doubly linked list
	int prevIdx, nextIdx;
} AStgRankNode;

extern DPContext gDPContext;
extern int errno;

// No implementation for strategy 0 
static void astgStg0(int aggIdx, void *trg, ...) {}

// ------------------------------------ Aggregate strategy 1
// Vote for average
static void astgStg1(int aggIdx, void *trg, ...)
{
	dpuAvg(aggIdx, trg);
}

// ------------------------------------ Aggregate strategy 2
// Vote for majority
static void astgStg2(int aggIdx, void *trg, ...)
{
	DPResultTab *pResultTab = &gDPContext.resultTab;
	DPAggInfo *pAggInfo = &gDPContext.pSharedTab->aggInfo[aggIdx];

	int i;
	void *pMjVal;
	int *pCnt;
	TypeID tID;
	int nmemb;
	int tSize;

	tID = pAggInfo->tID;
	nmemb = pAggInfo->nmemb;
	tSize = pAggInfo->tSize;

	pMjVal = (void *) malloc(tSize * nmemb);
	memset(pMjVal, 0, tSize * nmemb);
	
	pCnt = (int *) malloc(sizeof(int) * nmemb);
	memset(pCnt, 0, sizeof(int) * nmemb);

	for (i=pResultTab->resultHeadIdx; i<pResultTab->resultTailIdx; i++) {
			void *pResult = dpuReadResultById(i, aggIdx);
			dpuMjVote(tID, nmemb, pCnt, pMjVal, pResult);
	}

	// Write result back
	memcpy(trg, pMjVal, tSize * nmemb);

	free(pMjVal);
	free(pCnt);
}

// ------------------------------------ Aggregate strategy 3
// Search for the max score
//
// Three additional functions:
// 1. Init function before executing score function
// 2. Fini function after executing score functions for all samples
// 3. Score function for each sample
static void astgStg3(int aggIdx, void *trg, ...)
{
	va_list vl;
	int i, trgIdx;
	double trgScore;
	double (*scoreFunc)(void *, int);
	void (*scoreInitFunc)(void);
	void (*scoreFiniFunc)(void);
	DPResultTab *pResultTab = &gDPContext.resultTab;
	int nmemb = gDPContext.pSharedTab->aggInfo[aggIdx].nmemb;
	int tSize = gDPContext.pSharedTab->aggInfo[aggIdx].tSize;
	void *pResult;

	va_start(vl, trg);
	
	scoreInitFunc = va_arg(vl, void *);
	scoreFiniFunc = va_arg(vl, void *);
	scoreFunc = va_arg(vl, void *);
	
	va_end(vl);
/*
	// Found best result during sampling
	if (pResultTab->bestResultIdx >= 0) {
		trgIdx = pResultTab->bestResultIdx;
		trgScore = pResultTab->bestResultScore;

		pResult = dpuReadResultById(trgIdx, aggIdx);

		memcpy(trg, pResult, nmemb * tSize);
	
		dpuDumpParms(trgIdx, stdout);
		printf("Max score=%lf\n", trgScore);

		return;
	}
*/
	// If the best result not found during sampling, do it now
	trgIdx = -1;

	scoreInitFunc();
	
	for (i=0; i<pResultTab->resultTailIdx; i++) {
		void *pResult = dpuReadResult(i, trg);
		double score = scoreFunc(pResult, i);
	
		if (trgIdx == -1 || score > trgScore) {
			trgScore = score;
			trgIdx = i;
		}
		//printf("%lf\n", score);

		//if (gDPContext.execStatus & DP_FORKED_PARENT)
		//	break;
	}
	
	scoreFiniFunc();

	pResult = dpuReadResultById(trgIdx, aggIdx);

	memcpy(trg, pResult, nmemb * tSize);
	
	dpuDumpParms(trgIdx, stdout);

	printf("Max score=%lf\n", trgScore);
}

// ------------------------------------ Aggregate strategy 4
// Search for the min score
//
// Three additional functions:
// 1. Init function before executing score function
// 2. Fini function after executing score functions for all samples
// 3. Score function for each sample
static void astgStg4(int aggIdx, void *trg, ...)
{
	va_list vl;
	int i, trgIdx;
	double trgScore;
	double (*scoreFunc)(void *, int);
	void (*scoreInitFunc)(void);
	void (*scoreFiniFunc)(void);
	DPResultTab *pResultTab = &gDPContext.resultTab;
	int nmemb = gDPContext.pSharedTab->aggInfo[aggIdx].nmemb;
	int tSize = gDPContext.pSharedTab->aggInfo[aggIdx].tSize;
	void *pResult;

	va_start(vl, trg);
	
	scoreInitFunc = va_arg(vl, void *);
	scoreFiniFunc = va_arg(vl, void *);
	scoreFunc = va_arg(vl, void *);
	
	va_end(vl);
/*
	// Found best result during sampling
	if (pResultTab->bestResultIdx >= 0) {
		trgIdx = pResultTab->bestResultIdx;
		trgScore = pResultTab->bestResultScore;

		pResult = dpuReadResultById(trgIdx, aggIdx);

		memcpy(trg, pResult, nmemb * tSize);
	
		dpuDumpParms(trgIdx, stdout);
		
		printf("Min score=%lf\n", trgScore);

		return;
	}
*/
	// If the best result not found during sampling, do it now
	trgIdx = -1;
	
	scoreInitFunc();
	
	for (i=0; i<pResultTab->resultTailIdx; i++) {
		void *pResult = dpuReadResult(i, trg);
		double score = scoreFunc(pResult, i);

		if (trgIdx == -1 || score < trgScore) {
			trgScore = score;
			trgIdx = i;
		}
		printf("%lf\n", score);

		// ?
		//if (gDPContext.execStatus & DP_FORKED_PARENT)
		//	break;
	}
	
	scoreFiniFunc();

	pResult = dpuReadResultById(trgIdx, aggIdx);

	memcpy(trg, pResult, nmemb * tSize);
	
	dpuDumpParms(trgIdx, stdout);

	printf("Min score=%lf\n", trgScore);
}

// ------------------------------------ Aggregate strategy 5
// Average the result with shared memory
//
static void astgStg5(int aggIdx, void *trg, ...)
{
	DPSharedTab *pSharedTab = gDPContext.pSharedTab;
	DPAggInfo *pAggInfo = &pSharedTab->aggInfo[aggIdx];
	int tID, tSize, nmemb, shmSize;
	void *pShm;
	// Aggregation result related
	static double *pAvgResult = NULL;
	static int denom = 0;

	tID = pSharedTab->aggInfo[aggIdx].tID;
	tSize = pSharedTab->aggInfo[aggIdx].tSize;
	nmemb = pSharedTab->aggInfo[aggIdx].nmemb;
	shmSize = astgGetShmSize(5, tSize, nmemb);

	if (gDPContext.execStatus == DP_CHILD) {
		int shmIdx;
		va_list vl;

		va_start(vl, trg);
		shmIdx = va_arg(vl, int);
		va_end(vl);

		// Parent does not create the shared memory yet,
		// so child process needs to create by itself
		if (!pAggInfo->shmResult[shmIdx]) {
			if (!(pShm = sioCreateShm(getppid(), aggIdx, shmIdx, shmSize))) {
				printf("Error<astgStg5>: Child cannot create shared memory\n");
				dpExit(-1);
			}
		} else
			pShm = pAggInfo->shmResult[shmIdx];

		// Write the sampling result to shared memory
		memcpy(pShm, trg, shmSize);
	} else if (gDPContext.inSampling) {
		int i;

		if (!pAvgResult) {
			pAvgResult = (double *) malloc(sizeof(double) * nmemb);
			memset(pAvgResult, 0, sizeof(double) * nmemb);
		}

		for (i=0; i<pSharedTab->aggShmNum; i++) {
			// For each sampling result, we need to check whether
			// data is ready for aggregation
			if (!ASTG_SHM_READY(i))
				continue;

				pShm = pAggInfo->shmResult[i];
			// Do aggregation
			denom++;
			dpuAvgInc(tID, nmemb, denom, pAvgResult, pShm);
		}
	} else {
		// Write aggregation result
		dpuAvgCopy(tID, nmemb, trg, pAvgResult);
		
		// Cleanup for future use
		free(pAvgResult);
		pAvgResult = NULL;
		denom = 0;
	}
}

// ------------------------------------ Aggregate strategy 6
// Vote for majority with shared memory
//
static void astgStg6(int aggIdx, void *trg, ...)
{
	DPSharedTab *pSharedTab = gDPContext.pSharedTab;
	DPAggInfo *pAggInfo = &pSharedTab->aggInfo[aggIdx];
	int tID, tSize, nmemb, shmSize;
	void *pShm;
	// Aggregation result related
	static int *pCnt = NULL;
	static void *pMjVal = NULL;

	tID = pSharedTab->aggInfo[aggIdx].tID;
	tSize = pSharedTab->aggInfo[aggIdx].tSize;
	nmemb = pSharedTab->aggInfo[aggIdx].nmemb;
	shmSize = astgGetShmSize(6, tSize, nmemb);

	if (gDPContext.execStatus == DP_CHILD) {
		int shmIdx;
		va_list vl;

		va_start(vl, trg);
		shmIdx = va_arg(vl, int);
		va_end(vl);

		// Parent does not create the shared memory yet,
		// so child process needs to create by itself
		if (!pAggInfo->shmResult[shmIdx]) {
			if (!(pShm = sioCreateShm(getppid(), aggIdx, shmIdx, shmSize))) {
				printf("Error<astgStg6>: Child cannot create shared memory\n");
				dpExit(-1);
			}
		} else
			pShm = pAggInfo->shmResult[shmIdx];

		// Write the sampling result to shared memory
		memcpy(pShm, trg, shmSize);
	} else if (gDPContext.inSampling) {
		int i;

		if (!pCnt || !pMjVal) {
			pCnt = (int *) malloc(sizeof(int) * nmemb);
			memset(pCnt, 0, sizeof(int) * nmemb);
			pMjVal = (void *) malloc(tSize * nmemb);
			memset(pMjVal, 0, tSize * nmemb);
		}

		for (i=0; i<pSharedTab->aggShmNum; i++) {
			// For each sampling result, we need to check whether
			// data is ready for aggregation
			if (!ASTG_SHM_READY(i))
				continue;

			pShm = pAggInfo->shmResult[i];
			
			// Do aggregation
			dpuMjVote(tID, nmemb, pCnt, pMjVal, pShm);
		}
	} else {
		// Write aggregation result
		memcpy(trg, pMjVal, shmSize);

		// Cleanup for future use
		free(pCnt);
		pCnt = NULL;
		free(pMjVal);
		pMjVal = NULL;
	}
}

// ------------------------------------ Aggregate strategy 7
// Get Max
//
static void astgStg7(int aggIdx, void *trg, ...)
{
	DPSharedTab *pSharedTab = gDPContext.pSharedTab;
	DPAggInfo *pAggInfo = &pSharedTab->aggInfo[aggIdx];
	int tSize, nmemb, shmSize;
	void *pShm;
	// Aggregation result related
	static void *pMax = NULL;
	static double maxScore;
	static int maxScoreIdx;

	tSize = pSharedTab->aggInfo[aggIdx].tSize;
	nmemb = pSharedTab->aggInfo[aggIdx].nmemb;
	shmSize = astgGetShmSize(7, tSize, nmemb);

	if (gDPContext.execStatus == DP_CHILD) {
		va_list vl;
		double (*scoreFunc)(void *);
		void (*scoreInitFunc)(void);
		void (*scoreFiniFunc)(void);
		int shmIdx;
		double score;

		va_start(vl, trg);

		shmIdx = va_arg(vl, int);
		scoreInitFunc = va_arg(vl, void *);
		scoreFiniFunc = va_arg(vl, void *);
		scoreFunc = va_arg(vl, void *);

		va_end(vl);

		scoreInitFunc();
		score = scoreFunc(trg);
		scoreFiniFunc();

		// Parent does not create the shared memory yet,
		// so child process needs to create by itself
		if (!pAggInfo->shmResult[shmIdx]) {
			if (!(pShm = sioCreateShm(getppid(), aggIdx, shmIdx, shmSize))) {
				printf("Error<astgStg7>: Child cannot create shared memory\n");
				dpExit(-1);
			}
		} else
			pShm = pAggInfo->shmResult[shmIdx];

		// Write the sampling result to shared memory
		memcpy(pShm, trg, tSize * nmemb);
		// Write the score to shared memory
		memcpy(((char *)pShm + (tSize * nmemb)), &score, sizeof(double));
	} else if (gDPContext.inSampling) {
		int i;

		// Initialize static variables
		if (!pMax) {
			pMax = (void *) malloc(tSize * nmemb);
			memset(pMax, 0, tSize * nmemb);
			maxScore = 0;
			maxScoreIdx = -1;
		}

		for (i=0; i<pSharedTab->aggShmNum; i++) {
			// For each sampling result, we need to check whether
			// it is ready for aggregation
			if (!ASTG_SHM_READY(i))
				continue;
			
			pShm = pAggInfo->shmResult[i];
			// Do aggregation
			double *pScore = (double *)(((char *)pShm) + (tSize * nmemb));
			
			if (maxScoreIdx == -1 || *pScore > maxScore) {
				maxScore = *pScore;
				memcpy(pMax, pShm, tSize * nmemb);
				maxScoreIdx = ASTG_SHM_GET_RESULT_IDX(i);
			}
		}
	} else {
		// Write aggregation result
		memcpy(trg, pMax, tSize * nmemb);

		// Dump parameters and max score
		dpuDumpParms(maxScoreIdx, stdout);
		
		printf("Max score=%lf\n", maxScore);
		
		// Cleanup for future use
		free(pMax);
		pMax = NULL;
	}
}

// ------------------------------------ Aggregate strategy 8
// Get Min
//
static void astgStg8(int aggIdx, void *trg, ...)
{
	DPSharedTab *pSharedTab = gDPContext.pSharedTab;
	DPAggInfo *pAggInfo = &pSharedTab->aggInfo[aggIdx];
	int tSize, nmemb, shmSize;
	void *pShm;
	// Aggregation result related
	static void *pMin = NULL;
	static double minScore;
	static int minScoreIdx;

	tSize = pSharedTab->aggInfo[aggIdx].tSize;
	nmemb = pSharedTab->aggInfo[aggIdx].nmemb;
	shmSize = astgGetShmSize(8, tSize, nmemb);

	if (gDPContext.execStatus == DP_CHILD) {
		va_list vl;
		double (*scoreFunc)(void *);
		void (*scoreInitFunc)(void);
		void (*scoreFiniFunc)(void);
		int shmIdx;
		double score;

		va_start(vl, trg);

		shmIdx = va_arg(vl, int);
		scoreInitFunc = va_arg(vl, void *);
		scoreFiniFunc = va_arg(vl, void *);
		scoreFunc = va_arg(vl, void *);

		va_end(vl);

		scoreInitFunc();
		score = scoreFunc(trg);
		scoreFiniFunc();

		// Parent does not create the shared memory yet,
		// so child process needs to create by itself
		if (!pAggInfo->shmResult[shmIdx]) {
			if (!(pShm = sioCreateShm(getppid(), aggIdx, shmIdx, shmSize))) {
				printf("Error<astgStg8>: Child cannot create shared memory\n");
				dpExit(-1);
			}
		} else
			pShm = pAggInfo->shmResult[shmIdx];

		// Write the sampling result to shared memory
		memcpy(pShm, trg, tSize * nmemb);
		// Write the score to shared memory
		memcpy(((char *)pShm + (tSize * nmemb)), &score, sizeof(double));
	} else if (gDPContext.inSampling) {
		int i;

		// Initialize static variables
		if (!pMin) {
			pMin = (void *) malloc(tSize * nmemb);
			memset(pMin, 0, tSize * nmemb);
			minScore = 0;
			minScoreIdx = -1;
		}

		for (i=0; i<pSharedTab->aggShmNum; i++) {
			// For each sampling result, we need to check whether
			// it is ready for aggregation
			if (!ASTG_SHM_READY(i))
				continue;
			
			pShm = pAggInfo->shmResult[i];
			// Do aggregation
			double *pScore = (double *)(((char *)pShm) + (tSize * nmemb));
			
			if (minScoreIdx == -1 || *pScore < minScore) {
				minScore = *pScore;
				memcpy(pMin, pShm, tSize * nmemb);
				minScoreIdx = ASTG_SHM_GET_RESULT_IDX(i);
			}
		}
	} else {
		// Write aggregation result
		memcpy(trg, pMin, tSize * nmemb);

		// Dump parameters and min score
		dpuDumpParms(minScoreIdx, stdout);
		
		printf("Min score=%lf\n", minScore);

		// Cleanup for future use
		free(pMin);
		pMin = NULL;
	}
}

// opt: 0 for init, 1 for fini
// shmIdx: only used when opt = 0
void astgAggShmConfig(int opt, int shmIdx)
{
	int i, j, shmSize;
	void *pShm;
	DPSharedTab *pSharedTab = gDPContext.pSharedTab;
	static char shmInited[MAX_SAMPLING_SUB_TODO] = {0};

	// Init
	if (opt == 0) {
		if (shmInited[shmIdx])
			return;

		for (i=0; i<pSharedTab->aggNum; i++) {
			DPAggInfo *pAggInfo = &pSharedTab->aggInfo[i];

			switch (pAggInfo->stg) {
				case A_AVG_MEM:
				case A_MJ_VOTE_MEM:
				case A_MAX_MEM:
				case A_MIN_MEM:
					if (!pAggInfo->shmResult[shmIdx]) {
						shmSize = 
							astgGetShmSize(pAggInfo->stg, pAggInfo->tSize, pAggInfo->nmemb);

						if (!(pShm = sioCreateShm(getpid(), i, shmIdx, shmSize))) {
							printf("Error<astgAggShmConfig>: "
									"Parent cannot create shared memory of shmIdx=%d\n", shmIdx);
							dpExit(-1);
						} else {
							pAggInfo->shmResult[shmIdx] = pShm;
						}
					}
					break;
				default:
					break;
			}
		}

		shmInited[shmIdx] = 1;
	} 
	// Fini
	else {
		for (i=0; i<pSharedTab->aggNum; i++) {
			DPAggInfo *pAggInfo = &gDPContext.pSharedTab->aggInfo[i];
			int shmSize = 
				astgGetShmSize(pAggInfo->stg, pAggInfo->tSize, pAggInfo->nmemb);

			switch (pAggInfo->stg) {
				case A_AVG_MEM:
				case A_MJ_VOTE_MEM:
				case A_MAX_MEM:
				case A_MIN_MEM:
					for (j=0; j<MAX_SAMPLING_SUB_TODO; j++) {
						if (pAggInfo->shmResult[j] &&
								sioFreeShm(pAggInfo->shmResult[j], 
									getpid(), i, j, shmSize) < 0)
						{
							printf("Error<astgAggShmConfig>: "
									"Parent failed to free shared memory\n");
							dpExit(-1);
						}
					}
					break;
				default:
					break;
			}
		}

		memset(shmInited, 0, sizeof(char) * MAX_SAMPLING_SUB_TODO);
	}
}

void astgAggShm()
{
	int i;
	DPSharedTab *pSharedTab = gDPContext.pSharedTab;

	for (i=0; i<pSharedTab->aggNum; i++) {
		int stg = pSharedTab->aggInfo[i].stg;

		switch (stg) {
			case A_AVG_MEM:
			case A_MJ_VOTE_MEM:
			case A_MAX_MEM:
			case A_MIN_MEM:
				astgFuncTab[stg](i, NULL);
				break;
			default:
				break;
		}
	}

	// Cleanup for next round
	if (gDPContext.pSharedTab->aggShmNum) {
		memset(gDPContext.pSharedTab->aggShmReady, 0, 
			sizeof(int) * gDPContext.pSharedTab->aggShmNum);
		gDPContext.pSharedTab->aggShmNum = 0;
	}
}

int astgGetShmSize(int stg, int tSize, int nmemb)
{
	switch (stg) {
		case A_AVG_MEM:
				return tSize * nmemb;
		case A_MJ_VOTE_MEM:
				return tSize * nmemb;
			return 0;
		case A_MAX_MEM:
		case A_MIN_MEM:
				return sizeof(double) + (tSize * nmemb);
			return 0;
		default:
			return 0;
	}
}

double astgGetShmScore(int shmIdx, void *trg)
{
	int i;
	DPSharedTab *pSharedTab = gDPContext.pSharedTab;
	double score;
	void *pShm;

	for (i=0; i<pSharedTab->aggNum; i++) {
		DPAggInfo *pAggInfo = &pSharedTab->aggInfo[i];

		if (pAggInfo->trg == trg) {
			switch (pAggInfo->stg) {
				case A_MAX_MEM:
				case A_MIN_MEM:
					pShm = pAggInfo->shmResult[shmIdx];
					score = 
						*((double *)((char *)pShm + (pAggInfo->tSize * pAggInfo->nmemb)));
					return score;
				default:
					dpExit(-1);
			}
		}
	}

	dpExit(-1);

	return 0;
}

astgFuncPtr astgFuncTab[] = {
	astgStg0,
	astgStg1,
	astgStg2,
	astgStg3,
	astgStg4,
	astgStg5,
	astgStg6,
	astgStg7,
	astgStg8,
};
