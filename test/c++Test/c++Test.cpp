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

	hebPerformanceInit();
	for (;;) {
		char result[300*10];
		int ret = hebGetPerformance(result, sizeof(result));
		if (0 == ret) {
			printf_s(result);
		}
		printf_s("\n\n");
		Sleep(1000);
	}
	printf_s("done\n");
	getchar();
	return 0;
}
