#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <string.h>
#include "SamplingStg.h"
#include "DpLib.h"
#include "DpUserApi.h"
#include "SamplingIO.h"
#include "AggregateStg.h"
#include "Sched.h"

// At most 32 child processes will be forked one time,
// then parent process need to wait for them to exit
// and fork again.
#define MAX_CHD_PER_RUN		32

#define MAX_FAIL_NUM	256

// The sampling strategy implementations. Default strategy
// starts from 1 instead of 0. Strategy 0 will not be used 
// or called by the user. It is only used to occpupy the 
// 0th element if sampling strategy function table.

extern DPContext gDPContext;

// No implementation for strategy 0 
static void sstgStg0(int parmNum, ...) 
{
	int i;
	va_list vl;
	DPResultTab *pResultTab = &gDPContext.resultTab;
	int (*stgFunc)(int);
	void (*stgInitFunc)(void);
	void (*stgFiniFunc)(void);
	int subTodoNum = pResultTab->subTodoNum;
	int failedNum = 0;

	va_start(vl, parmNum);
	
	stgInitFunc = va_arg(vl, void *);
	stgFiniFunc = va_arg(vl, void *);
	stgFunc = va_arg(vl, void *);
	
	va_end(vl);

	stgInitFunc();
	do {
		// We don;t have to use mutex here because only parent process
		// will be here.
		gDPContext.pSharedTab->syActiveChildNum = 
			(pResultTab->todoNum < subTodoNum) ?
			pResultTab->todoNum : subTodoNum;
		
		// Fork child according to sampling threshold
		// To prevent too many forks, break out the loop per subTodoNum
		// forks
		for (i=0; i<pResultTab->todoNum && i<subTodoNum; i++) {
			if (fork() == 0) {
				gDPContext.execStatus = 0;
				pResultTab->todoNum = 0;
				return;
			}
		}

		printf("#todo=%d #done=%d\n", 
			pResultTab->todoNum, pResultTab->resultTailIdx);
		fflush(stdout);
		// Wait for all child process to exit
		while (1) {
			int status;
			pid_t donePID = wait(&status);

			if (donePID > 0) {
				// Child exit normally
				assert(WIFEXITED(status));

				// Child exit status:
				switch(WEXITSTATUS(status)) {
					case 0: // 0: Nnormal exit
					{
						int cc;

						pResultTab->result[pResultTab->resultTailIdx].pid = donePID;
						cc = stgFunc(pResultTab->resultTailIdx);

						// Free the sampling result if user don't want it
						if (!cc) {
							if (pResultTab->result[pResultTab->resultTailIdx].pData[0])
								dpuFreeResult(pResultTab->resultTailIdx);
							pResultTab->result[pResultTab->resultTailIdx].isParmRead = 0;
						} else {
							pResultTab->todoNum--;
							pResultTab->resultTailIdx++;
						}
						break;
					}
					case 1: // 1: Check failed
						failedNum++;
						// If the failed number reachs a threshold, minus
						// the to-do number by 1 and reset failedNum
						if (failedNum > MAX_FAIL_NUM) {
							failedNum = 0;
							pResultTab->todoNum--;
						}
						break;
					case 2: // 2: Forked parent process exit
						break;
					case SHM_EXIT_STATUS_BASE ... 
							(SHM_EXIT_STATUS_BASE + MAX_SAMPLING_SUB_TODO):
					{
						int shmIdx = WEXITSTATUS(status) - SHM_EXIT_STATUS_BASE;
		
						astgAggShmConfig(0, shmIdx);

						// Store the index to the result and set the shm as ready
						ASTG_SHM_SET_READY(shmIdx, pResultTab->resultTailIdx);
						
						pResultTab->result[pResultTab->resultTailIdx].pid = donePID;
						pResultTab->result[pResultTab->resultTailIdx].shmIdx = shmIdx;
						pResultTab->todoNum--;
						pResultTab->resultTailIdx++;
						break;
					}
					default:
						printf("Fatal failed: Child %d exit abnormally, status=%d.\n"
							"Data processing stopped\n", donePID, WEXITSTATUS(status));
						dpExit(-1);
				}
			} else if (donePID == -1 && errno == ECHILD) {
#ifdef DEBUG
				printf("No more child, exit loop\n");
#endif
				// Do incremental aggregation
				astgAggShm();

				// Finalize
				if (pResultTab->todoNum == 0) {
					astgAggShmConfig(1, -1);
					stgFiniFunc();
				}
				break;
			} else {
				assert(0);
			}
		}
	} while (pResultTab->todoNum);
}

