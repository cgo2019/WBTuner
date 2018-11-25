#include <stdio.h>
#include "dpUserInc.h"
#include "DpLib.h"
#include "DpUserApi.h"
#include "SamplingIO.h"
#define arrayf(i)	\
	((gDPContext.resultTab.result[i].isBinRead) ? \
	((float *)gDPContext.resultTab.result[i].pData) :\
	((float *)dpuReadResult(i)))
