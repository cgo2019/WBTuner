#ifndef _DP_LIB_H_
#define _DP_LIB_H_

#include <stdio.h>
#include <pthread.h>

// Enforce all forked processes exit without calling
// destructor
/*
#define dpExit(status)		\
	do {										\
		schSched(S_EXIT, 0);	\
		_exit(status);				\
	} while (0);
*/
// This is used to distinguish signed and unsigned type variables.
// The reason we use it is because LLVM IR do not know whether a
// variable is signed or unsuigned, and it will result in a problem
// in later voting process. Thus, this mark will be used to mark
// a variable as unsigned and DpFront.h also defines it.
#define UNSIGNED_MARK   0xFF

// Max number of randomized variables during each sampling
#define MAX_RAND_VAR		16
// Max number of exposed variables during the whole execution
#define MAX_EXP_VAR		32
// Max number of generated child process during each mcmc sampling
#define MAX_MUTEX_NUM		4
//
#define MAX_STR_LEN			64
//
#define MAX_SAMPLING_NUM 65536
//
#define MAX_AGG_NUM		16	
//
#define MAX_CV_SUB_TODO		64	
//
#define MAX_SAMPLING_SUB_TODO 128	
//
#define SHM_EXIT_STATUS_BASE	32

#define DP_RESULT_TO_FILE

// Execution status definition
// Current process is forked inside a tuning region
#define DP_CHILD						0
// Current process is a tuning process
#define DP_PARENT						1
// This process is the parent of this sampling-aggregate
// region
#define DP_SA_PARENT				2
// This process is a tuning process forked by other 
// parent proces
#define DP_FORKED_PARENT		4

// Variable arguments macro definition
#define __NARG__(...)  __NARG_I_(__VA_ARGS__,__RSEQ_N())
#define __NARG_I_(...) __ARG_N(__VA_ARGS__)
#define __ARG_N( \
	_1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
 _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
 _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
 _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
 _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
 _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
 _61,_62,_63,N,...) N
#define __RSEQ_N() \
 63,62,61,60,                   \
 59,58,57,56,55,54,53,52,51,50, \
 49,48,47,46,45,44,43,42,41,40, \
 39,38,37,36,35,34,33,32,31,30, \
 29,28,27,26,25,24,23,22,21,20, \
 19,18,17,16,15,14,13,12,11,10, \
 9,8,7,6,5,4,3,2,1,0

// general definition for any function name
#define _VFUNC_(name, n) name##n
#define _VFUNC(name, n) _VFUNC_(name, n)
#define VFUNC(func, ...) _VFUNC(func, __NARG__(__VA_ARGS__)) (__VA_ARGS__)

// TypeID enum definition:
typedef enum { 
	dpC = 1,
	dpS,
	dpI,
	dpL,
	dpUC,
	dpUS,
	dpUI,
	dpUL,
	dpF,
	dpD,
	dpSt,
	dpP,
} TypeID;

// Randomized variable's information during sampling
typedef struct DPVar {
	union {
		char charVal;
		short shortVal;
		int intVal;
		long longVal;
		unsigned char ucharVal;
		unsigned short ushortVal;
		unsigned int uintVal;
		unsigned long long ulongVal;
		float fltVal;
		double dblVal;
	};
	TypeID tID;
	int tSize;
} DPVar;

typedef struct DPExpVar {
	DPVar var;
	char pName[MAX_STR_LEN];
} DPExpVar;

typedef struct DPAggInfo {
	TypeID tID;
	int tSize;
	void *trg;
	int nmemb;
	int stg;
	// Sampling result stored shm
	void *shmResult[MAX_SAMPLING_SUB_TODO];
} DPAggInfo;

