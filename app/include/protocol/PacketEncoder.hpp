#pragma once

#include <protocol/Packet.hpp>
#include <protocol/PacketParser.hpp>

class PacketEncoder
{
public:
    static std::vector<uint8_t> build_frame(uint8_t sid,
                                            uint8_t type,
                                            const uint8_t* payload,
                                            uint16_t payload_len,
                                            uint16_t cid);
};

inline uint8_t calc_crc8(const uint8_t* data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
    }
    return crc;
}