#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "SamplingIO.h"

// This is an util lib for reading/writing sampling output 
// from/to a file.

// Sampling result directory
const char sioResultDir[] = ".dpResultDir\0";
// Sync data result directory
const char sioSyncDir[] = ".dpSyncDir\0";
// Cross validation splitted data directory
const char sioCVDir[] = ".dpCVDir\0";

extern int errno;
static char gTmpBuf[MAX_STR_LEN * MAX_STR_LEN];

// Initialize infrastructure file io, return 0, else return -1.
int sioInit()
{
	int cc = 0;

	// Sampling result dir does not exist
	if (access(sioResultDir, F_OK) == -1) {
		if (mkdir(sioResultDir, 0755) < 0) {
			printf("Error<sioInit>: Cannot create directory for sampling result\n");
			return -1;
		}
	} else {
		sprintf(gTmpBuf, "rm -rf %s/*", sioResultDir);
		cc = system(gTmpBuf);
	}
	
	// Sync dir does not exist
	if (access(sioSyncDir, F_OK) == -1) {
		if (mkdir(sioSyncDir, 0755) < 0) {
			printf("Error<sioInit>: Cannot create directory for synchronization\n");
			return -1;
		}
	} else {
		sprintf(gTmpBuf, "rm -rf %s/*", sioSyncDir);
		cc = system(gTmpBuf);
	}

	return cc;
}

// Finalize the infrastructure file io.
int sioFini()
{
	int cc;

	sprintf(gTmpBuf, "rm -rf %s/%d.syncData", sioSyncDir, getpid());

	cc = system(gTmpBuf);

	if (cc) {
		printf("Error<sioFini>: Cannot remove synchronization directory %s,"
			" errno=%d\n",
			gTmpBuf, errno);
		return -1;
	}
	
	sprintf(gTmpBuf, "rm -rf %s/%d.dir", sioResultDir, getpid());

	cc = system(gTmpBuf);

	if (cc) {
		printf("Error<sioFini>: Cannot remove result directory %s,"
			" errno=%d\n",
			gTmpBuf, errno);
		return -1;
	}

	return 0;
}

// Initialize sampling-aggregate region file io. If ok, return 0, else return -1.
int sioSARegionInit()
{
	// Create a new sync directory for a newly forked parent process.
	sprintf(gTmpBuf, "%s/%d.syncData", sioSyncDir, getpid());
	
	// Synchronization directory should not exist
	if (access(gTmpBuf, F_OK) == -1) {
		if (mkdir(gTmpBuf, 0755) < 0) {
			printf("Error<sioSARegionInit>: "
				"Cannot create sync directory %s\n",
				gTmpBuf);
			dpExit(-1);
		}
	} else {
		printf("Error<sioSARegionInit>: "
			"Cannot create sync directory %sn",
			gTmpBuf);
		dpExit(-1);
	}

	// Create a new result directory for a newly forked parent process.
	sprintf(gTmpBuf, "%s/%d.dir", sioResultDir, getpid());
	// Result directory should not exist
	if (access(gTmpBuf, F_OK) == -1) {
		if (mkdir(gTmpBuf, 0755) < 0) {
			printf("Error<sioSARegionInit>: "
				"Cannot create result directory %s\n",
				gTmpBuf);
			dpExit(-1);
		}
	} else {
		printf("Error<sioSARegionInit>: "
			"Cannot create result directory %s\n",
			gTmpBuf);
		dpExit(-1);
	}

	return 0;
}

// Finalize sampling-aggregate region file io. If ok, return 0, else return -1.
// Remove .result and .parm created by child processes
int sioSARegionFini()
{

	if ((gDPContext.execStatus & (DP_PARENT | DP_SA_PARENT)) !=
			(DP_PARENT | DP_SA_PARENT))
	{	
		printf("Error<sioSARegionFini>: "
			"Only parent of this sampling-aggregate region can do sioFini()\n");
		assert(0);
	}
	
	sprintf(gTmpBuf, "rm -f %s/%d.dir/*", sioResultDir, getpid());

	return system(gTmpBuf);
}

int sioRmResult(int ppid, int pid)
{
	// Failed to remove parm files is tolerable
	sprintf(gTmpBuf, "%s/%d.dir/%d.parm", sioResultDir, ppid, pid);
	remove(gTmpBuf);

	sprintf(gTmpBuf, "%s/%d.dir/%d.result", sioResultDir, ppid, pid);
	return remove(gTmpBuf);
}

