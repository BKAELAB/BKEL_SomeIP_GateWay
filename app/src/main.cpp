#include "main.h"

int main(int argc, char* argv[])
{
    UART uart("/dev/serial0", B115200);
    PacketParser parser;

    uint8_t txBuf[128];
    uint8_t rxBuf[128];

    while (true)
    {
        ssize_t n = uart.readData(rxBuf, sizeof(rxBuf));
        if (n > 0)
        {
            printf("[RAW RX %zd] ", n);
            for (ssize_t i = 0; i < n; i++)
                printf("%02X ", rxBuf[i]);
            printf("\n");
        }
    }
    return 0;
}