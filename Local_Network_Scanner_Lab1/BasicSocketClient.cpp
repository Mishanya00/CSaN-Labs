#include "BasicSocketClient.hpp"

#include <stdexcept>
#include <ws2tcpip.h>

BasicSocketClient::BasicSocketClient() : ConnectSocket_(INVALID_SOCKET) {
    iResult_ = WSAStartup(MAKEWORD(2, 2), &wsaData_);
    if (iResult_ != 0) {
        throw std::runtime_error("WSAStartup failed: " + std::to_string(iResult_));
    }
}

BasicSocketClient::~BasicSocketClient() {
    if (ConnectSocket_ != INVALID_SOCKET) {
        closesocket(ConnectSocket_);
    }
    WSACleanup();
}

void BasicSocketClient::Connect(std::string& serverName, std::string& port) {
    struct addrinfo* result = nullptr;
    struct addrinfo* ptr = nullptr;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult_ = getaddrinfo(serverName.c_str(), port.c_str(), &hints, &result);
    if (iResult_ != 0) {
        throw std::runtime_error("getaddrinfo failed: " + std::to_string(iResult_));
    }

    for (ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        ConnectSocket_ = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket_ == INVALID_SOCKET) {
            throw std::runtime_error("socket failed: " + std::to_string(WSAGetLastError()));
        }

        iResult_ = connect(ConnectSocket_, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult_ == SOCKET_ERROR) {
            closesocket(ConnectSocket_);
            ConnectSocket_ = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket_ == INVALID_SOCKET) {
        throw std::runtime_error("Unable to connect to server");
    }
}

void BasicSocketClient::SendData(const std::string& data) {
    iResult_ = send(ConnectSocket_, data.c_str(), (int)data.length(), 0);
    if (iResult_ == SOCKET_ERROR) {
        throw std::runtime_error("send failed: " + std::to_string(WSAGetLastError()));
    }
}

std::string BasicSocketClient::ReceiveData() {
    char buffer[BUFFER_SIZE];
    std::string receivedData;
    int bytesReceived;

    while ((bytesReceived = recv(ConnectSocket_, buffer, sizeof(buffer), 0)) > 0) {
        receivedData.append(buffer, bytesReceived);
    }

    if (bytesReceived == SOCKET_ERROR) {
        throw std::runtime_error("recv failed: " + std::to_string(WSAGetLastError()));
    }

    return receivedData;
}

void BasicSocketClient::ShutdownConnection() {
    iResult_ = shutdown(ConnectSocket_, SD_SEND);
    if (iResult_ == SOCKET_ERROR) {
        throw std::runtime_error("shutdown failed: " + std::to_string(WSAGetLastError()));
    }
}