int sioSyncInit()
{
	return 0;
}

int sioSyncFini(pid_t ppid)
{
	sprintf(gTmpBuf, "rm -rf %s/%d.syncData/*.sync", sioSyncDir, ppid);

	return system(gTmpBuf);
}

int sioWriteResult(void *buf, int nmemb, int size, 
	pid_t ppid, pid_t pid)
{
	sprintf(gTmpBuf, "%s/%d.dir/%d.result", sioResultDir, ppid, pid);

	if (sioWriteBin(buf, nmemb, size, gTmpBuf) < 0) {
		printf("Error<sioWriteResult>: Cannot write result of %s, errno=%d\n", 
			gTmpBuf, errno);
		return -1;
	}

	return 0;
}

int sioWriteSyncData(void *buf, int nmemb, int size, 
	pid_t ppid, pid_t pid)
{
	sprintf(gTmpBuf, "%s/%d.syncData/%d.sync", sioSyncDir, ppid, pid);

	if (sioWriteBin(buf, nmemb, size, gTmpBuf) < 0) {
		printf("Error<sioWriteSyncData>: Cannot write result of %s, errno=%d\n", 
			gTmpBuf, errno);
		return -1;
	}

	return 0;
}

// Write a sampling result from buf to file .result
// Parm:
// 1. Source data buffer
// 2. Data offset
// 3. Number of elements
// 4. Per element size
// 5. Target file path
int sioWriteBin(void *buf, int nmemb, int size, char *path)
{
	FILE *file;
	int cc;

	if ((file = fopen(path, "ab")) == 0) {
		printf("Error<sioWriteBin>: Cannot write %s to output\n", path);
		return -1;
	}

	if (nmemb) {
		cc = fwrite(buf, size, nmemb, file);

		if (cc != nmemb) {
			printf("Error<sioWriteBin>: Unexpected writing length to %s, "
					"expected=%d real=%d %d\n", path, nmemb, cc, errno);
			return -1;
		}
	} else {
		cc = fwrite((char *)&nmemb, sizeof(nmemb), 1, file);
		cc = fwrite(buf, size, 1, file);
		
		if (cc != 1) {
			printf("Error<sioWriteBin>: Unexpected writing length to %s, "
					"expected=1 real=%d %d\n", path, cc, errno);
			return -1;
		}
	}

	fflush(file);
	fclose(file);

	return 0;
}

//
int sioWriteParm(char *info, pid_t ppid, pid_t pid)
{
	FILE *file;
	int cc;

	sprintf(gTmpBuf, "%s/%d.dir/%d.parm", sioResultDir, ppid, pid);
		
	if ((file = fopen(gTmpBuf, "a")) == 0) {
		printf("Error<sioWriteParm>: Cannot write %s to output, errno = %d\n", 
			gTmpBuf, errno);
		return -1;
	}

	cc = fwrite(info, sizeof(char), strlen(info), file);

	if (cc != strlen(info)) {
		printf("Error<sioWriteInfo>: Unexpected writing length to %s, "
				"expected=%d real=%d errno=%d\n", 
				gTmpBuf, (int)strlen(info), cc, errno);
		return -1;
	}

	fclose(file);

	return 0;
}

void *sioReadResult(pid_t ppid, pid_t pid)
{
	struct stat fileStat;
	void *pResult;

	sprintf(gTmpBuf, "%s/%d.dir/%d.result", sioResultDir, ppid, pid);

	if (stat(gTmpBuf, &fileStat) == -1) {
		printf("Error<sioReadResult>: Cannot read result of %s, errno=%d\n", 
			gTmpBuf, errno);
		return NULL;
	} else {
		pResult = (void *) malloc(fileStat.st_size);
	}

	if (sioReadBin(pResult, fileStat.st_size, 1, gTmpBuf) < 0) {
		printf("Error<sioReadResult>: Cannot read result of %s, errno=%d\n", 
			gTmpBuf, errno);
		return NULL;
	}

	return pResult;
}

