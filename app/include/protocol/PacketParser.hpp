#pragma once

#include <protocol/Packet.hpp>
#include <protocol/PacketEncoder.hpp>
#include <functional>
#include <vector>

#include <mutex>

class PacketParser
{
public:
    void push(const uint8_t* data, size_t len);

    void setCallback(std::function<void(const BKEL_Frame&)> cb) {
        callback = cb;
    }

private:
    std::vector<uint8_t> rxBuffer;
    std::function<void(const BKEL_Frame&)> callback;
    std::mutex mtx;

    void parse();
    void onFrame(const BKEL_Frame& frame);
};
