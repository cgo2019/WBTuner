#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <DpInc.h>

extern void cbValid();
extern void cbAgg();
void cbInit()
{
}
void cbFini()
{
}

int main()
{
	int i;
	float arrayf[10], f = 1234;
	int nmemb = 10;
	FILE *fp;
	char fTrainName[32] = "train.in";

	dpExp(ctxNmemb, dpI, nmemb);

	dpSampling(1, 32, S_CV, 10, fTrainName, cbValid, NULL);
	f = dpRSD(0.12345, 1.12345, NULL);

	fp = fopen(fTrainName, "r");

	printf("Sampled:%f\n", f);

	for (i=0; i<10; i++) {
		arrayf[i] = f;
	}

	fclose(fp);

	dpAggregate(1, 
		(void *)arrayf, dpF, sizeof(float), nmemb, A_USER_CB, NULL, NULL, cbAgg);
	
	for (i=0; i<10; i++) {
		printf("%f\n", arrayf[i]);
	}

	return 0;
}
