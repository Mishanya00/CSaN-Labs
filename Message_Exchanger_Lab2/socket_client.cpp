#include "socket_client.hpp"

#include <stdexcept>
#include <ws2tcpip.h>
#include "safe_cmd.hpp"
#include <iostream>

BasicSocketClient::BasicSocketClient(std::string server_ip, std::string server_port, std::unordered_map<std::string, std::string> ipSubnetMap) : 
                    stop_(false), ConnectSocket_(INVALID_SOCKET), server_ip_(server_ip), server_port_(server_port),
                    ip_subnet_map_(ipSubnetMap)
{ }

BasicSocketClient::~BasicSocketClient() {
    if (ConnectSocket_ != INVALID_SOCKET) {
        closesocket(ConnectSocket_);
    }
}

void BasicSocketClient::Run()
{
    std::string receiver_ip; // = server_ip_; //"127.0.0.1";
    std::string port; //="12345";
    std::string menu;
    bool isSelect = true;

    std::string message;

    while (!stop_)
    {
        if (isSelect) {
           
            /*
            do {
                SafePrint("IP to message: ");
                receiver_ip = SafeInput();
            } while (ip_subnet_map_.find(receiver_ip) == ip_subnet_map_.end());
            */

            std::cout << "IP to message: ";
            std::cin >> receiver_ip;

            std::cout << "PORT to message: ";
            std::cin >> port;
            isSelect = false;
        }
        this->Connect(receiver_ip, port);

        menu = "---------------------------------\nIP: " + receiver_ip + " port: " + port + '\n';
        menu += "1. Send message\n2. Change receiver's IP & port\n";
        menu += "---------------------------------\n";
        SafePrint(menu);
        
        message = SafeInput();

        if (message == "1") {
            SafePrint("Your message: ");
            message = SafeInput();

            this->SendData(server_ip_ + " " + server_port_ + " " + message);
            SafePrint("Sent: " + message + '\n');
        }
        else if (message == "2") {
            isSelect = true;
        }

        // std::string response = this->ReceiveData();
        // SafePrint("Received: " + response + '\n');

        this->ShutdownConnection(); 
    }
}

void BasicSocketClient::Start()
{
    client_thread_ = std::thread(&BasicSocketClient::Run, this);
}

void BasicSocketClient::Stop()
{
    stop_ = true;
    if (client_thread_.joinable()) {
        client_thread_.join();
    }
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