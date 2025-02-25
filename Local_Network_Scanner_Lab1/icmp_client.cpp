#include "icmp_client.hpp"

#include <iostream>
#include <ws2tcpip.h>
#include <chrono>
#include <stdexcept>

IcmpClient::IcmpClient() : sock(INVALID_SOCKET) {
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }

    sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Raw socket creation failed: " + std::to_string(WSAGetLastError()));
    }
}

IcmpClient::~IcmpClient() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }
    WSACleanup();
}

void IcmpClient::SendPing(const std::string& targetIp) {
    const int packetSize = sizeof(IcmpHeader) + 32;
    char packet[packetSize];
    memset(packet, 0, packetSize);

    IcmpHeader* icmpHeader = (IcmpHeader*)packet;
    icmpHeader->type = 8;
    icmpHeader->code = 0;
    icmpHeader->id = GetCurrentProcessId() & 0xFFFF;
    icmpHeader->seq = 1;
    icmpHeader->checksum = 0;

    icmpHeader->checksum = CalculateChecksum(packet, packetSize);

    sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    inet_pton(AF_INET, targetIp.c_str(), &destAddr.sin_addr);

    auto start = std::chrono::steady_clock::now();
    if (sendto(sock, packet, packetSize, 0, (sockaddr*)&destAddr, sizeof(destAddr)) == SOCKET_ERROR) {
        throw std::runtime_error("Send failed: " + std::to_string(WSAGetLastError()));
    }

    char recvBuffer[1024];
    sockaddr_in fromAddr;
    int fromAddrLen = sizeof(fromAddr);

    int bytesReceived = recvfrom(sock, recvBuffer, sizeof(recvBuffer), 0, (sockaddr*)&fromAddr, &fromAddrLen);
    if (bytesReceived == SOCKET_ERROR) {
        throw std::runtime_error("Receive failed: " + std::to_string(WSAGetLastError()));
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    IcmpHeader* replyHeader = (IcmpHeader*)(recvBuffer + 20);
    if (replyHeader->type == 0 && replyHeader->code == 0) { 
        std::cout << "Reply from " << targetIp << ": bytes=" << bytesReceived - 20
            << " time=" << elapsed.count() * 1000 << "ms\n";
    }
    else {
        std::cerr << "Unexpected ICMP reply\n";
    }
}

uint16_t IcmpClient::CalculateChecksum(const void* data, size_t length)
{
    const uint16_t* ptr = (const uint16_t*)data;
    uint32_t sum = 0;

    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }

    if (length == 1) {
        sum += *(const uint8_t*)ptr;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (uint16_t)(~sum);
}