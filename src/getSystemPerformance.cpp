// getSystemPerformance.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "getSystemPerformance.h"

struct HebInterfaceInfo {
	char macAddr[56];
	char name[240];
	std::string allIP;
	uint32_t MaxSpeedBps;

	//开机到现在，统计到的流量。
	bool bNowFlowDataValid;
	DWORD dwNowTotalSendByte;
	DWORD dwNowTotalRecvByte;

	bool bLastFlowDataValid;
	DWORD dwLastTotalSendByte;
	DWORD dwLastTotalRecvByte;
};

struct HebAllStatusInfo {
	HebInterfaceInfo interfaces[20];
	int interfaceCount;

	std::chrono::time_point<std::chrono::system_clock> actionTime;
};

struct HebCpuStatusInfo {
	FILETIME m_prevKernelTime;
	FILETIME m_prevUserTime;
	FILETIME m_prevIdleTime;
};

HebAllStatusInfo* gHebNet = nullptr;
HebCpuStatusInfo* gHebCpu = nullptr;

WCHAR* hebMulti2Wide_needDeleteArray(const char* multiContent, unsigned int multiType) {

	int i = MultiByteToWideChar(multiType, NULL, multiContent, strlen(multiContent), NULL, 0);
	WCHAR *wideContent = new WCHAR[i + 1];
	MultiByteToWideChar(multiType, NULL, multiContent, strlen(multiContent), wideContent, i);
	wideContent[i] = '\0';

	return wideContent;
};

char* hebWide2Multi_needDeleteArray(const WCHAR* wideContent, unsigned int multiType) {

	int i = WideCharToMultiByte(multiType, 0, wideContent, -1, NULL, 0, NULL, NULL);
	char *multiContent = new char[i + 1];
	WideCharToMultiByte(multiType, 0, wideContent, -1, (LPSTR)multiContent, i, NULL, NULL);
	multiContent[i] = '\0';

	return multiContent;
};

bool hebAnsi2Utf8(const char* src, char* utf8Buf, int utf8BufSize)
{
	if (!src || strlen(src) < 1 || !utf8Buf || utf8BufSize < 2)
	{
		return false;
	}

	WCHAR* wideStr = hebMulti2Wide_needDeleteArray(src, CP_ACP);
	char* utf8Str = hebWide2Multi_needDeleteArray(wideStr, CP_UTF8);

	if (strlen(utf8Str) + 1 > utf8BufSize)
	{
		delete[] wideStr;
		delete[] utf8Str;
		return false;
	}
	strcpy_s(utf8Buf, utf8BufSize, utf8Str);

	delete[] wideStr;
	delete[] utf8Str;
	return true;
}

int hebMacToStr(char* destBuf, int destBufCap, UCHAR* mac, int macLen) {
	if (nullptr == destBuf || nullptr == mac || macLen < 3) {
		return 0;
	}
	if (destBufCap < macLen * 3) { //include '\0';
		return 0;
	}

	char* cur = destBuf;
	cur[0] = '\0';

	for (int i = 0; i < (int)macLen; i++) {
		if (i == (macLen - 1)) {
			cur += sprintf_s(cur, destBuf + destBufCap - cur, "%.2X",
				(int)mac[i]);
		}
		else {
			cur += sprintf_s(cur, destBuf + destBufCap - cur, "%.2X-",
				(int)mac[i]);
		}
	}

	cur[0] = '\0';
	return cur - destBuf;
}

