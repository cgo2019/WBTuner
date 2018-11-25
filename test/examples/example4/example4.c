#include <stdio.h>
#include <DpInc.h>

extern void cbSync();

int main()
{
	int i;
	int x;
	float arrayf[10], f = 1234;
	int nmemb = 10;

	dpSampling(3, 32, S_RAND);
	f = dpRSF(0.12345, 1.12345, NULL);
	x = dpRSI(1, 2, NULL);

	printf("Sampled:%f %x\n", f, x);
	
	dpSync((void *)&x, dpI, sizeof(int), 1, cbSync);
	
	for (i=0; i<10; i++) {
		arrayf[i] = f;
	}

	dpAggregate(1, 
		(void *)arrayf, dpF, sizeof(float), nmemb, A_AVG);
	
	for (i=0; i<10; i++) {
		printf("%f\n", arrayf[i]);
	}
}