typedef struct DPSharedTab {
	// For random sample usage
	// Number of random variables in the sampling-voting region
	int rRandVarNum;
	// Static lo
	DPVar rLoS[MAX_RAND_VAR];
	// dynamic lo 
	DPVar rLoD[MAX_RAND_VAR];
	// Static hi 
	DPVar rHiS[MAX_RAND_VAR];
	// Dynamic hi 
	DPVar rHiD[MAX_RAND_VAR];
	//
	char rRandVarName[MAX_RAND_VAR][MAX_STR_LEN];
	// For sync usage
	// Use mutex #0 from sypMtx
	int sySyncChildNum;
	// Use mutex #1 from sypMtx
	int sySyncChildDoneNum;
	// Use mutex #2 from sypMtx
	int syActiveChildNum;
	void **sySyncData;
	int *sySyncPID;

	// For aggregate usage
	// Target aggregate variable number
	int aggNum;
	// Two pieces of info are stored:
	// Bit 31: Shm ready or not
	// Bit 0-30: Result idx
	unsigned int aggShmReady[MAX_SAMPLING_SUB_TODO];
	// Number of used shared memory
	int aggShmNum;
	// Target aggregate variable information
	DPAggInfo aggInfo[MAX_AGG_NUM];
	// The border field to separate the data into
	// two fields. And memset can only do cleaning up to
	// this field.
	int memsetBorder;
	pthread_mutex_t *sypMtx[MAX_MUTEX_NUM];
	//
	pthread_mutex_t aggShmMtx;
} DPSharedTab;

// Data processing sampling result
typedef struct DPResult {
	unsigned int isParmRead:1;
	unsigned int shmIdx:31;
	int pid;
	DPVar randVar[MAX_RAND_VAR];
	void *pData[MAX_AGG_NUM];
} DPResult;

// Data processing sampling result table
typedef struct DPResultTab {
	// Left sampling child
	int todoNum;
	//
	int subTodoNum;
	//
	int resultHeadIdx;
	//
	int resultTailIdx;
	DPResult result[MAX_SAMPLING_NUM];
	// The index of best result
	int bestResultIdx;
	// The score of best result
	double bestResultScore;
	// Table of exposed variables
	DPExpVar expTab[MAX_EXP_VAR];
	//
	int expIdx;
} DPResultTab;

typedef struct DPChildTab {
	// Whether we are now in cross validation
	unsigned char cvRunning:1;
	// Cross validation callback function
	double (*cvCB)(char *, ...);  
	// ID used to identify which sampling this child
	// process belongs to
	int cvSamplingID;
	// Mutex
	pthread_mutex_t *cvMtx;
	// Validation error
	double *cvValidError;
	// Remaining child number
	int *cvRemChildNum;
	// Random sample variable table
	DPVar *cvRSVarTab;
	// K for K-fold cross validation
	char *cvValidFileName;
	int K;
} DPChildTab;

// Data processing context
typedef struct DPContext {
	// If we are doing sampling right now
	unsigned char inSampling:1;
	//
	unsigned int execStatus;
	//
	DPResultTab resultTab;
	// Table whoose fields are shared by both parent and child processes
	DPSharedTab *pSharedTab;
	// Table used by forked child process
	DPChildTab childTab;
} DPContext;

extern DPContext gDPContext;

// The validation error is opaque to the user
// and defined as global variable
extern double gCVError;

// Sampling glue function
void dpSampling(int snum, int subsnum, int stg, ...);
// Aggregate glue function
void dpAggregate(int aggNum, ...);
// Sync glue function
void dpSync(void *trg, TypeID tID, int tSize, int nmemb, ...);
// Check glue function
void dpCheck(int (*funcPtr)(void *, ...), int chkNum, ...);
// Random int glue function
int dpRSI(int lo, int hi, void *cb);
// Random float glue function
float dpRSF(float lo, float hi, void *cb);
// Random double glue function
double dpRSD(double lo, double hi, void *cb);
// Randomly generate an int value between lo and hi
int dpRSampleInt(int lo, int hi);
// Randomly generate a real value between lo and hi
double dpRSampleReal(double lo, double hi);
// This API is implemented as macro
#define dpExp(_expName, _tID, _value)	dpExpInternal(#_expName, _tID, _value)
void dpExpInternal(char *expName, TypeID tID, ...);

// This API is implemented as macro
#define dpLoad(_expName) dpLoadInternal(#_expName)
//
void *dpLoadInternal(char *expName);
int dpGetStatus();
void dpParentInit();

void dpExit(int statux);

#endif
