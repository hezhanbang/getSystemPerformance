// getSystemPerformance.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "getSystemPerformance.h"


bool gHebInited = false;

char* Wide2Multi_needDelete(const WCHAR* wideContent, unsigned int multiType) {

	int i = WideCharToMultiByte(multiType, 0, wideContent, -1, NULL, 0, NULL, NULL);
	char *multiContent = new char[i + 1];
	WideCharToMultiByte(multiType, 0, wideContent, -1, (LPSTR)multiContent, i, NULL, NULL);
	multiContent[i] = '\0';

	return multiContent;
};

void dumpOneInterface(PIP_ADAPTER_ADDRESSES pCurrAddresses) {
	printf("\tAdapter name: %s\n", pCurrAddresses->AdapterName);

	char* friendlyName = Wide2Multi_needDelete(pCurrAddresses->FriendlyName, CP_ACP);
	char* description = Wide2Multi_needDelete(pCurrAddresses->Description, CP_ACP);

	printf("\tFriendly name: %s\n", friendlyName); //在"ipconfig"上看到的网卡名称
	printf("\tDescription: %s\n", description); //在"ipconfig /all"上看到的网卡描述。

	delete(friendlyName);
	delete(description);

	/*
	网卡类型
	IfType==6，即IF_TYPE_ETHERNET_CSMACD，An Ethernet network interface.
	IfType==24，即IF_TYPE_SOFTWARE_LOOPBACK，A software loopback network interface.
	*/
	printf("\tIfType: %ld\n", pCurrAddresses->IfType);

	/*
	获取网卡的状态。
	OperStatus==1，即IfOperStatusUp，The interface is up and able to pass packets.
	*/
	printf("\tOperStatus: %ld\n", pCurrAddresses->OperStatus);

	/*
	物理地址，mac地址。
	回环虚拟网卡，没有mac地址。
	*/
	if (pCurrAddresses->PhysicalAddressLength != 0) {
		printf("\tPhysical address: ");
		for (int i = 0; i < (int)pCurrAddresses->PhysicalAddressLength;
			i++) {
			if (i == (pCurrAddresses->PhysicalAddressLength - 1))
				printf("%.2X\n",
				(int)pCurrAddresses->PhysicalAddress[i]);
			else
				printf("%.2X-",
				(int)pCurrAddresses->PhysicalAddress[i]);
		}
	}

	//获取本网卡的多个本地IP地址
	PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
	if (pUnicast != NULL) {
		for (int i = 0; pUnicast != NULL; i++) {
			if (AF_INET == pUnicast->Address.lpSockaddr->sa_family) {
				char localIP[20];
				sockaddr_in* sa_in = (sockaddr_in *)pUnicast->Address.lpSockaddr;
				inet_ntop(AF_INET, &sa_in->sin_addr, localIP, sizeof(localIP));

				printf("\tIPV4 Unicast Address:%s\n", localIP);
			}

			pUnicast = pUnicast->Next;
		}
	}
	/*
	百兆网卡，即TransmitLinkSpeed和ReceiveLinkSpeed都等于100000000，
	即100Mbps(Mbps = Mega(兆) bit per second)，即每秒钟能传送100M的比特位数据。

	100000000 = 100,  000,  000
	_________________|kBit |kBit|
	_________________|    Mb    |
	__________=  100 Mb
	*/

	printf("\tTransmit link speed: %I64u\n", pCurrAddresses->TransmitLinkSpeed);
	printf("\tReceive link speed: %I64u\n", pCurrAddresses->ReceiveLinkSpeed);

	printf("\n");
}

int hebInit() {
	bool bOk = false;
	ULONG Iterations = 0;
	DWORD dwRetVal = 0;
	ULONG outBufCap = 200;
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
#define MAX_TRIES 3

	do {
		pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufCap);

		dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &outBufCap);
		if (ERROR_BUFFER_OVERFLOW == dwRetVal) {
			free(pAddresses);
			pAddresses = nullptr;
		}
		else {
			if (NO_ERROR == dwRetVal) {
				bOk = true;
			}
			break;
		}
		Iterations++;
	} while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

	assert(bOk);

	//获取网卡带宽
	PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;

	while (pCurrAddresses) {
		dumpOneInterface(pCurrAddresses);
		pCurrAddresses = pCurrAddresses->Next;
	}
	return 0;
}

GETSYSTEMPERFORMANCE_API INT32 hebGetPerformance() {
	if (false == gHebInited) {
		int ret = hebInit();
		assert(0 == ret);
		gHebInited = true;
	}



	return 0;
}

GETSYSTEMPERFORMANCE_API INT32 hebPerformanceTest(char* msg, char* outMsg, INT32 outCap)
{
	if (nullptr == msg || nullptr == outMsg || outCap < 100) {
		return -1;
	}

	return sprintf_s(outMsg, outCap, "hi, i am c++ performance dll, i have got your msg \"%s\"\n", msg);
}