int sioReadSyncData(void *buf, int nmemb, int size, pid_t ppid, pid_t pid)
{
	sprintf(gTmpBuf, "%s/%d.syncData/%d.sync", sioSyncDir, ppid, pid);

	if (sioReadBin(buf, nmemb, size, gTmpBuf) < 0) {
		printf("Error<sioReadSyncData>: Cannot read result of %s, errno=%d\n", 
			gTmpBuf, errno);
		return -1;
	}

	return 0;
}

int sioReadSyncDataAll(int syncChildNum, int nmemb, int size, pid_t ppid)
{
	int i;
	DIR *dir;
	struct dirent *dirEnt;

	if (!gDPContext.pSharedTab->sySyncData) {
		printf("Error<sioReadSyncDataAll>: Cannot read all sync data of %d, "
			"gDPContext.pSharedTab->sySyncData not allocated yet\n", 
			ppid);

		return -1;
	}

	sprintf(gTmpBuf, "%s/%d.syncData", sioSyncDir, ppid);

	if ((dir = opendir(gTmpBuf)) != NULL) {
		for (i=0; (dirEnt = readdir(dir)) != NULL;) {
			int cc;
		
			if (!strstr(dirEnt->d_name, ".sync"))
				continue;

			sprintf(gTmpBuf, "%s/%d.syncData/%s", 
				sioSyncDir, ppid, dirEnt->d_name);
			
			cc = sioReadBin(gDPContext.pSharedTab->sySyncData[i], 
				nmemb, size, gTmpBuf);

			strncpy(gTmpBuf, dirEnt->d_name, 
				strstr(dirEnt->d_name, ".") - dirEnt->d_name);

			gDPContext.pSharedTab->sySyncPID[i] = atoi(gTmpBuf);

			i++;

			if (cc < 0) {
				closedir (dir);
				return -1;
			}
		}

		closedir (dir);
	} else {
		printf("Error<sioReadSyncDataAll>: Cannot open dir=%s\n", gTmpBuf);
		return -1;
	}

	return 0;
}

// Read a sampling result from file to buf
// Parm:
// 1. Target data buffer
// 2. Number of elements
// 3. Per element size
// 4. Source file, named as process id
int sioReadBin(void *buf, int nmemb, int size, char *path)
{
	FILE *file;
	int cc;

	if ((file = fopen(path, "rb")) == 0) {
		printf("Error<sioReadBin>: Cannot read %s, errno=%d\n", path, errno);
		return -1;
	}

	if (nmemb) {
		cc = fread(buf, size, nmemb, file);

		if (cc != nmemb) {
			printf("Error<sioReadBin>: Unexpected reading length from %s, "
					"expected=%d real=%d\n", path, nmemb, cc);
			return -1;
		}
	} else {
		cc = fread(buf, size, 1, file);

		if (cc != 1) {
			printf("Error<sioReadBin>: Unexpected reading length from %s, "
					"expected=1 real=%d errno=%d\n", path, cc, errno);
			return -1;
		}
	}

	fclose(file);
	return 0;
}

int sioReadParm(DPVar *pRandVar, pid_t ppid, pid_t pid)
{
	FILE *file;
	int i = 0;

	sprintf(gTmpBuf, "%s/%d.dir/%d.parm", sioResultDir, ppid, pid);
	
	if ((file = fopen(gTmpBuf, "r")) == NULL || !pRandVar) {
		printf("Error<sioReadInfo>: Cannot read %s, errno=%d, pRandVar=%lx\n", 
			gTmpBuf, errno, (long unsigned int)pRandVar);
		return -1;
	}

	while (fscanf(file, "%u %d %s\n", 
			&pRandVar[i].tID, &pRandVar[i].tSize, gTmpBuf) != EOF) {
		
		switch (pRandVar[i].tID) {
			case dpI:
				pRandVar[i].intVal = atoi(gTmpBuf);
				break;
			case dpF:
				pRandVar[i].fltVal = strtof(gTmpBuf, NULL);
				break;
			case dpD:
				pRandVar[i].dblVal = atof(gTmpBuf);
				break;
			default:
				printf("Error<sioReadInfo>: Unexpected parameter "
					"type=%d size=%d value=%s\n",
					pRandVar[i].tID, pRandVar[i].tSize, gTmpBuf);
				dpExit(-1);
				break;
		}

		i++;
	}

	fclose(file);
	return i;
}

