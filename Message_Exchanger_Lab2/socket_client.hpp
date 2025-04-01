#include <string>
#include <WinSock2.h>
#include <thread>
#include <atomic>

#include <unordered_map>

class BasicSocketClient {
public:

    BasicSocketClient(std::string server_ip, std::string server_port, std::unordered_map<std::string, std::string> ipSubnetMap);

    ~BasicSocketClient();

    void Start();

    void Stop();

    void Connect(std::string& serverName, std::string& port);
    void SendData(const std::string& data);
    std::string ReceiveData();

    void ShutdownConnection();

    bool isActive = true;

protected:

    void Run();

private:
    static constexpr int BUFFER_SIZE = 1028;

    SOCKET ConnectSocket_;

    int iResult_;

    std::thread client_thread_;
    std::atomic<bool> stop_;

    std::string server_ip_;
    std::string server_port_;
    std::unordered_map<std::string, std::string> ip_subnet_map_;
};