#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma comment(lib, "IPHLPAPI.lib")

#include <iostream>
#include <string>
#include <unordered_map>

#include <thread>
#include <chrono>

#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#include "socket_client.hpp"
#include "socket_server.hpp"
#include "mishanya_utils.hpp"


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


bool isPortAvailable(const std::string& ipAddress, const std::string& port) {
	WSADATA wsaData;
	SOCKET testSocket = INVALID_SOCKET;
	struct addrinfo* result = NULL;
	struct addrinfo hints;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
		return false;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(ipAddress.c_str(), port.c_str(), &hints, &result) != 0) {
		std::cerr << "getaddrinfo failed: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return false;
	}

	testSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (testSocket == INVALID_SOCKET) {
		std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	if (bind(testSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
		int error = WSAGetLastError();
		if (error == WSAEADDRINUSE) {
			std::cerr << "Port " << port << " is already in use" << std::endl;
		}
		else {
			std::cerr << "bind failed: " << error << std::endl;
		}
		closesocket(testSocket);
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	closesocket(testSocket);
	freeaddrinfo(result);
	WSACleanup();
	return true;
}


int main()
{
    WSADATA wsaData;
    int iResult_ = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult_ != 0) {
        throw std::runtime_error("WSAStartup failed: " + std::to_string(iResult_));
    }

	std::unordered_map<std::string, std::string> ipSubnetMap;
	std::string server_ip;
	std::string server_port;

	std::cout << " - - - - - - - - - - AVAILABLE IPS - - - - - - - - - - \n";

	GetInterfaces(4, ipSubnetMap);

	std::cout << " - - - - - - - - - - - - - - - - - - - - - - - - - - -\n";

	std::cout << "Type server's IP: ";
	std::cin >> server_ip;

	if (ipSubnetMap.find(server_ip) != ipSubnetMap.end()) {
		std::cout << "Option selected:\n";
		std::cout << "IP: " << server_ip << '\n';
		std::cout << "Subnet mask: " << ipSubnetMap[server_ip] << '\n';
	}
	else {
		std::cout << "Non-existing interface selected!\n";
		return 1;
	}

	std::cout << "Type server's port: ";
	std::cin >> server_port;

	if (isPortAvailable(server_ip, server_port))
	{
		std::cout << "GOOD!\n";
		BasicSocketServer server;
		server.Start(server_ip, server_port);

		BasicSocketClient client(server_ip, server_port, ipSubnetMap);
		client.Start();

		while (client.isActive)
		{ }
	}
	else {
		std::cout << "IP:PORT is busy!\n";
		char c;
		std::cin >> c;
		return 1;
	}

	while (true)
	{ }


    WSACleanup();

    return 0;
}

//std::string server_ip = "127.0.0.1";
//std::string server_port = "12345";

//BasicSocketServer server;
//server.Start(server_ip, server_port);

//BasicSocketClient client(server_ip, server_port);
//client.Start();

//while (client.isActive)
//{ }

//client.Stop();
//server.Stop();