int sioSplitSyncInit(int resultIdx, pid_t ppid)
{
	int cc;

	if (!gDPContext.pSharedTab->sySyncPID) {
		printf("Error<sioSplitSyncInit>: "
			"This api can only be used during process sync\n");
		return -1;
	}

	if (resultIdx >= gDPContext.pSharedTab->sySyncChildDoneNum) {
		printf("Error<sioSplitSyncInit>: "
			"Result index=%d exceeds sync process number=%d\n",
			resultIdx, gDPContext.pSharedTab->sySyncChildDoneNum);
		return -1;
	}

	sprintf(gTmpBuf, "%s/%d.syncData/%d.forked", 
		sioSyncDir, ppid, gDPContext.pSharedTab->sySyncPID[resultIdx]);

	// Target process not forked yet, so initiate the fork by generating a 
	// pid.sync.fork file to indicate target process
	if (access(gTmpBuf, F_OK) == -1) {
		sprintf(gTmpBuf, "touch %s/%d.syncData/%d.fork", 
				sioSyncDir, ppid, gDPContext.pSharedTab->sySyncPID[resultIdx]);
		cc = system(gTmpBuf);
	} else {
		printf("Warning<sioSplitSyncInit>: process=%d has already forked\n",
			gDPContext.pSharedTab->sySyncPID[resultIdx]);
		return -1;
	}

	return cc;
}

int sioSplitSyncCheck(pid_t ppid, pid_t pid)
{
	sprintf(gTmpBuf, "%s/%d.syncData/%d.fork", 
		sioSyncDir, ppid, pid);

	// User want to fork this process
	if (access(gTmpBuf, F_OK) == 0) {
		sprintf(gTmpBuf, "%s/%d.syncData/%d.forked", 
			sioSyncDir, ppid, pid);

		// Already forked, do nothing
		if (access(gTmpBuf, F_OK) == 0) {
			return 0;
		} else {
			int cc;

			sprintf(gTmpBuf, "touch %s/%d.syncData/%d.forked", 
				sioSyncDir, ppid, pid);
			
			cc = system(gTmpBuf);

			return (cc == 0) ? 1 : 0;
		}
	}

	return 0;
}

char **sioCVInit(char *pTrainName, int K)
{
	FILE *pTrain;
	char **ppSplittedName;
	int sampleNum = 0, splittedSampleNum;
	int i, j, cc;

	// Cross validation dir does not exist
	if (access(sioCVDir, F_OK) == -1) {
		if (mkdir(sioCVDir, 0755) < 0) {
			printf("Error<sioCVInit>: Cannot create directory for cross validation\n");
			return NULL;
		}
	} else {
		sprintf(gTmpBuf, "rm -rf %s/*", sioCVDir);
		cc = system(gTmpBuf);
	}

	if ((pTrain = fopen(pTrainName, "r")) == NULL) {
		printf("Error<sioCVInit>: "
			"Cannot open training file \"%s\" for cross validation\n",
			pTrainName);
		return NULL;
	}

	// Count the total number of samples in the data
	while (fgets(gTmpBuf, sizeof(gTmpBuf), pTrain) != NULL) {
		sampleNum++;
	}

	splittedSampleNum = 
		((sampleNum % K) == 0) ? sampleNum / K : (sampleNum / K) + 1;

	printf("Number of samples in training data %s is %d\n", 
		pTrainName, sampleNum);

	ppSplittedName = (char **) malloc(sizeof(char *) * (K * 2));

	// Get the last occurence of "/" and slice the last
	// string
	if (strrchr(pTrainName, '/'))
		pTrainName = strrchr(pTrainName, '/') + 1;

	// Begin splitting
	for (i=0, j=0; i<K; i++, j=0) {
		FILE *pSplittedTrain, *pSplittedValid;

		sprintf(gTmpBuf, "%s/%s.%d", sioCVDir, pTrainName, i);

		if ((pSplittedTrain = fopen(gTmpBuf, "w")) == NULL) {
			printf("Error<sioCVInit>: Cannot split file %s for training\n",
				gTmpBuf);
		}
		
		ppSplittedName[i] = strdup(gTmpBuf);
		
		sprintf(gTmpBuf, "%s/%s.valid.%d", sioCVDir, pTrainName, i);

		if ((pSplittedValid = fopen(gTmpBuf, "w")) == NULL) {
			printf("Error<sioCVInit>: Cannot split file %s for validation\n",
				gTmpBuf);
		}
		
		ppSplittedName[i + K] = strdup(gTmpBuf);

		rewind(pTrain);
		// Go through all samples
		while (fgets(gTmpBuf, sizeof(gTmpBuf), pTrain) != NULL) {
			if (j >= (splittedSampleNum * i) && j < (splittedSampleNum * (i + 1))) {
				fputs(gTmpBuf, pSplittedValid);
			} else {
				fputs(gTmpBuf, pSplittedTrain);
			}
			j++;
		}
		fclose(pSplittedTrain);
		fclose(pSplittedValid);
	}

	fclose(pTrain);

	return ppSplittedName;
}

