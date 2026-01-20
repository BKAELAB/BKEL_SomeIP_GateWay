#pragma once

#include <protocol/Packet.hpp>
#include <protocol/PacketEncoder.hpp>

class PacketParser
{
public:
    void push(const uint8_t* data, size_t len);

private:
    std::vector<uint8_t> rxBuffer;

    void parse();
    void onFrame(const BKEL_Frame& frame);
};
