#include <string>
#include <WinSock2.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

class BasicSocketClient {
public:

    BasicSocketClient();

    ~BasicSocketClient();

    void Connect(std::string& serverName, std::string& port);
    void SendData(const std::string& data);
    std::string ReceiveData();

    void ShutdownConnection();

private:
    static constexpr int BUFFER_SIZE = 1028;
    WSADATA wsaData_;
    SOCKET ConnectSocket_;
    int iResult_;
};