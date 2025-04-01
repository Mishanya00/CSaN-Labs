#include "socket_server.hpp"

#include "safe_cmd.hpp"

#include <stdexcept>
#include <ws2tcpip.h>
#include <chrono>

#include <iostream>

#define DEFAULT_BUFLEN 512

BasicSocketServer::BasicSocketServer() : ListenSocket_(INVALID_SOCKET), ClientSocket_(INVALID_SOCKET), stop_(false)
{
}

BasicSocketServer::~BasicSocketServer()
{
    if (ListenSocket_ != INVALID_SOCKET) {
        closesocket(ListenSocket_);
    }
    if (ClientSocket_ != INVALID_SOCKET) {
        closesocket(ClientSocket_);
    }
}

void BasicSocketServer::Start(const std::string& ip, const std::string& port)
{
    server_thread_ = std::thread(&BasicSocketServer::Launch, this, ip, port);
}

void BasicSocketServer::Stop()
{
    stop_ = true;
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void BasicSocketServer::Launch(const std::string& serverName, const std::string& port)
{
    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult_ = getaddrinfo(NULL, port.c_str(), &hints, &result);
    if (iResult_ != 0) {
        throw std::runtime_error("getaddrinfo failed: " + std::to_string(iResult_));
        WSACleanup();
    }

    ListenSocket_ = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket_ == INVALID_SOCKET) {
        freeaddrinfo(result);
        WSACleanup();
        throw std::runtime_error("Socket creation failed: " + std::to_string(WSAGetLastError()));
    }

    iResult_ = bind(ListenSocket_, result->ai_addr, (int)result->ai_addrlen);
    if (iResult_ == SOCKET_ERROR) {
        freeaddrinfo(result);
        closesocket(ListenSocket_);
        WSACleanup();
        throw std::runtime_error("Bind failed: " + std::to_string(WSAGetLastError()));
    }

    freeaddrinfo(result);
    
    iResult_ = listen(ListenSocket_, SOMAXCONN);
    if (iResult_ == SOCKET_ERROR) {
        closesocket(ListenSocket_);
        WSACleanup();
        throw std::runtime_error("Listen failed: " + std::to_string(WSAGetLastError()));
    }

    // SafePrint("Listening on " + serverName + ":" + port + "..." + '\n');

    while (!stop_) {
        ClientSocket_ = accept(ListenSocket_, NULL, NULL);
        if (ClientSocket_ == INVALID_SOCKET) {
            closesocket(ListenSocket_);
            WSACleanup();
            throw std::runtime_error("Accept failed: " + std::to_string(WSAGetLastError()));
        }

        // SafePrint("Client connected to server" + '\n');

        char recvbuf[DEFAULT_BUFLEN];
        int recvbuflen = DEFAULT_BUFLEN;
        iResult_ = recv(ClientSocket_, recvbuf, recvbuflen, 0);
        if (iResult_ > 0) {
            //SafePrint("Server received message: " + std::string(recvbuf, iResult_) + '\n');

            std::cout << "\nYou received message: " + std::string(recvbuf, iResult_) + '\n';

            iResult_ = send(ClientSocket_, recvbuf, iResult_, 0);
            if (iResult_ == SOCKET_ERROR) {
                closesocket(ClientSocket_);
                throw std::runtime_error("Send failed: " + std::to_string(WSAGetLastError()));
            }
        }
        else if (iResult_ == 0) {
            // SafePrint("Connection closing..." + '\n');
        }
        else {
            closesocket(ClientSocket_);
            throw std::runtime_error("Recv failed: " + std::to_string(WSAGetLastError()));
        }

        closesocket(ClientSocket_);
        //SafePrint("Connection closed." + '\n');

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
