#ifndef _SAMPLING_IO_H_
#define _SAMPLING_IO_H_

#include <unistd.h>
#include "DpLib.h"

extern const char sioDir[];
extern int sioResult[];
extern int sioResultIdx;

// Initialize sampling file io. If ok, return 0, else return -1.
int sioInit();

// Finalize sampling file io.
int sioFini();

int sioSARegionInit();

int sioSARegionFini();

int sioRmResult(int ppid, int pid);

int sioSyncInit();

int sioSyncFini(pid_t ppid);

// Write a sampling result from buf to file
int sioWriteResult(void *buf, int nmemb, int size, pid_t ppid, pid_t pid);

// Write the data for synchronization from buf to file
int sioWriteSyncData(void *buf, int nmemb, int size, pid_t ppid, pid_t pid);

// Write the data from buf to file, in binary
int sioWriteBin(void *buf, int nmemb, int size, char *path);

// Write parameter value to file, in text
int sioWriteParm(char *info, pid_t ppid, pid_t pid);

// Read a sampling result from file to buf
void *sioReadResult(pid_t ppid, pid_t pid);

// Read the data for synchronization from file to buf
int sioReadSyncData(void *buf, int nmemb, int size, pid_t ppid, pid_t pid);

//
int sioReadSyncDataAll(int syncChildNum, int nmemb, int size, pid_t ppid);

// Read the data result from file to buf, in binary
int sioReadBin(void *buf, int nmemb, int size, char *path);

// Read parameter value from file, in text
int sioReadParm(DPVar *pRandVar, pid_t ppid, pid_t pid);

// Called by master process in charge of cross process synchronization
int sioSplitSyncInit(int resultIdx, pid_t ppid);
// Called by each child process in dpSyncGlue
int sioSplitSyncCheck(pid_t ppid, pid_t pid);
// Called by parent process to split the original data into K
// different parts.
char **sioCVInit(char *pTrainName, int K);
int sioCVFini(char **ppSplittedName, int idx, int K);

// Create the shared memory fd for storing sampling result
void *sioCreateShm(pid_t ppid, int aggIdx, int shmIdx, int shmSize);

int sioFreeShm(void *pShm, pid_t ppid, int aggIdx, int shmIdx, int shmSize);

#endif
