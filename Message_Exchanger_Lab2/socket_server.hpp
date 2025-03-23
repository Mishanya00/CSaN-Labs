#include <string>
#include <WinSock2.h>
#include <thread>
#include <atomic>

class BasicSocketServer {
public:

    BasicSocketServer();

    ~BasicSocketServer();

    void Start(const std::string& ip, const std::string& port);

    void Stop();

protected:

    void Launch(const std::string& serverName, const std::string& port);

private:
    static constexpr int BUFFER_SIZE = 1028;
    
    SOCKET ListenSocket_ = INVALID_SOCKET;
    SOCKET ClientSocket_ = INVALID_SOCKET;

    std::thread server_thread_;
    std::atomic<bool> stop_;

    int iResult_;
};