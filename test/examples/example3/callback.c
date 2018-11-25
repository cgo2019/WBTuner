#include <string.h>
#include "DpInc.h"

int cbChk(float *f, int *i) {
	printf("%d\n", *i);

	if (*f < 1)
		return 0;
	else
		return 1;
}
