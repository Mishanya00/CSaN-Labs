#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include <iostream>
#include <string>

#include <thread>
#include <chrono>

#include <ws2tcpip.h>

#include "socket_client.hpp"
#include "socket_server.hpp"
#include "mishanya_utils.hpp"

std::string GetLocalIPAddress() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        std::cerr << "Error getting hostname: " << WSAGetLastError() << "\n";
        return "";
    }

    addrinfo hints = {};
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0) {
        std::cerr << "Error resolving hostname: " << WSAGetLastError() << "\n";
        return "";
    }

    std::string ip;
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        sockaddr_in* sockaddr_ipv4 = reinterpret_cast<sockaddr_in*>(ptr->ai_addr);
        char ipStr[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ipStr, sizeof(ipStr)) != nullptr) {
            ip = ipStr;
            if (ip.substr(0, 3) != "127") { // Skip loopback address
                break;
            }
        }
    }

    freeaddrinfo(result);
    return ip;
}


int main()
{
    WSADATA wsaData;
    int iResult_ = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult_ != 0) {
        throw std::runtime_error("WSAStartup failed: " + std::to_string(iResult_));
    }

    std::string server_ip = "127.0.0.1";
    std::string server_port = "12345";

    BasicSocketServer server;
    server.Start(server_ip, server_port);

    BasicSocketClient client(server_ip, server_port);
    client.Start();
   
    while (client.isActive)
    { }

    client.Stop();
    server.Stop();
    WSACleanup();

    return 0;
}