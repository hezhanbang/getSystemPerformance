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

	hebGetPerformance();

	printf_s("done\n");
	getchar();
	return 0;
}
