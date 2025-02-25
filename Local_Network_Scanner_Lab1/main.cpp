#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include <iostream>
#include <string>
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

int main()
{
    try {
        IcmpClient icmpClient;
        icmpClient.SendPing("192.168.1.4"); // Send ICMP ping to 192.168.1.4
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}