// Random sampling
static void sstgStg1(int parmNum, ...)
{
	int i;
	DPResultTab *pResultTab = &gDPContext.resultTab;
	int subTodoNum = pResultTab->subTodoNum;
	int failedNum = 0;

	do {
		// We don;t have to use mutex here because only parent process
		// will be here.
		gDPContext.pSharedTab->syActiveChildNum = 
			(pResultTab->todoNum < subTodoNum) ?
			pResultTab->todoNum : subTodoNum;

		// Fork child according to sampling threshold
		// To prevent too many forks, break out the loop per MAX_CHD_PER_RUN 
		// forks
		for (i=0; i<pResultTab->todoNum && i<subTodoNum; i++) {
			schSched(S_SPAWN_SAMPLING, pResultTab->todoNum);
			if (fork() == 0) {
				gDPContext.execStatus = 0;
				pResultTab->todoNum = 0;
				return;
			}
		}

		printf("#TODO=%d\n", pResultTab->todoNum);
		fflush(stdout);
		// Wait for all child process to exit
		while (1) {
			int status;
			pid_t donePID = wait(&status);

			if (donePID > 0) {
				assert(WIFEXITED(status));

				// Child exit status:
				switch(WEXITSTATUS(status)) {
					case 0: // 0: Nnormal exit
						pResultTab->result[pResultTab->resultTailIdx].pid = donePID;
						pResultTab->todoNum--;
						pResultTab->resultTailIdx++;
						break;
					case 1: // 1: Check failed
						failedNum++;
						// If the failed number reachs a threshold, minus
						// the to-do number by 1 and reset failedNum
						if (failedNum > MAX_FAIL_NUM) {
							failedNum = 0;
							pResultTab->todoNum--;
						}
						break;
					case 2: // 2: Forked parent process exit
						break;
					case SHM_EXIT_STATUS_BASE ... 
							(SHM_EXIT_STATUS_BASE + MAX_SAMPLING_SUB_TODO):
					{
						int shmIdx = WEXITSTATUS(status) - SHM_EXIT_STATUS_BASE;
		
						astgAggShmConfig(0, shmIdx);

						// Store the index to the result and set the shm as ready
						ASTG_SHM_SET_READY(shmIdx, pResultTab->resultTailIdx);
						
						pResultTab->result[pResultTab->resultTailIdx].pid = donePID;
						pResultTab->result[pResultTab->resultTailIdx].shmIdx = shmIdx;
						pResultTab->todoNum--;
						pResultTab->resultTailIdx++;
						break;
					}
					default:
						printf("Fatal failed: Child %d exit abnormally, status=%d.\n"
							"Data processing stopped\n", donePID, WEXITSTATUS(status));
						dpExit(-1);
				}
			} else if (donePID == -1 && errno == ECHILD) {
#ifdef DEBUG
				printf("No more child, exit loop\n");
#endif
				// Do incremental aggregation
				astgAggShm();

				// Finalize
				if (pResultTab->todoNum == 0) {
					astgAggShmConfig(1, -1);
				}
				break;
			} else {
				assert(0);
			}
		}
	} while (pResultTab->todoNum);
}

