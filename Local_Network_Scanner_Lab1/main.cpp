#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

#include <thread>
#include <chrono>
#include <mutex>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>

//#pragma comment (lib, "Mswsock.lib")
//#pragma comment (lib, "AdvApi32.lib")

#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")

std::mutex mtx;

void GetMacAddress(const char* ipAddress) {
	PMIB_IPNETTABLE pIpNetTable = nullptr;
	DWORD dwSize = 0;
	DWORD dwRetVal = 0;

	// So actually this first call is for retrieving the size of ARP table
	dwRetVal = GetIpNetTable(pIpNetTable, &dwSize, FALSE);
	if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
		pIpNetTable = (PMIB_IPNETTABLE)malloc(dwSize);
		if (pIpNetTable == nullptr) {
			std::cout << "Memory allocation failed for ARP table.\n";
			return;
		}
	}

	dwRetVal = GetIpNetTable(pIpNetTable, &dwSize, FALSE);
	if (dwRetVal != NO_ERROR) {
		std::cout << "GetIpNetTable failed with error: " << dwRetVal << '\n';
		free(pIpNetTable);
		return;
	}

	in_addr addr;
	inet_pton(AF_INET, ipAddress, &addr);

	if (pIpNetTable != NULL)
	{
		for (DWORD i = 0; i < pIpNetTable->dwNumEntries; i++) {
			if (pIpNetTable->table[i].dwAddr == addr.s_addr) {
				BYTE* macAddr = pIpNetTable->table[i].bPhysAddr;
				printf("Responded! MAC: %02X-%02X-%02X-%02X-%02X-%02X\n",
					ipAddress, macAddr[0], macAddr[1], macAddr[2],
					macAddr[3], macAddr[4], macAddr[5]);
				free(pIpNetTable);
				return;
			}
		}
	}

	std::cout << "MAC address for IP " << ipAddress << " not found in ARP table.\n";
	free(pIpNetTable);
}

int PingIpAddr(std::string ip_to_ping)
{
	in_addr addr;
	HANDLE hIcmpFile;
	int iResult;

	char* addr_s = new char[ip_to_ping.size() + 1];
	for (int i = 0; i < ip_to_ping.size(); ++i) {
		addr_s[i] = ip_to_ping[i];
	}
	addr_s[ip_to_ping.size()] = '\0';

	DWORD dwRetVal = 0;
	char SendData[32] = "Data Buffer";
	LPVOID ReplyBuffer = NULL;
	DWORD ReplySize = 0;

	hIcmpFile = IcmpCreateFile();
	if (hIcmpFile == INVALID_HANDLE_VALUE) {
		std::cout << "Icmp File cannot be created!\n";
		std::cout << "Icmp creation error: " << GetLastError() << '\n';
		IcmpCloseHandle(hIcmpFile);
		return 1;
	}

	iResult = inet_pton(AF_INET, addr_s, &addr);
	if (iResult == -1) {
		std::cerr << "Invalid IP address format!\n";
		IcmpCloseHandle(hIcmpFile);
		return 1;
	}

	ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
	ReplyBuffer = (void*)malloc(ReplySize);
	if (ReplyBuffer == nullptr) {
		std::cout << "allocation error!\n";
		IcmpCloseHandle(hIcmpFile);
		return 1;
	}

	dwRetVal = IcmpSendEcho(hIcmpFile, addr.s_addr, SendData, sizeof(SendData),
		NULL, ReplyBuffer, ReplySize, 1000);

	if (dwRetVal > 0) {
		PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
		struct in_addr ReplyAddr;
		ReplyAddr.s_addr = pEchoReply->Address;

		if (pEchoReply->Status == 0) {
			mtx.lock();
			std::cout << "\nResponded! IP:  " << addr_s << "\n";
			GetMacAddress(addr_s);
			std::cout << "Roundtrip time = " << pEchoReply->RoundTripTime << " milliseconds\n";
			mtx.unlock();
		}
		else {
			std::cout << "IP: " << addr_s << "is unreachable\n";
		}
	}
	else {
		IcmpCloseHandle(hIcmpFile);
		//std::cout << "No device with this IP address\n";
		//std::cout << "IcmpSendEcho returned error: " << GetLastError() << '\n';
		return 1;
	}

	IcmpCloseHandle(hIcmpFile);
}

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

