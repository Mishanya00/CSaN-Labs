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

    uint16_t CalculateChecksum(const void* data, size_t length) {
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
};