#include <stdio.h>
#include "DpInc.h"

extern void cbChk();

int main()
{
	int i = 3;
	float arrayf[10], f = 1234;
	int nmemb = 10;

	dpSampling(10, 32, S_RAND);
	f = dpRSF(0.12345, 1.12345, NULL);

	dpCheck(cbChk, 2, (void *)&f, (void *)&i);

	printf("%f\n", f);

	for (i=0; i<10; i++) {
		arrayf[i] = f;
	}

	dpAggregate(1, 
		(void *)arrayf, dpF, sizeof(float), nmemb, A_AVG);
	
	for (i=0; i<10; i++) {
		printf("%f\n", arrayf[i]);
	}
}