int GetInterfaces(int addr_family, std::unordered_map<std::string, std::string>& ipSubnetMap) {
	DWORD dwRetVal = 0;
	ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
	ULONG family = AF_UNSPEC;

	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	ULONG outBufLen = WORKING_BUFFER_SIZE;
	ULONG Iterations = 0;

	PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
	PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;

	if (addr_family == 4)
		family = AF_INET;
	else if (addr_family == 6)
		family = AF_INET6;

	do {
		pAddresses = (IP_ADAPTER_ADDRESSES*)MALLOC(outBufLen);
		if (pAddresses == NULL) {
			std::cerr << "Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n";
			return 1;
		}

		dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

		if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
			FREE(pAddresses);
			pAddresses = NULL;
		}
		else {
			break;
		}

		Iterations++;
	} while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

	if (dwRetVal == NO_ERROR) {
		pCurrAddresses = pAddresses;
		while (pCurrAddresses) {

			std::wcout << L"Interface Name: " << pCurrAddresses->FriendlyName << std::endl;

			pUnicast = pCurrAddresses->FirstUnicastAddress;
			while (pUnicast) {
				SOCKADDR* addr = pUnicast->Address.lpSockaddr;
				if (addr->sa_family == AF_INET) {
					char ipStr[INET_ADDRSTRLEN] = { 0 };
					char subnetMask[INET_ADDRSTRLEN] = { 0 };

					sockaddr_in* ipv4 = (sockaddr_in*)addr;
					inet_ntop(AF_INET, &ipv4->sin_addr, ipStr, sizeof(ipStr));

					ULONG prefixLength = pUnicast->OnLinkPrefixLength;
					ULONG mask = htonl(~((1 << (32 - prefixLength)) - 1));
					inet_ntop(AF_INET, &mask, subnetMask, sizeof(subnetMask));

					std::cout << "IPv4 Address: " << ipStr << std::endl;
					std::cout << "Subnet Mask: " << subnetMask << std::endl;

					ipSubnetMap[ipStr] = subnetMask;
				}
				pUnicast = pUnicast->Next;
			}
			std::cout << std::endl;

			pCurrAddresses = pCurrAddresses->Next;
		}
	}
	else {
		std::cerr << "Call to GetAdaptersAddresses failed with error: " << dwRetVal << std::endl;
		if (dwRetVal == ERROR_NO_DATA) {
			std::cerr << "\tNo addresses were found for the requested parameters\n";
		}
	}

	if (pAddresses) {
		FREE(pAddresses);
	}

	return 0;
}

int main()
{
	std::vector<std::thread> threads;
	std::unordered_map<std::string, std::string> ipSubnetMap;
	std::string ip_addr;

	GetInterfaces(4, ipSubnetMap);

	std::cout << "Type IP to ping: ";
	std::cin >> ip_addr;
	if (ipSubnetMap.find(ip_addr) != ipSubnetMap.end()) {
		std::cout << "Option selected:\n";
		std::cout << "IP: " << ip_addr << '\n';
		std::cout << "Subnet mask: " << ipSubnetMap[ip_addr] << '\n';
	}
	else {
		std::cout << "Non-existing interface selected!\n";
		return 1;
	}

	in_addr ip, mask;
	inet_pton(AF_INET, ip_addr.c_str(), &ip);
	inet_pton(AF_INET, ipSubnetMap[ip_addr].c_str(), &mask);

	in_addr networkAddress;
	char networkStr[INET_ADDRSTRLEN] = { 0 };
	networkAddress.s_addr = ip.s_addr & mask.s_addr;

	in_addr broadcastAddress;
	char broadcastStr[INET_ADDRSTRLEN] = { 0 };
	broadcastAddress.s_addr = networkAddress.s_addr | ~mask.s_addr;

	inet_ntop(AF_INET, &networkAddress, networkStr, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &broadcastAddress, broadcastStr, INET_ADDRSTRLEN);

	std::cout << "IP Range: " << networkStr << " - " << broadcastStr << '\n';

	while (networkAddress.s_addr != broadcastAddress.s_addr)
	{
		threads.emplace_back(PingIpAddr, std::string(networkStr));
		uint32_t temp = ntohl(networkAddress.s_addr);
		temp++;
		networkAddress.s_addr = htonl(temp);
		inet_ntop(AF_INET, &networkAddress, networkStr, INET_ADDRSTRLEN);
	}

	for (auto& t : threads) {
		t.join();
	}

	return 0;
}