void dumpOneInterface(PIP_ADAPTER_ADDRESSES pCurrAddresses) {
	printf("\tAdapter name: %s\n", pCurrAddresses->AdapterName);

	char* friendlyName = hebWide2Multi_needDeleteArray(pCurrAddresses->FriendlyName, CP_ACP);
	char* description = hebWide2Multi_needDeleteArray(pCurrAddresses->Description, CP_ACP);

	printf("\tFriendly name: %s\n", friendlyName); //在"ipconfig"上看到的网卡名称
	printf("\tDescription: %s\n", description); //在"ipconfig /all"上看到的网卡描述。

	delete[] friendlyName;
	delete[] description;

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
	回环虚拟网卡，没有mac地址，下线的设备可能也会没有mac地址。
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

int32_t handleOneInterface(PIP_ADAPTER_ADDRESSES pCurrAddresses, HebInterfaceInfo &oneInfo) {
	//网卡类型，IfType==24，即IF_TYPE_SOFTWARE_LOOPBACK，A software loopback network interface.
	if (IF_TYPE_SOFTWARE_LOOPBACK == pCurrAddresses->IfType) {
		return -1;
	}

	/*
	获取网卡的状态。
	OperStatus==1，即IfOperStatusUp，The interface is up and able to pass packets.
	*/
	if (IfOperStatusUp != pCurrAddresses->OperStatus) {
		return -2;
	}

	/*
	物理地址，mac地址。
	回环虚拟网卡，没有mac地址，下线的设备可能也会没有mac地址。
	*/
	int macStrLen = hebMacToStr(oneInfo.macAddr, sizeof(oneInfo.macAddr), pCurrAddresses->PhysicalAddress, pCurrAddresses->PhysicalAddressLength);
	if (macStrLen <= 1) {
		return -3;
	}
	assert(macStrLen + 5 <= sizeof(oneInfo.macAddr));

	//在"ipconfig"上看到的网卡名称
	char* friendlyName = hebWide2Multi_needDeleteArray(pCurrAddresses->FriendlyName, CP_ACP);
	int nameLen = strcpy_s(oneInfo.name, sizeof(oneInfo.name), friendlyName);
	assert(nameLen + 5 <= sizeof(oneInfo.name));
	delete[] friendlyName;

	//获取本网卡的多个本地IP地址
	oneInfo.allIP = "";
	PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
	if (pUnicast != NULL) {
		for (int i = 0; pUnicast != NULL; i++) {
			if (AF_INET == pUnicast->Address.lpSockaddr->sa_family) {
				char localIP[20];
				sockaddr_in* sa_in = (sockaddr_in *)pUnicast->Address.lpSockaddr;
				inet_ntop(AF_INET, &sa_in->sin_addr, localIP, sizeof(localIP));

				oneInfo.allIP += localIP;
				oneInfo.allIP += ",";
			}

			pUnicast = pUnicast->Next;
		}

		if ("" != oneInfo.allIP) {
			oneInfo.allIP.pop_back();
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
	assert(pCurrAddresses->ReceiveLinkSpeed == pCurrAddresses->TransmitLinkSpeed); //bit unit
	oneInfo.MaxSpeedBps = pCurrAddresses->ReceiveLinkSpeed;

	return 0;
}

typedef std::unique_ptr<IP_ADAPTER_ADDRESSES, std::function<void(IP_ADAPTER_ADDRESSES*)> > hebAdapterBufHolder;

int hebGetOnlineInterfaces() {
	ULONG Iterations = 0;
	DWORD dwRetVal = 0;
	ULONG outBufCap = 200;
	const int tryTimes = 3;

	hebAdapterBufHolder adapterBufHolder;

	do {
		hebAdapterBufHolder outBufHolder((IP_ADAPTER_ADDRESSES*)malloc(outBufCap), [](IP_ADAPTER_ADDRESSES* ptr) {
			free(ptr);
			printf_s("delete outBufHolder in hebGetOnlineInterfaces\n");
		});

		//https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersaddresses
		dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, outBufHolder.get(), &outBufCap);
		if (NO_ERROR == dwRetVal) {
			adapterBufHolder = std::move(outBufHolder);
			break;
		}

		Iterations++;
	} while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < tryTimes));

	assert(adapterBufHolder);

	//获取网卡带宽
	PIP_ADAPTER_ADDRESSES pCurrAddresses = adapterBufHolder.get();
	gHebNet->interfaceCount = 0;
	HebInterfaceInfo* currInfo = gHebNet->interfaces;

	while (pCurrAddresses) {
		int ret = handleOneInterface(pCurrAddresses, *currInfo);
		if (0 == ret) {
			gHebNet->interfaceCount++;
			currInfo++;
		}
		pCurrAddresses = pCurrAddresses->Next;
	}

	//对于已经失效的网口，重置其ip字段
	for (int i = gHebNet->interfaceCount; i < sizeof(gHebNet->interfaces) / sizeof(gHebNet->interfaces[0]); i++) {
		gHebNet->interfaces[i].allIP = "";
	}

	return 0;
}

typedef std::unique_ptr<MIB_IFTABLE, std::function<void(MIB_IFTABLE*)> > hebIfTableBufHolder;

