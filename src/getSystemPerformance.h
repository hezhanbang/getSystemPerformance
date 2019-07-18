// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GETSYSTEMPERFORMANCE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GETSYSTEMPERFORMANCE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef GETSYSTEMPERFORMANCE_EXPORTS
#define GETSYSTEMPERFORMANCE_API extern "C" __declspec(dllexport)
#else
#define GETSYSTEMPERFORMANCE_API extern "C" __declspec(dllimport)
#endif

GETSYSTEMPERFORMANCE_API INT32 hebPerformance_init();
GETSYSTEMPERFORMANCE_API INT32 hebPerformance_networkInfo(bool bUtf8, char* outBuf, int outBufCap);
GETSYSTEMPERFORMANCE_API INT32 hebPerformance_memoryInfo(unsigned int* usedPercent);
GETSYSTEMPERFORMANCE_API INT32 hebPerformance_cpuInfo(unsigned int* usedPercent);

GETSYSTEMPERFORMANCE_API INT32 hebPerformance_test(char* msg, char* outMsg, INT32 outCap);
