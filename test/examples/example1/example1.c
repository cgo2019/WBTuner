#include <stdio.h>
#include "DpInc.h"

int main()
{
	int i;
	float arrayf[10], f = 1234;
	int nmemb = 10;

	dpSampling(10, 32, S_RAND);
	f = dpRSF(0.12345, 1.12345, NULL);

	printf("sampled:%f\n", f);
	for (i=0; i<10; i++) {
		arrayf[i] = f;
	}

	dpAggregate(1, 
		(void *)arrayf, dpF, sizeof(float), 10, A_AVG, NULL, NULL, NULL);

	for (i=0; i<10; i++) {
		printf("%f\n", arrayf[i]);
	}
}