// Both parent and child process need to do the cleanup
// For child process, idx will be 0 to K-1
// For parent process, idx will be K
int sioCVFini(char **ppSplittedName, int idx, int K)
{
	int i;

	for (i=0; i<K; i++) {
		if (i == idx)
			continue;

		free(ppSplittedName[i]);
		free(ppSplittedName[i + K]);
	}

	free(ppSplittedName);

	// Child process return directly
	if (idx < K)
		return 0;

	sprintf(gTmpBuf, "rm -rf %s/*", sioCVDir);
	return system(gTmpBuf);
}

// Create the shared memory for storing sampling result
void *sioCreateShm(pid_t ppid, int aggIdx, int shmIdx, int shmSize)
{
	void *pShm;
	struct stat sb;
	int fd;

	sprintf(gTmpBuf, "%d.%d.%d", ppid, aggIdx, shmIdx);

	fd = shm_open(gTmpBuf, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

	// The shared memory is newly opened, initialize it
	if (fd < 0) {
		printf("Error<sioGetShm>: Cannot open shared memory, "
				"name=%s errno=%d\n", gTmpBuf, errno);
		return NULL;
	}

	if (fstat(fd, &sb) < 0) {
		printf("Error<sioGetShm>: Cannot get the size of shared memory, "
				"name=%s fd=%d errno=%d\n", gTmpBuf, fd, errno);
		return NULL;
	}

	// A new shm is being created, initialize it
	if (sb.st_size == 0) {
		// For child process to initialize shm, shmIdx < MAX_SAMPLING_SUB_TODO
		// For parent process to initialize shm, shmIdx == MAX_SAMPLING_SUB_TODO
		if (gDPContext.execStatus != DP_CHILD && shmIdx != MAX_SAMPLING_SUB_TODO) {
			printf("Error<sioGetShm>: "
				"Only child process can create shared memory, "
				"exec status=%x name=%s\n",
				gDPContext.execStatus, gTmpBuf);
			return NULL;
		}

		// Truncate the file
		if (ftruncate(fd, shmSize) == -1) {
			printf("Error<sioGetShm>: Cannot truncate shared mem, "
				"name=%s mem size=%d", gTmpBuf, shmSize);
			return NULL;
		}
	}

	// Map the memory
	pShm = mmap(NULL, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	if (pShm == MAP_FAILED) {
			printf("Error<sioGetShm>: "
				"Cannot map shared mem, name=%s fd=%d mem size=%d errno=%d\n", 
				gTmpBuf, fd, shmSize, errno);
			return NULL;
	}

	return pShm;
}

int sioFreeShm(void *pShm, pid_t ppid, int aggIdx, int shmIdx, int shmSize)
{
	if ((gDPContext.execStatus & (DP_PARENT | DP_SA_PARENT)) != 
			(DP_PARENT | DP_SA_PARENT)) 
	{
		printf("Error<sioFreeShm>: "
			"Only parent process can free shared memory, exec status=%x\n",
			gDPContext.execStatus);
		goto bail;
	}

	// unmap the shared memory
	if (munmap(pShm, shmSize) < 0) {
		printf("Error<sioFreeShm>: Cannot unmap shared memory, errno=%d\n",
			errno);
		goto bail;
	}

	sprintf(gTmpBuf, "%d.%d.%d", ppid, aggIdx, shmIdx);

	// unlink the shared memory
	if (shm_unlink(gTmpBuf) < 0) {
		printf("Error<sioFreeShm>: Cannot unlink shared memory, "
			"shm name=%s errno=%d\n",
			gTmpBuf, errno);
		goto bail;
	}

	return 0;

bail:
	return -1;
}
