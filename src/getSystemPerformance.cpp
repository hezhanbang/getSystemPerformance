// getSystemPerformance.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "getSystemPerformance.h"


GETSYSTEMPERFORMANCE_API INT32 hebTest(char* msg, char* outMsg, int outCap)
{
	if (nullptr == msg || nullptr == outMsg || outCap < 100) {
		return -1;
	}

	return sprintf_s(outMsg, outCap, "hi, i am c++ performance dll, i have got your msg \"%s\"\n", msg);
}
