#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>
#include "DpUserApi.h"
#include "SamplingIO.h"
#include "AggregateStg.h"

extern DPContext gDPContext;
extern errno;

void dpuAvgInc(int tID, int nmemb, int denom,
	double *pAvgResult, void *pSrc)
{
#define DPU_AVG_INC(_type, _nmemb, _denom, _avgResult, _src)	\
	do {																												\
		int i;																										\
		_type *pS = (_type *)(_src);															\
																															\
		for (i=0; i<nmemb; i++) {																	\
			_avgResult[i] +=																				\
				(((double)pS[i] - _avgResult[i]) / (double)_denom);		\
		}																													\
	} while (0)

	switch (tID) {
		case dpC:
		{
			DPU_AVG_INC(char, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpS:
		{
			DPU_AVG_INC(short, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpI:
		{
			DPU_AVG_INC(int, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpL:
		{
			DPU_AVG_INC(long long, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpF:
		{
			DPU_AVG_INC(float, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpD:
		{
			DPU_AVG_INC(double, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpUC:
		{
			DPU_AVG_INC(unsigned char, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpUS:
		{
			DPU_AVG_INC(unsigned short, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpUI:
		{
			DPU_AVG_INC(unsigned int, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpUL:
		{
			DPU_AVG_INC(unsigned long long, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		default:
			printf("Do nothing!!!!!\n");
			break;
	}

#undef DPU_AVG_INC
}

void dpuAvgAll(int tID, int nmemb, int denom, 
	double *pAvgResult, void *pSrc)
{
#define DPU_AVG_ALL(_type, _nmemb, _denom, _avgResult, _src)	\
	do {																												\
		int i;																										\
		_type *pS = (_type *)(_src);															\
																															\
		for (i=0; i<nmemb; i++) {																	\
			_avgResult[i] +=																				\
				(((double)pS[i]) / (double)_denom);										\
		}																													\
	} while (0)

	switch (tID) {
		case dpC:
		{
			DPU_AVG_ALL(char, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpS:
		{
			DPU_AVG_ALL(short, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpI:
		{
			DPU_AVG_ALL(int, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpL:
		{
			DPU_AVG_ALL(long long, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpF:
		{
			DPU_AVG_ALL(float, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpD:
		{
			DPU_AVG_ALL(double, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpUC:
		{
			DPU_AVG_ALL(unsigned char, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpUS:
		{
			DPU_AVG_ALL(unsigned short, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpUI:
		{
			DPU_AVG_ALL(unsigned int, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		case dpUL:
		{
			DPU_AVG_ALL(unsigned long long, nmemb, denom, pAvgResult, pSrc);
			break;
		}
		default:
			printf("Do nothing!!!!!\n");
			break;
	}
#undef DPU_AVG_ALL
}

void dpuAvgCopy(int tID, int nmemb, void *pTrg, double *pAvgResult)
{
#define DPU_AVG_COPY(_type, _nmemb, _trg, _avgResult)					\
	do {																												\
		int i;																										\
		_type *pT = (_type *)(_trg);															\
																															\
		for (i=0; i<nmemb; i++) {																	\
			pT[i] = (_type) _avgResult[i];													\
		}																													\
	} while (0)

	switch (tID) {
		case dpC:
		{
			DPU_AVG_COPY(char, nmemb, pTrg, pAvgResult);
			break;
		}
		case dpS:
		{
			DPU_AVG_COPY(short, nmemb, pTrg, pAvgResult);
			break;
		}
		case dpI:
		{
			DPU_AVG_COPY(int, nmemb, pTrg, pAvgResult);
			break;
		}
		case dpL:
		{
			DPU_AVG_COPY(long long, nmemb, pTrg, pAvgResult);
			break;
		}
		case dpF:
		{
			DPU_AVG_COPY(float, nmemb, pTrg, pAvgResult);
			break;
		}
		case dpD:
		{
			DPU_AVG_COPY(double, nmemb, pTrg, pAvgResult);
			break;
		}
		case dpUC:
		{
			DPU_AVG_COPY(unsigned char, nmemb, pTrg, pAvgResult);
			break;
		}
		case dpUS:
		{
			DPU_AVG_COPY(unsigned short, nmemb, pTrg, pAvgResult);
			break;
		}
		case dpUI:
		{
			DPU_AVG_COPY(unsigned int, nmemb, pTrg, pAvgResult);
			break;
		}
		case dpUL:
		{
			DPU_AVG_COPY(unsigned long long, nmemb, pTrg, pAvgResult);
			break;
		}
		default:
			printf("Do nothing!!!!!\n");
			break;
	}
#undef DPU_AVG_COPY
}

void dpuAvg(int aggIdx, void *trg)
{
	DPResultTab *pResultTab = &gDPContext.resultTab;
	DPAggInfo *pAggInfo = gDPContext.pSharedTab->aggInfo;

	double *pAvgResult;
	int i;
	int resultNum;
	TypeID tID;
	int nmemb;

	resultNum =	pResultTab->resultTailIdx - pResultTab->resultHeadIdx;

	tID = pAggInfo[aggIdx].tID;
	nmemb = pAggInfo[aggIdx].nmemb;

	pAvgResult = (double *) malloc(sizeof(double) * nmemb);
	memset(pAvgResult, 0, sizeof(double) * nmemb);

	for (i=pResultTab->resultHeadIdx; i<pResultTab->resultTailIdx; i++) {
			void *pResult = dpuReadResultById(i, aggIdx);
			dpuAvgAll(tID, nmemb, resultNum, pAvgResult, pResult);
	}

	// Write result back
	dpuAvgCopy(tID, nmemb, trg, pAvgResult);

	free(pAvgResult);
}

// Majority vote
void dpuMjVote(int tID, int nmemb, int *pCnt, void *pMjVal, void *pSrc)
{
#define MJ_VOTE(_type, _nmemb, _cnt, _mjVal, _src)			\
	do {																									\
		int i;																							\
		_type *pS = (_type *)(_src);												\
		_type *pMjV = (_type *)(_mjVal);										\
																												\
		for (i=0; i<_nmemb; i++) {													\
			if (_cnt[i] == 0) {																\
				_cnt[i]++;																			\
				pMjV[i] = pS[i];																\
			} else if (pMjV[i] == pS[i]) {										\
				_cnt[i]++;																			\
			} else {																					\
				_cnt[i]--;																			\
			}																									\
		}																										\
	} while (0)

	switch (tID) {
		case dpC:
		{
			MJ_VOTE(char, nmemb, pCnt, pMjVal, pSrc);
			break;
		}
		case dpS:
		{
			MJ_VOTE(short, nmemb, pCnt, pMjVal, pSrc);
			break;
		}
		case dpI:
		{
			MJ_VOTE(int, nmemb, pCnt, pMjVal, pSrc);
			break;
		}
		case dpL:
		{
			MJ_VOTE(long long, nmemb, pCnt, pMjVal, pSrc);
			break;
		}
		case dpUC:
		{
			MJ_VOTE(unsigned char, nmemb, pCnt, pMjVal, pSrc);
			break;
		}
		case dpUS:
		{
			MJ_VOTE(unsigned short, nmemb, pCnt, pMjVal, pSrc);
			break;
		}
		case dpUI:
		{
			MJ_VOTE(unsigned int, nmemb, pCnt, pMjVal, pSrc);
			break;
		}
		case dpUL:
		{
			MJ_VOTE(unsigned long long, nmemb, pCnt, pMjVal, pSrc);
			break;
		}
		case dpF:
		{
			MJ_VOTE(float, nmemb, pCnt, pMjVal, pSrc);
			break;
		}
		case dpD:
		{
			MJ_VOTE(double, nmemb, pCnt, pMjVal, pSrc);
			break;
		}
		default:
			break;
	}
#undef MJ_VOTE
}

int dpuReadResultParms(int resultIdx)
{
	DPResultTab *pResultTab = &gDPContext.resultTab;

	if (resultIdx > gDPContext.resultTab.resultTailIdx) {
		printf("Error<dpuReadResultParms>: resultIdx=%d exceeds done results=%d\n",
			resultIdx, gDPContext.resultTab.resultTailIdx);
		return -1;
	}

	if (pResultTab->result[resultIdx].isParmRead) {
		return gDPContext.pSharedTab->rRandVarNum;
	} else {
		int cc;

		cc = sioReadParm((DPVar *)&pResultTab->result[resultIdx].randVar,
					getpid(),
					pResultTab->result[resultIdx].pid);

		// Parameter number mismatch. It is a fatal error
		if (cc != gDPContext.pSharedTab->rRandVarNum) {
			printf("Error<dpuReadResultParms>: Parameter number mismatch\n"
				"expected num=%d, real num=%d\n",
				gDPContext.pSharedTab->rRandVarNum, cc);
			dpExit(-1);
		}

		pResultTab->result[resultIdx].isParmRead = 1;

		return cc;
	}
}

void dpuDumpParms(int resultIdx, FILE *file)
{
	int cc;
	
	cc = dpuReadResultParms(resultIdx);

	if (cc < 0) {
		printf("Error<dpuDumpParms>: Cannot dump parameters of result=%d\n",
			resultIdx);
		return;
	}

	int i;
	DPVar *pRandVar = 
		(DPVar *)&gDPContext.resultTab.result[resultIdx].randVar;

	for (i=0; i<cc; i++) {
		fprintf(stderr, "Parameter index=%d, ", i);
		switch (pRandVar[i].tID) {
			case dpI:
				fprintf(file, "Type=Integer, Value=%d\n", pRandVar[i].intVal);
				break;
			case dpF:
				fprintf(file, "Type=Float,   Value=%f\n", pRandVar[i].fltVal);
				break;
			case dpD:
				fprintf(file, "Type=Double,  Value=%lf\n", pRandVar[i].dblVal);
				break;
			default:
				printf("Error<dpuDumpParms>: Impossible to be here\n");
				dpExit(-1);
				break;
		}
	}
}

int dpuGetParms(int resultIdx, DPVar *pRandVar)
{
	return 0;
}

// Internal function which takes va_list as argument
static int dpuSetParmLo2Result1(int resultIdx, int parmNum, va_list args)
{
	int cc;
	DPResult *pDPResult;

	if (resultIdx > gDPContext.resultTab.resultTailIdx) {
		printf("Error<dpuSetParmLo2Result1>: "
			"resultIdx=%d exceeds done results=%d\n",
			resultIdx, gDPContext.resultTab.resultTailIdx);
		return -1;
	}

	cc = dpuReadResultParms(resultIdx);
	
	if (cc < 0) {
		printf("Error<dpuSetParmLo2Result1>: Cannot set default lo to result=%d\n",
			resultIdx);
		return -1;
	}

	pDPResult = &gDPContext.resultTab.result[resultIdx];

	// Use the whole parm lo from specified resultIdx
	if (parmNum == 0) {
		memcpy(&gDPContext.pSharedTab->rLoD, &pDPResult->randVar, 
			sizeof(DPVar) * gDPContext.pSharedTab->rRandVarNum);
	} else {
		int i;

		for (i=0; i<parmNum; i++) {
			if (i < gDPContext.pSharedTab->rRandVarNum) {
				int trgParmIdx;
				
				trgParmIdx = va_arg(args, int);

				gDPContext.pSharedTab->rLoD[trgParmIdx].tID =
					pDPResult->randVar[trgParmIdx].tID;
				
				switch (pDPResult->randVar[trgParmIdx].tID) {
					case dpI:
						gDPContext.pSharedTab->rLoD[trgParmIdx].intVal = 
							pDPResult->randVar[trgParmIdx].intVal;
						break;
					case dpF:
						gDPContext.pSharedTab->rLoD[trgParmIdx].fltVal = 
							pDPResult->randVar[trgParmIdx].fltVal;
						break;
					case dpD:
						gDPContext.pSharedTab->rLoD[trgParmIdx].dblVal = 
							pDPResult->randVar[trgParmIdx].dblVal;
						break;
					default:
						printf("Error<dpuSetParmLo2Result1>: Impossible to be here\n");
						dpExit(-1);
						break;
				}
			}
		}
	}

	return 0;
}

int dpuSetParmLo2Result(int resultIdx, int parmNum, ...)
{
	va_list vl;
	int cc;
	
	va_start(vl, parmNum);
	
	cc = dpuSetParmLo2Result1(resultIdx, parmNum, vl);
	
	if (cc < 0) 
		printf("Error<dpuSetParmLo2Result>: Cannot set default lo to result=%d\n",
			resultIdx);

	va_end(vl);

	return cc;
}

static int dpuSetParmHi2Result1(int resultIdx, int parmNum, va_list vl)
{
	int cc;
	DPResult *pDPResult;

	if (resultIdx > gDPContext.resultTab.resultTailIdx) {
		printf("Error<dpuSetParmHi2Result1>: "
			"resultIdx=%d exceeds done results=%d\n",
			resultIdx, gDPContext.resultTab.resultTailIdx);
		return -1;
	}

	cc = dpuReadResultParms(resultIdx);
	
	if (cc < 0) {
		printf("Error<dpuSetParmHi2Result1>: Cannot set default hi to result=%d\n",
			resultIdx);
		return -1;
	}
	
	pDPResult = &gDPContext.resultTab.result[resultIdx];

	// Use the whole parm lo from specified resultIdx
	if (parmNum == 0) {
		memcpy(&gDPContext.pSharedTab->rLoD, &pDPResult->randVar, 
			sizeof(DPVar) * gDPContext.pSharedTab->rRandVarNum);
	} else {
		int i;

		for (i=0; i<parmNum; i++) {
			if (i < gDPContext.pSharedTab->rRandVarNum) {
				int trgParmIdx;
				
				trgParmIdx = va_arg(vl, int);
				
				gDPContext.pSharedTab->rHiD[trgParmIdx].tID =
					pDPResult->randVar[trgParmIdx].tID;
				
				switch (pDPResult->randVar[trgParmIdx].tID) {
					case dpI:
						gDPContext.pSharedTab->rHiD[trgParmIdx].intVal = 
							pDPResult->randVar[trgParmIdx].intVal;
						break;
					case dpF:
						gDPContext.pSharedTab->rHiD[trgParmIdx].fltVal = 
							pDPResult->randVar[trgParmIdx].fltVal;
						break;
					case dpD:
						gDPContext.pSharedTab->rHiD[trgParmIdx].dblVal = 
							pDPResult->randVar[trgParmIdx].dblVal;
						break;
					default:
						printf("Error<dpuSetParmHi2Result1>: Impossible to be here\n");
						dpExit(-1);
						break;
				}
			}
		}
	}

	return 0;
}

int dpuSetParmHi2Result(int resultIdx, int parmNum, ...)
{
	va_list vl;
	int cc;
	
	va_start(vl, parmNum);
	
	cc = dpuSetParmHi2Result1(resultIdx, parmNum, vl);
	
	if (cc < 0) 
		printf("Error<dpuSetParmHi2Result>: Cannot set default hi to result=%d\n",
			resultIdx);

	va_end(vl);

	return cc;
}

int dpuSetParm2Result(int resultIdx, int parmNum, ...)
{
	va_list vl;
	int cc;

	va_start(vl, parmNum);
	
	cc = dpuSetParmLo2Result1(resultIdx, parmNum, vl);

	va_end(vl);
	
	if (cc < 0)
		goto fail_bail;

	va_start(vl, parmNum);
	
	cc = dpuSetParmHi2Result1(resultIdx, parmNum, vl);
	
	va_end(vl);

	if (cc < 0)
		goto fail_bail;

	return 0;

fail_bail:
	printf("Error<dpuSetParm2Result>: Cannot set parm to result=%d\n",
		resultIdx);

	return -1;
}

static int dpuSetParmLo1(int parmNum, va_list vl)
{
	// Use the whole parm lo from specified resultIdx
	if (parmNum == 0) {
		return 0;
	} else {
		int i;

		for (i=0; i<parmNum; i++) {
			int trgParmIdx;

			trgParmIdx = va_arg(vl, int);

			// Warning: 
			// User wants to set the parameter before we can get parameters'
			// information. Which means they know the types of parameters
			// exactly, and parameters type is in parameter list. 
			if (!gDPContext.pSharedTab->rHiS[trgParmIdx].tID) {
				int tID = va_arg(vl, int);

				gDPContext.pSharedTab->rLoD[trgParmIdx].tID = tID;
			} else { 
				gDPContext.pSharedTab->rLoD[trgParmIdx].tID =
					gDPContext.pSharedTab->rLoS[trgParmIdx].tID;
			}

			switch (gDPContext.pSharedTab->rLoD[trgParmIdx].tID) {
				case dpI:
				{
					gDPContext.pSharedTab->rLoD[trgParmIdx].intVal = va_arg(vl, int);
					break;
				}
				case dpF:
				{
					gDPContext.pSharedTab->rLoD[trgParmIdx].fltVal = va_arg(vl, double);
					break;
				}
				case dpD:
				{
					gDPContext.pSharedTab->rLoD[trgParmIdx].dblVal = va_arg(vl, double);
					break;
				}
				default:
					printf("Error<dpuSetParmLo1>: Impossible to be here\n");
					dpExit(-1);
					break;
			}
		}
	}

	return 0;
}

int dpuSetParmLo(int parmNum, ...)
{
	va_list vl;
	int cc;
	
	va_start(vl, parmNum);
	
	cc = dpuSetParmLo1(parmNum, vl);
	
	if (cc < 0) 
		printf("Error<dpuSetParmLo>: Cannot set default lo\n");
	
	va_end(vl);

	return cc;
}

static int dpuSetParmHi1(int parmNum, va_list vl)
{
	// Use the whole parm lo from specified resultIdx
	if (parmNum == 0) {
		return 0;
	} else {
		int i;

		for (i=0; i<parmNum; i++) {
			int trgParmIdx;

			trgParmIdx = va_arg(vl, int);

			// Warning: 
			// User wants to set the parameter before we can get parameters'
			// information. Which means they know the types of parameters
			// exactly, and parameters type is in parameter list. 
			if (!gDPContext.pSharedTab->rHiS[trgParmIdx].tID) {
				int tID = va_arg(vl, int);

				gDPContext.pSharedTab->rHiD[trgParmIdx].tID = tID;
			} else { 
				gDPContext.pSharedTab->rHiD[trgParmIdx].tID =
					gDPContext.pSharedTab->rHiS[trgParmIdx].tID;
			}

			switch (gDPContext.pSharedTab->rHiD[trgParmIdx].tID) {
				case dpI:
				{
					gDPContext.pSharedTab->rHiD[trgParmIdx].intVal = va_arg(vl, int);
					break;
				}
				case dpF:
				{
					gDPContext.pSharedTab->rHiD[trgParmIdx].fltVal = va_arg(vl, double);
					break;
				}
				case dpD:
				{
					gDPContext.pSharedTab->rHiD[trgParmIdx].dblVal = va_arg(vl, double);
					break;
				}
				default:
					printf("Error<dpuSetParmHi1>: Impossible to be here\n");
					dpExit(-1);
					break;
			}
		}
	}

	return 0;
}
int dpuSetParmHi(int parmNum, ...)
{
	va_list vl;
	int cc;
	
	va_start(vl, parmNum);
	
	cc = dpuSetParmHi1(parmNum, vl);
	
	if (cc < 0) 
		printf("Error<dpuSetParmHi>: Cannot set default hi\n");
	
	va_end(vl);

	return cc;
}

int dpuSetParm(int parmNum, ...)
{
	va_list vl;
	int cc;

	va_start(vl, parmNum);
	
	cc = dpuSetParmLo1(parmNum, vl);

	va_end(vl);
	
	if (cc < 0)
		goto fail_bail;

	va_start(vl, parmNum);
	
	cc = dpuSetParmHi1(parmNum, vl);
	
	va_end(vl);

	if (cc < 0)
		goto fail_bail;

	return 0;

fail_bail:
	printf("Error<dpuSetParm>: Cannot set parm value\n");

	return -1;
}

int dpuSetParmLoWithType(int parmNum, ...)
{
	va_list vl;
	int cc;
	
	va_start(vl, parmNum);
	
	cc = dpuSetParmLo1(parmNum, vl);
	
	if (cc < 0) 
		printf("Error<dpuSetParmLoWithType>: Cannot set default lo\n");
	
	va_end(vl);

	return cc;
}

int dpuSetParmHiWithType(int parmNum, ...)
{
	va_list vl;
	int cc;
	
	va_start(vl, parmNum);
	
	cc = dpuSetParmHi1(parmNum, vl);
	
	if (cc < 0) 
		printf("Error<dpuSetParmHiWithType>: Cannot set default hi\n");
	
	va_end(vl);

	return cc;
}

int dpuRandSetParmWithProb(int parmNum, ...)
{
	int i;
	va_list vl;

	va_start(vl, parmNum);
	
	for (i=0; i<parmNum; i++) {
		double prob;
		int parmIdx;
		
		parmIdx = va_arg(vl, int);
		prob = va_arg(vl, double);
	
		if (prob > dpRSampleReal(0.0, 1.0)) {
			switch (gDPContext.pSharedTab->rLoS[parmIdx].tID) {
				case dpI:
				{
					int randNum = 
						dpRSampleInt(gDPContext.pSharedTab->rLoS[parmIdx].intVal,
							gDPContext.pSharedTab->rHiS[parmIdx].intVal);

					if (dpuSetParm(1, parmIdx, randNum) < 0) {
						printf("Error<dpuRandSetParmWithProb>: "
							"Cannot set %dth parm with prob %lf\n",
							parmIdx, prob);
					}

					break;
				}
				case dpF:
				{
					double randNum = 
						dpRSampleReal(gDPContext.pSharedTab->rLoS[parmIdx].fltVal,
							gDPContext.pSharedTab->rHiS[parmIdx].fltVal);

					if (dpuSetParm(1, parmIdx, randNum) < 0) {
						printf("Error<dpuRandSetParmWithProb>: "
							"Cannot set %dth parm with prob %lf\n",
							parmIdx, prob);
					}
					break;
				}
				case dpD:
				{
					double randNum = 
						dpRSampleReal(gDPContext.pSharedTab->rLoS[parmIdx].dblVal,
							gDPContext.pSharedTab->rHiS[parmIdx].dblVal);

					if (dpuSetParm(1, parmIdx, randNum) < 0) {
						printf("Error<dpuRandSetParmWithProb>: "
							"Cannot set %dth parm with prob %lf\n",
							parmIdx, prob);
					}
					break;
				}
				default:
					printf("Error<dpuSetParmHi1>: Impossible to be here\n");
					dpExit(-1);
					break;
			}
		}
	}
	
	va_end(vl);

	return 0;
}

void dpuResetParm()
{

}

// Only read the result in the data table without returning
// the data
int dpuReadResultBin(int resultIdx)
{
	DPResultTab *pResultTab = &gDPContext.resultTab;
	DPAggInfo *pAggInfo = gDPContext.pSharedTab->aggInfo;

	if (resultIdx > gDPContext.resultTab.resultTailIdx) {
		printf("Error<dpuReadResultBin>: resultIdx=%d exceeds done results=%d\n",
			resultIdx, gDPContext.resultTab.resultTailIdx);
		return -1;
	}

	if (pResultTab->result[resultIdx].pData[0]) {
		return 0;
	} else {
		char *pResult = NULL;
		int i;
		int aggNum = gDPContext.pSharedTab->aggNum;

		pResult = 
			sioReadResult(getpid(),	pResultTab->result[resultIdx].pid);

		if (pResult == NULL)
			return -1;
	
		for (i=0; i<aggNum; i++) {
			pResultTab->result[resultIdx].pData[i] = pResult;
			pResult += pAggInfo[i].nmemb * pAggInfo[i].tSize;
		}

		// Check for cross validation
		if (pAggInfo[aggNum].trg == &gDPContext.childTab.cvValidError) {
			pResultTab->result[resultIdx].pData[i] = pResult;
		}
	}

	return 0;
}

// If result has been read, return the data directly. Otherwise,
// read it first and return the data.
void *dpuReadResultById(int resultIdx, int aggIdx)
{
	int cc;
	DPAggInfo *pAggInfo = &gDPContext.pSharedTab->aggInfo[aggIdx];

	// Sampling result is stored in file
	if (pAggInfo->stg < A_AVG_MEM) {
		cc = dpuReadResultBin(resultIdx);

		if (cc < 0)
			return NULL;

		return gDPContext.resultTab.result[resultIdx].pData[aggIdx];
	}
	// Sampleing result is stored in shared memory
	else {
		int shmIdx = gDPContext.resultTab.result[resultIdx].shmIdx;

		return pAggInfo->shmResult[shmIdx];
	} 
}

// If result has been read, return the data directly. Otherwise,
// read it first and return the data.
void *dpuReadResult(int resultIdx, void *trg)
{
	int cc;
	int i;
	DPResultTab *pResultTab = &gDPContext.resultTab;
	DPAggInfo *pAggInfo = gDPContext.pSharedTab->aggInfo;
	int aggNum = gDPContext.pSharedTab->aggNum;

	for (i=0; i<aggNum; i++) {
		if (pAggInfo[i].trg == trg) {
			// Sampleing result is stored in file 
			if (pAggInfo[i].stg < A_AVG_MEM) {
				cc = dpuReadResultBin(resultIdx);

				if (cc < 0)
					return NULL;
				
				return pResultTab->result[resultIdx].pData[i];
			} 
			// Sampleing result is stored in shared memory
			else {
				return pAggInfo[i].shmResult[pResultTab->result[resultIdx].shmIdx];
			}
		}
	}
	
	return NULL;
}

void *dpuReadSync(int syncIdx)
{
	return gDPContext.pSharedTab->sySyncData[syncIdx];
}

int dpuGetNmemb(int resultIdx, int aggIdx)
{
	int *pNmemb = NULL;

	if (!gDPContext.resultTab.result[resultIdx].pData[0] && 
			dpuReadResultBin(resultIdx) < 0)
	{
		return 0;
	}

	pNmemb = gDPContext.resultTab.result[resultIdx].pData[aggIdx];

	return *(pNmemb - 1);
}

int dpuGetResultNum()
{
	return gDPContext.resultTab.resultTailIdx;
}


// In split sync, instead of forking directly, 
// we need to notify the target process by 
// creating a pid.sync.fork file.
void dpuSplitSync(int resultIdx)
{
	if (sioSplitSyncInit(resultIdx, getppid()) < 0) {
		printf("Error<dpuSplitSync>: Split for synchronization failed\n");
	}
}

void dpuFreeResult(int resultIdx)
{
	// Minus 4 to get the base address of the allocated memory
	if (gDPContext.resultTab.result[resultIdx].pData[0]) {
		free(gDPContext.resultTab.result[resultIdx].pData[0]);
		gDPContext.resultTab.result[resultIdx].pData[0] = NULL;
	}
}

void dpuFreeResultAll()
{
	int i;
	DPResultTab *pResultTab = &gDPContext.resultTab;

	for (i=0; i<gDPContext.resultTab.resultTailIdx; i++) {
		if (pResultTab->result[i].pData[0]) {
			// Minus 4 to get the base address of the allocated memory
			free(gDPContext.resultTab.result[i].pData[0]);
			pResultTab->result[i].pData[0] = NULL;
		}
		pResultTab->result[i].isParmRead = 0;
	}
}

void dpuRmResult(int resultIdx)
{
	dpuFreeResult(resultIdx);
	if (sioRmResult(getpid(), gDPContext.resultTab.result[resultIdx].pid) < 0) {
		printf("Error<dpuRmResult>: Failed to remove result with"
			"ppid=%d pid=%d errno=%d\n",
			getpid(), gDPContext.resultTab.result[resultIdx].pid, errno);
		dpExit(-1);
	}
}

double dpuReadCVResult(int resultIdx)
{
	void *pResult = 
		dpuReadResultById(resultIdx, gDPContext.pSharedTab->aggNum);

	if (!pResult) {
		printf("Error<dpuReadCVResult>: Failed to cross validation result "
			"ppid=%d pid=%d resultIdx=%d\n",
			getpid(), gDPContext.resultTab.result[resultIdx].pid, resultIdx);
		dpExit(-1);
	}

	return *((double *)pResult);
}
