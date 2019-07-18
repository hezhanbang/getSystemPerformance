// c++Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../../src/getSystemPerformance.h"
#pragma comment(lib, "getSystemPerformance.lib")

int main()
{
	/*
	char outStr[255];
	int ret = hebPerformanceTest("i am main.exe", outStr, sizeof(outStr));
	assert(ret == strlen(outStr));

	printf_s(outStr);
	*/

	hebPerformance_init();
	for (;;) {
		char result[300 * 10];

		//network flow speed
		int ret = hebPerformance_networkInfo(false, result, sizeof(result));
		if (ret > 0) {
			printf_s(result);
		}

		//memory usage
		UINT32 usedMemory = 0;
		ret = hebPerformance_memoryInfo(&usedMemory);
		if (ret > 0) {
			printf_s("memory: %d%%, ", usedMemory);
		}

		//cpu usage
		UINT32 usedCpu = 0;
		ret = hebPerformance_cpuInfo(&usedCpu);
		if (ret > 0) {
			printf_s("cpu: %d%%\n", usedCpu);
		}

		printf_s("\n\n");
		Sleep(1000);
	}
	printf_s("done\n");
	getchar();
	return 0;
}
