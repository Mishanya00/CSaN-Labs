#pragma once

#include <string>
#include <winsock2.h>

class IcmpClient {
public:
    IcmpClient();

    ~IcmpClient();

    void SendPing(const std::string& targetIp);

private:
    SOCKET sock;
    WSADATA wsaData;

    struct IcmpHeader {
        uint8_t type;
        uint8_t code;
        uint16_t checksum;
        uint16_t id;
        uint16_t seq;
    };

    uint16_t CalculateChecksum(const void* data, size_t length);
};