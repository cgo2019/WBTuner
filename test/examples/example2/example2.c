#include <stdio.h>
#include "DpInc.h"

extern void cbAgg();

int main()
{
	int i;
	float arrayf[10], f = 1234;
	int nmemb = 10;

	dpExp(ctxNmemb, dpI, nmemb);
	dpSampling(10, 32, S_RAND);
	f = dpRSF(0.12345, 1.12345, NULL);

	for (i=0; i<10; i++) {
		arrayf[i] = f;
	}

	dpAggregate(1, 
		(void *)arrayf, dpF, sizeof(float), nmemb, A_USER_CB, NULL, NULL, cbAgg);

	for (i=0; i<10; i++) {
		printf("%f\n", arrayf[i]);
	}
}