// MCMC sampling for max
static void sstgStg2(int parmNum, ...)
{
	int i;
	va_list vl;
	DPResultTab *pResultTab = &gDPContext.resultTab;
	double (*scoreFunc)(int);
	void (*scoreInitFunc)(void);
	void (*scoreFiniFunc)(void);
	double (*acceptProbFunc)(double, double);
	double oldScore = 0;
	int subTodoNum = pResultTab->subTodoNum;

	va_start(vl, parmNum);
	
	scoreInitFunc = va_arg(vl, void *);
	scoreFiniFunc = va_arg(vl, void *);
	scoreFunc = va_arg(vl, void *);
	acceptProbFunc = va_arg(vl, void *);

	va_end(vl);
	
	if (scoreInitFunc)
		scoreInitFunc();
	
	do {
		// We don;t have to use mutex here because only parent process
		// will be here.
		gDPContext.pSharedTab->syActiveChildNum = 
			(pResultTab->todoNum < subTodoNum) ?
			pResultTab->todoNum : subTodoNum;
		
		// Fork child according to sampling threshold
		// To prevent too many forks, break out the loop per 16
		// forks
		for (i=0; i<pResultTab->todoNum && i<subTodoNum; i++) {
			schSched(S_SPAWN_SAMPLING, pResultTab->todoNum);
			if (fork() == 0) {
				gDPContext.execStatus = 0;
				pResultTab->todoNum = 0;
				break;
			}
		}

		if (gDPContext.execStatus) {
			printf("#TODO=%d #DONE=%d\n", 
				pResultTab->todoNum, pResultTab->resultTailIdx);
			fflush(stdout);
			// Wait for all child process to exit
			while (1) {
				int status;
				pid_t donePID = wait(&status);

				if (donePID > 0) {
					assert(WIFEXITED(status));
				
					switch (WEXITSTATUS(status)) {
						case 0:
						{
							double newScore;
							double prob = dpRSampleReal(0, 1);
							
							pResultTab->result[pResultTab->resultTailIdx].pid = donePID;
							newScore = scoreFunc(pResultTab->resultTailIdx);

							// Accept if:
							// 1. First sample result or
							// 2. Acceptance probility > random probility
							if (pResultTab->resultTailIdx == 0 || 
									acceptProbFunc(oldScore, newScore) > prob) 
							{
								if (pResultTab->resultTailIdx == 0 || 
									 newScore > pResultTab->bestResultScore) 
								{
									pResultTab->bestResultIdx = pResultTab->resultTailIdx;
									pResultTab->bestResultScore = newScore;
								}

								oldScore = newScore;
								pResultTab->todoNum--;
								pResultTab->resultTailIdx++;
							} else {
								// Reject: Free the memory if the result is read
								if (pResultTab->result[pResultTab->resultTailIdx].pData[0])
									dpuFreeResult(pResultTab->resultTailIdx);
								
								pResultTab->result[pResultTab->resultTailIdx].isParmRead = 0;
							}
							break;
						}
						case 1:
						case 2:
							break;
						case SHM_EXIT_STATUS_BASE ... 
								(SHM_EXIT_STATUS_BASE + MAX_SAMPLING_SUB_TODO):
						{
							double newScore;
							double prob = dpRSampleReal(0, 1);
							int shmIdx = WEXITSTATUS(status) - SHM_EXIT_STATUS_BASE;
			
							astgAggShmConfig(0, shmIdx);

							pResultTab->result[pResultTab->resultTailIdx].pid = donePID;
							pResultTab->result[pResultTab->resultTailIdx].shmIdx = shmIdx;
							
							// Execute the score function to get the score back
							if (scoreInitFunc && scoreFiniFunc) {
								newScore = scoreFunc(pResultTab->resultTailIdx);
							} 
							// Otherwise, we get the score computed by the child process
							// from shm
							else {
								// ScoreFunc will be the address of target variable for
								// searching agg variable's index
								newScore = astgGetShmScore(shmIdx, (void *)scoreFunc);
							}
						
							// Store the index to the result and set the shm as ready
							ASTG_SHM_SET_READY(shmIdx, pResultTab->resultTailIdx);
						
							// Accept if:
							// 1. First sample result or
							// 2. Acceptance probility > random probility
							if (pResultTab->resultTailIdx == 0 || 
									acceptProbFunc(oldScore, newScore) > prob) 
							{
								oldScore = newScore;
								pResultTab->todoNum--;
								pResultTab->resultTailIdx++;
							} else {
								// Reject: Set target shm not ready
								pResultTab->result[pResultTab->resultTailIdx].isParmRead = 0;
								ASTG_SHM_SET_UNREADY(shmIdx);
							}
							break;
						}
						default:
							printf("Fatal failed: Child %d exit abnormally, status=%d.\n"
								"Data processing stopped\n", donePID, WEXITSTATUS(status));
							dpExit(-1);
					}
				} else if (donePID == -1 && errno == ECHILD) {
#ifdef DEBUG
					printf("No more child, exit loop\n");
#endif
					// Do incremental aggregation
					astgAggShm();
				
					// Finalize
					if (pResultTab->todoNum == 0) {
						if (scoreFiniFunc)
							scoreFiniFunc();

						astgAggShmConfig(1, -1);
					}
					break;
				} else {
					assert(0);
				}
			} // End while
		}
	} while (pResultTab->todoNum);
	
}

