// getSystemPerformance.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "getSystemPerformance.h"


// This is an example of an exported variable
GETSYSTEMPERFORMANCE_API int ngetSystemPerformance=0;

// This is an example of an exported function.
GETSYSTEMPERFORMANCE_API int fngetSystemPerformance(void)
{
    return 42;
}

// This is the constructor of a class that has been exported.
// see getSystemPerformance.h for the class definition
CgetSystemPerformance::CgetSystemPerformance()
{
    return;
}
