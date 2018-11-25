#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "DpInc.h"

double cbValid(char *fValidName, float *arrayf)
{
	FILE *fp = fopen(fValidName, "r");
	char buf[32];
	double validError = 0;

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		printf("validation input=%s", buf);
		validError += atoi(buf) * arrayf[0];
	}

	fclose(fp);

	return validError;
}

void cbAgg(int resultNum, float *arrayf)
{
	int i, nmemb;

	for (i=0; i<resultNum; i++) {
		double cvError = dpuReadCVResult(i);
	
		printf("cvError=%lf\n", cvError);
	}

	void *pResult = dpuReadResult(0, arrayf);
	nmemb = *((int *)dpLoad(ctxNmemb));

	memcpy(arrayf, pResult, sizeof(float) * nmemb);
}