// MCMC sampling for min
static void sstgStg3(int parmNum, ...)
{
	int i;
	va_list vl;
	DPResultTab *pResultTab = &gDPContext.resultTab;
	double (*scoreFunc)(int);
	void (*scoreInitFunc)(void);
	void (*scoreFiniFunc)(void);
	double (*acceptProbFunc)(double, double);
	double oldScore = 0;
	int subTodoNum = pResultTab->subTodoNum;

	va_start(vl, parmNum);
	
	scoreInitFunc = va_arg(vl, void *);
	scoreFiniFunc = va_arg(vl, void *);
	scoreFunc = va_arg(vl, void *);
	acceptProbFunc = va_arg(vl, void *);

	va_end(vl);
	
	if (scoreInitFunc)
		scoreInitFunc();
	
	do {
		// We don;t have to use mutex here because only parent process
		// will be here.
		gDPContext.pSharedTab->syActiveChildNum = 
			(pResultTab->todoNum < subTodoNum) ?
			pResultTab->todoNum : subTodoNum;
		
		// Fork child according to sampling threshold
		// To prevent too many forks, break out the loop per 16
		// forks
		for (i=0; i<pResultTab->todoNum && i<subTodoNum; i++) {
			if (fork() == 0) {
				gDPContext.execStatus = 0;
				pResultTab->todoNum = 0;
				break;
			}
		}

		if (gDPContext.execStatus) {
			printf("#TODO=%d #DONE=%d\n", 
				pResultTab->todoNum, pResultTab->resultTailIdx);
			fflush(stdout);
			// Wait for all child process to exit
			while (1) {
				int status;
				pid_t donePID = wait(&status);

				if (donePID > 0) {
					assert(WIFEXITED(status));
				
					switch (WEXITSTATUS(status)) {
						case 0:
						{
							double newScore;
							double prob = dpRSampleReal(0, 1);
							
							pResultTab->result[pResultTab->resultTailIdx].pid = donePID;
							newScore = scoreFunc(pResultTab->resultTailIdx);

							// Accept if:
							// 1. First sample result or
							// 2. Acceptance probility > random probility
							if (pResultTab->resultTailIdx == 0 || 
									acceptProbFunc(oldScore, newScore) > prob) 
							{
								if (pResultTab->resultTailIdx == 0 || 
									 newScore < pResultTab->bestResultScore) 
								{
									pResultTab->bestResultIdx = pResultTab->resultTailIdx;
									pResultTab->bestResultScore = newScore;
								}

								oldScore = newScore;
								pResultTab->todoNum--;
								pResultTab->resultTailIdx++;
							} else {
								// Reject: Free the memory if the result is read
								if (pResultTab->result[pResultTab->resultTailIdx].pData[0])
									dpuFreeResult(pResultTab->resultTailIdx);
								
								pResultTab->result[pResultTab->resultTailIdx].isParmRead = 0;
							}
							break;
						}
						case 1:
						case 2:
							break;
						case SHM_EXIT_STATUS_BASE ... 
								(SHM_EXIT_STATUS_BASE + MAX_SAMPLING_SUB_TODO):
						{
							double newScore;
							double prob = dpRSampleReal(0, 1);
							int shmIdx = WEXITSTATUS(status) - SHM_EXIT_STATUS_BASE;
			
							astgAggShmConfig(0, shmIdx);

							// Process id set to child processes' exit status
							pResultTab->result[pResultTab->resultTailIdx].pid = donePID;
							pResultTab->result[pResultTab->resultTailIdx].shmIdx = shmIdx;
							
							// Execute the score function to get the score back
							if (scoreInitFunc && scoreFiniFunc) {
								newScore = scoreFunc(pResultTab->resultTailIdx);
							} 
							// Otherwise, we get the score computed by the child process
							// from shm
							else {
								// ScoreFunc will be the address of target variable for
								// searching agg variable's index
								newScore = astgGetShmScore(shmIdx, (void *)scoreFunc);
							}
						
							// Store the index to the result and set the shm as ready
							ASTG_SHM_SET_READY(shmIdx, pResultTab->resultTailIdx);
						
							// Accept if:
							// 1. First sample result or
							// 2. Acceptance probility > random probility
							if (pResultTab->resultTailIdx == 0 || 
									acceptProbFunc(oldScore, newScore) > prob) 
							{
								oldScore = newScore;
								pResultTab->todoNum--;
								pResultTab->resultTailIdx++;
							} else {
								// Reject: Set target shm not ready
								pResultTab->result[pResultTab->resultTailIdx].isParmRead = 0;
								ASTG_SHM_SET_UNREADY(shmIdx);
							}
							break;
						}
						default:
							printf("Fatal failed: Child %d exit abnormally, status=%d.\n"
								"Data processing stopped\n", donePID, WEXITSTATUS(status));
							dpExit(-1);
					}
				} else if (donePID == -1 && errno == ECHILD) {
#ifdef DEBUG
					printf("No more child, exit loop\n");
#endif
					// Do incremental aggregation
					astgAggShm();
				
					// Finalize
					if (pResultTab->todoNum == 0) {
						if (scoreFiniFunc)
							scoreFiniFunc();

						astgAggShmConfig(1, -1);
					}
					break;
				} else {
					assert(0);
				}
			} // End while
		}
	} while (pResultTab->todoNum);
	
}


