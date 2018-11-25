#ifndef _SAMPLING_STG_H_
#define _SAMPLING_STG_H_

typedef enum {
	S_USER_CB = 0,
	S_RAND,
	S_MCMC_MAX,
	S_MCMC_MIN,
	S_CV,
} SStg;

typedef void (*sstgFuncPtr)(int, ...);  

extern sstgFuncPtr sstgFuncTab[];

#endif
