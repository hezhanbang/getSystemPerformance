// c++Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../../src/getSystemPerformance.h"
#pragma comment(lib, "getSystemPerformance.lib")

int main()
{
	printf_s("hello\n");

	char outStr[255];
	int ret = hebPerformanceTest("i am main.exe", outStr, sizeof(outStr));

	getchar();
	return 0;
}

