#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "basic_socket_client.hpp"
#include "icmp_client.hpp"


int BasicSocketClientTest()
{
    try {
        BasicSocketClient client;

        std::string serverAddress = "127.0.0.1";
        std::string port = "12345";
        client.Connect(serverAddress, port);

        std::string message = "Hello, Local Server!";
        client.SendData(message);
        std::cout << "Sent: " << message << std::endl;

        std::string response = client.ReceiveData();
        std::cout << "Received: " << response << std::endl;

        client.ShutdownConnection();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

void Ping(const std::string& targetIp) {
    try {
        IcmpClient icmpClient;
        icmpClient.SendPing(targetIp);
    }
    catch (const std::exception& e) {
        std::cerr << "Error in thread: " << e.what() << std::endl;
    }
}

// 192.168.1.x

int main()
{
    std::vector<std::thread> threads;
    std::string ip_addr;

    for (int i = 0; i <= 255; ++i) {
        ip_addr = "192.168.1." + std::to_string(i);
        threads.emplace_back(Ping, ip_addr);
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}