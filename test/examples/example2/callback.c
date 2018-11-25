#include <string.h>
#include "DpInc.h"

void cbAgg(int resultNum, float *arrayf) {
	int i;
	int nmemb = *((int *)dpLoad(ctxNmemb));

	for (i=0; i<resultNum; i++) {
		int j;
		float *pResult = dpuReadResult(i, arrayf);

		printf("%dth sampled result: First element=%f\n", i, pResult[0]);
		
		if (i == 5) {
			printf("Split 5th result to a new tuning process\n");
			memcpy(arrayf, pResult, sizeof(float)*nmemb);
			dpuSplit();
		}

		if (i == 8) {
			printf("Copy 8th result as the final result\n");
			memcpy(arrayf, pResult, sizeof(float)*nmemb);
		}
	}
}

