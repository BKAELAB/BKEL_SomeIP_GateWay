#pragma once

#include <cstdint>

class UART
{
public:
    explicit UART(const char* device, int baudrate);
    ~UART();
    uint32_t writeData(const uint8_t* data, uint32_t len);
    uint32_t readData(uint8_t* buf, uint32_t len);

private:
    int fd;
};