int32_t hebGetCurrentFlowData() {
	ULONG Iterations = 0;
	DWORD dwRetVal = 0;
	ULONG outBufCap = 200;
	const int tryTimes = 3;

	hebIfTableBufHolder ifTableBufHolder;

	do {
		hebIfTableBufHolder outBufHolder((MIB_IFTABLE*)malloc(outBufCap), [](MIB_IFTABLE* ptr) {
			free(ptr);
			printf_s("delete outBufHolder in hebGetCurrentFlowData\n");
		});

		//https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getiftable
		dwRetVal = GetIfTable(outBufHolder.get(), &outBufCap, TRUE);
		if (NO_ERROR == dwRetVal) {
			ifTableBufHolder = std::move(outBufHolder);
			break;
		}

		Iterations++;
	} while ((dwRetVal == ERROR_INSUFFICIENT_BUFFER) && (Iterations < tryTimes));

	assert(ifTableBufHolder);

	//获取到网卡状态，记录下当前时间。
	gHebNet->actionTime = std::chrono::system_clock::now();


	//统计全部网卡的流量统计信息
	char curMac[56];
	for (int i = 0; i != ifTableBufHolder->dwNumEntries; ++i)
	{
		MIB_IFROW &curInfo = ifTableBufHolder->table[i];

		if (IF_TYPE_SOFTWARE_LOOPBACK == curInfo.dwType) {
			continue;
		}
		if (curInfo.dwPhysAddrLen <= 0) {
			assert(0 == curInfo.dwInOctets || 0 == curInfo.dwOutOctets);
			continue;
		}

		//根据mac地址，找到状态上下文。
		hebMacToStr(curMac, sizeof(curMac), curInfo.bPhysAddr, curInfo.dwPhysAddrLen);
		for (int statusIndex = 0; statusIndex < gHebNet->interfaceCount; statusIndex++) {

			HebInterfaceInfo &statusInfo = gHebNet->interfaces[statusIndex];

			if (strcmp(curMac, statusInfo.macAddr) == 0) {
				statusInfo.dwNowTotalRecvByte = curInfo.dwInOctets;
				statusInfo.dwNowTotalSendByte = curInfo.dwOutOctets;
				statusInfo.bNowFlowDataValid = true;
				break;
			}

		}// end for iterator all device.
	}

	return 0;
}

int32_t hebGetByteDiff(DWORD now, DWORD last) {
	assert(4 == sizeof(DWORD));

	if (now >= last) {
		return now - last;
	}

	return MAXDWORD - last + 1 + now; //wrap around
}

std::string hebFormatSpeed(double dSpeedByte) {
	if (dSpeedByte < 0) {
		return "(-1)bps";
	}
	uint64_t uintSpeed = dSpeedByte * 8;
	if (uintSpeed > 1000) {
		uintSpeed += 1;
	}

	char t[16];
	UINT32 based = 1000;
	//UINT32 based = 1024;

	if (uintSpeed < based) {
		sprintf_s(t, sizeof(t), "%lubps", uintSpeed);
	}
	else if (uintSpeed < based * based) {
		double dSpeedBit = uintSpeed*1.0 / based;
		sprintf_s(t, sizeof(t), "%0.1fKbps", dSpeedBit);
	}
	else if (uintSpeed < based * based * based) {
		double dSpeedBit = uintSpeed*1.0 / based / based;
		sprintf_s(t, sizeof(t), "%0.2fMbps", dSpeedBit);
	}
	else if (uintSpeed < based * based * based * based) {
		double dSpeedBit = uintSpeed*1.0 / based / based / based;
		sprintf_s(t, sizeof(t), "%0.3fGbps", dSpeedBit);
	}
	else {
		assert(false);
	}

	return t;
}

INT32 hebGetSpeed(std::chrono::time_point<std::chrono::system_clock> &startTime, std::chrono::time_point<std::chrono::system_clock> &endTime,
	bool bUtf8, std::string &result) {
	if (endTime <= startTime) {
		return -1;
	}
	std::chrono::duration<double> duration = endTime - startTime;
	double diffTime = 1.0 /*duration.count()*/;

	result = "<?xml version=\"1.0\"?><root>";
	bool bHaveAdapter = false;
	char oneDeviceResult[512];
	char utf8Name[sizeof(gHebNet->interfaces[0].name)];

	for (int i = 0; i < gHebNet->interfaceCount; i++) {
		HebInterfaceInfo &statusInfo = gHebNet->interfaces[i];
		if (false == statusInfo.bLastFlowDataValid || false == statusInfo.bNowFlowDataValid) {
			continue;
		}

		int32_t diffRecvByte = hebGetByteDiff(statusInfo.dwNowTotalRecvByte, statusInfo.dwLastTotalRecvByte);
		int32_t diffSendByte = hebGetByteDiff(statusInfo.dwNowTotalSendByte, statusInfo.dwLastTotalSendByte);
		assert(diffRecvByte >= 0 && diffSendByte >= 0);

		double recvSpeedByte = diffRecvByte / diffTime;
		double sendSpeedByte = diffSendByte / diffTime;
		int64_t iRecvSpeedBit = recvSpeedByte * 8;
		int64_t iSendSpeedBit = sendSpeedByte * 8;

		//printf_s("adapter[%s]: recvSpeed=%s, sendSpeed=%s\n", statusInfo.name, hebFormatSpeed(recvSpeedByte).c_str(), hebFormatSpeed(sendSpeedByte).c_str());

		char* adapterName = statusInfo.name;
		if (bUtf8) {
			bool bRet = hebAnsi2Utf8(statusInfo.name, utf8Name, sizeof(utf8Name));
			assert(bRet);
			adapterName = utf8Name;
		}

		int len = sprintf_s(
			oneDeviceResult,
			sizeof(oneDeviceResult),
			"<adapter><name>%s</name><ip>%s</ip><maxSpeedBit>%lu</maxSpeedBit><recvSpeed><bit>%lld</bit><str>%s</str></recvSpeed><sendSpeed><bit>%lld</bit><str>%s</str></sendSpeed></adapter>",
			adapterName,
			statusInfo.allIP.c_str(),
			statusInfo.MaxSpeedBps,
			iRecvSpeedBit,
			hebFormatSpeed(recvSpeedByte).c_str(),
			iSendSpeedBit,
			hebFormatSpeed(sendSpeedByte).c_str()
		);
		assert(len + 5 < sizeof(oneDeviceResult));

		result += oneDeviceResult;
		bHaveAdapter = true;
	}

	result += "</root>";
	return 0;
}