static int sstgStg4CB(int resultNum)
{
	double cvError = dpuReadCVResult(resultNum);

//	printf("validation error of resultIdx:%d = %lf\n", 
//		resultNum, cvError);

	return 1;
}

// K-fold cross validation sampling
static void sstgStg4(int parmNum, ...)
{
	int i, j;
	va_list vl;
	DPResultTab *pResultTab = &gDPContext.resultTab;
	int subTodoNum = pResultTab->subTodoNum;
	int K;
	char *pTrainName, **ppSplittedName;
	double (*validFunc)(char *, ...);
	double (*acceptProbFunc)(double, double);
	DPChildTab *pChildTab = &gDPContext.childTab;
	pthread_mutexattr_t mtxAttr;
	char *mapAddrTab[MAX_CV_SUB_TODO];
	double oldScore = 0;

	va_start(vl, parmNum);

	K = va_arg(vl, int);
	pTrainName = va_arg(vl, char *);
	validFunc = va_arg(vl, void *);
	acceptProbFunc = va_arg(vl, void *);

	va_end(vl);

	ppSplittedName = sioCVInit(pTrainName, K);
/*	
	for (i=0; i<(K * 2); i++) {
		printf("%s\n", ppSplittedName[i]);
	}
	*/
	// Initialize the shared memory region and mutex
	// These mmaped memory regions are only shared by the child processes
	// with the same Sampling ID. In order to reduce mmap overhead, we
	// mmap series of address
	pthread_mutexattr_init(&mtxAttr);
	pthread_mutexattr_setpshared(&mtxAttr, PTHREAD_PROCESS_SHARED);
	
	for (i=0; i<subTodoNum; i++) {
		mapAddrTab[i] =
			mmap(0, 
			sizeof(pthread_mutex_t) + sizeof(double) + sizeof(int) + 
				(sizeof(DPVar) * MAX_RAND_VAR),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
		
		pthread_mutex_init((pthread_mutex_t *) mapAddrTab[i], &mtxAttr);
	}

	// Store the cross validation callback and child process
	// can do it in the end
	pChildTab->cvCB = validFunc;
	// Set K
	pChildTab->K = K;

	do {
		// We don't have to use mutex here because only parent process
		// will be here.
		gDPContext.pSharedTab->syActiveChildNum = 
			(pResultTab->todoNum < subTodoNum) ?
			pResultTab->todoNum : subTodoNum;
	
		// Fork child according to sampling threshold
		// To prevent too many forks, break out the loop per 4 * K
		// forks
		for (i=0; i<pResultTab->todoNum && i<subTodoNum; i++) {
			// 1. Sharing mutex
			pChildTab->cvMtx = (pthread_mutex_t *) mapAddrTab[i];
		
			// 2. Sharing validation error
			pChildTab->cvValidError = (double *) 
				(mapAddrTab[i] + sizeof(pthread_mutex_t));
			*(pChildTab->cvValidError) = 0;

			// 3. Sharing remaining child number
			pChildTab->cvRemChildNum = (int *)
				(mapAddrTab[i] + sizeof(pthread_mutex_t) + sizeof(double));
			*(pChildTab->cvRemChildNum) = K;

			// 4. Sharing random sample variable
			pChildTab->cvRSVarTab = (DPVar *)
				(mapAddrTab[i] + 
					sizeof(pthread_mutex_t) + sizeof(double) + sizeof(int));
			memset(pChildTab->cvRSVarTab, 0, sizeof(DPVar) * MAX_RAND_VAR);

			pChildTab->cvSamplingID = pResultTab->resultTailIdx;
		
			for (j=0; j<K; j++) {
				schSched(S_SPAWN_SAMPLING, pResultTab->todoNum);
				if (fork() == 0) {
					gDPContext.execStatus = 0;
					pResultTab->todoNum = 0;
					pChildTab->cvRunning = 1;
					
					memcpy(pTrainName, ppSplittedName[j], strlen(ppSplittedName[j]) + 1);
					pChildTab->cvValidFileName = ppSplittedName[j + K];
		
					sioCVFini(ppSplittedName, j, K);
					return;
				}
			}
		}

		printf("#TODO=%d #DONE=%d\n", 
			pResultTab->todoNum, pResultTab->resultTailIdx);
		fflush(stdout);
		// Wait for all child process to exit
		while (1) {
			int status;
			pid_t donePID = wait(&status);

			if (donePID > 0) {
				// Child exit normally
				assert(WIFEXITED(status));

				// In cross validation, if check failed for any one child process
				// out of K child process, the others K - 1 child processes are
				// highly likely to fail. Thus, we only handle the sampling with
				// all child processes exit normally.
				// Child exit status:
				switch(WEXITSTATUS(status)) {
					case 0: // 0: Nnormal exit
					{
						int cc = 1;

						pResultTab->result[pResultTab->resultTailIdx].pid = donePID;

						if (acceptProbFunc) {
							double prob = dpRSampleReal(0, 1);
							double newScore = dpuReadCVResult(pResultTab->resultTailIdx);
							
							cc = (pResultTab->resultTailIdx == 0) ||
								(acceptProbFunc(oldScore, newScore) > prob);
							
							oldScore = newScore;
						} else {
							sstgStg4CB(pResultTab->resultTailIdx);
						}

						if (!cc) {
							if (pResultTab->result[pResultTab->resultTailIdx].pData[0])
								dpuFreeResult(pResultTab->resultTailIdx);
							pResultTab->result[pResultTab->resultTailIdx].isParmRead = 0;
						} else {
							pResultTab->todoNum--;
							pResultTab->resultTailIdx++;
						}
						break;
					}
					case 1: // 1: Check failed
					case 2: // 2: Forked parent process exit
					case 3: // 3: The others K - 1 child processes exit normally
						break;
					default:
						printf("Fatal failed: Child %d exit abnormally, status=%d.\n"
							"Data processing stopped\n", donePID, WEXITSTATUS(status));
						dpExit(-1);
				}
			} else if (donePID == -1 && errno == ECHILD) {
#ifdef DEBUG
				printf("No more child, exit loop\n");
#endif
				if (pResultTab->todoNum == 0) {
					pthread_mutexattr_destroy(&mtxAttr);
					sioCVFini(ppSplittedName, K, K);
					
					for (i=0; i<subTodoNum; i++) {
						pthread_mutex_destroy((pthread_mutex_t *)mapAddrTab[i]);
						
						munmap(mapAddrTab[i], 
							sizeof(pthread_mutex_t) + sizeof(double) + sizeof(int) + 
								(sizeof(DPVar) * MAX_RAND_VAR));
					}
				}

				break;
			} else {
				assert(0);
			}
		}
	} while (pResultTab->todoNum);
	
}

sstgFuncPtr sstgFuncTab[] = {
	sstgStg0,
	sstgStg1,
	sstgStg2,
	sstgStg3,
	sstgStg4,
};

