#include "DpInc.h"

// int  x
void cbSync(int syncNum) {
	int i, j;

	for (i=0; i<syncNum-1; i++) {
		int *pSyncI = dpuReadSync(i);
		for (j=i+1; j<syncNum; j++) {
			int *pSyncJ = dpuReadSync(j);
			printf("In sync callback:\n"
				"x(%d)=%d x(%d)=%d\n", i, *pSyncI, j, *pSyncJ);

			if (*pSyncI == *pSyncJ) {
				dpuSplitSync(i);
				dpuSplitSync(j);
			}
		}
	}
}