int hebGetCurrentSystemTime() {
	return 0;
}

//接口函数
GETSYSTEMPERFORMANCE_API INT32 hebPerformance_init() {
	gHebNet = new HebAllStatusInfo();
	gHebNet->interfaceCount = 0;

	gHebCpu = new HebCpuStatusInfo();

	return 0;
}

GETSYSTEMPERFORMANCE_API INT32 hebPerformance_networkInfo(bool bUtf8, char* outBuf, int outBufCap) {
	if (nullptr == gHebNet) {
		return -1;
	}
	if (nullptr == outBuf || outBufCap < 20) {
		return -2;
	}
	hebGetOnlineInterfaces();

	//重置流量统计
	for (int i = 0; i < gHebNet->interfaceCount; i++) {
		HebInterfaceInfo &statusInfo = gHebNet->interfaces[i];

		statusInfo.bLastFlowDataValid = false;
		statusInfo.dwLastTotalRecvByte = 0;
		statusInfo.dwLastTotalSendByte = 0;

		statusInfo.bNowFlowDataValid = false;
		statusInfo.dwNowTotalRecvByte = 0;
		statusInfo.dwNowTotalSendByte = 0;
	}

	//统计流量
	hebGetCurrentFlowData();
	auto startTime = gHebNet->actionTime;

	for (int i = 0; i < gHebNet->interfaceCount; i++) {
		HebInterfaceInfo &statusInfo = gHebNet->interfaces[i];

		statusInfo.bLastFlowDataValid = statusInfo.bNowFlowDataValid;
		statusInfo.dwLastTotalRecvByte = statusInfo.dwNowTotalRecvByte;
		statusInfo.dwLastTotalSendByte = statusInfo.dwNowTotalSendByte;
	}

	::Sleep(1000); //休眠一秒

	//统计流量
	hebGetCurrentFlowData();
	auto endTime = gHebNet->actionTime;

	//计算速度
	std::string result = "";
	hebGetSpeed(startTime, endTime, bUtf8, result);

	int retLen = result.size();
	if (outBufCap <= retLen) { //include '\0'
		return -3;
	}
	strcpy_s(outBuf, outBufCap, result.c_str());

	std::string().swap(result);
	return retLen;
}

GETSYSTEMPERFORMANCE_API INT32 hebPerformance_memoryInfo(unsigned int* usedPercent) {
	if (nullptr == usedPercent) {
		return -1;
	}

	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	if (!GlobalMemoryStatusEx(&memInfo)) {
		return -2;
	}

	//[0,101)
	double availPercent = (memInfo.ullAvailPhys*1.0 / memInfo.ullTotalPhys)*100.0;
	int32_t used = 100 - availPercent;
	if (used < 0) {
		used = 100;
	}

	*usedPercent = used;
	return 0;
}

GETSYSTEMPERFORMANCE_API INT32 hebPerformance_cpuInfo(unsigned int* usedPercent) {
	if (nullptr == gHebCpu) {
		return -1;
	}
	if (nullptr == usedPercent) {
		return -2;
	}

	FILETIME ftSysIdle, ftSysKernel, ftSysUser;
	if (!GetSystemTimes(&ftSysIdle, &ftSysKernel, &ftSysUser)) {
		return -3;
	}



	return 0;
}

GETSYSTEMPERFORMANCE_API INT32 hebPerformance_test(char* msg, char* outMsg, INT32 outCap) {
	if (nullptr == msg || nullptr == outMsg || outCap < 100) {
		return -1;
	}
	return sprintf_s(outMsg, outCap, "hi, i am c++ performance dll, i have got your msg \"%s\"\n", msg);
}

