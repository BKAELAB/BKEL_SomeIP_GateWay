#pragma once

#include <vector>
#include <cstdint>

class ITransport {
public:
    virtual ~ITransport() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void sendData(const std::vector<uint8_t>& data) = 0;
    virtual uint16_t getCid() const = 0;
};
