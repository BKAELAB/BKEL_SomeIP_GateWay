#include <protocol/PacketParser.hpp>
#include "main.h"

void PacketParser::push(const uint8_t* data, size_t len)
{
    rxBuffer.insert(rxBuffer.end(), data, data + len);
    parse();
}

void PacketParser::parse()
{
    while (true)
    {
        if (rxBuffer.size() < BKEL_SOF_SIZE + BKEL_HDR_SIZE)
            return;

        /* SOF 탐색 */
        if (rxBuffer[0] != SOF_DATA_VALUE)
        {
            rxBuffer.erase(rxBuffer.begin());
            continue;
        }

        BKEL_Data_Frame_Header hdr;
        std::memcpy(&hdr,
                    rxBuffer.data() + BKEL_SOF_SIZE,
                    BKEL_HDR_SIZE);

        if (hdr.dlc > BKEL_MAX_PAYLOAD)
        {
            rxBuffer.erase(rxBuffer.begin());
            continue;
        }

        size_t frameLen =
            BKEL_SOF_SIZE +
            BKEL_HDR_SIZE +
            hdr.dlc +
            BKEL_CID_SIZE +
            BKEL_CRC_SIZE;

        if (rxBuffer.size() < frameLen)
            return;

        const uint8_t* payload =
            rxBuffer.data() + BKEL_SOF_SIZE + BKEL_HDR_SIZE;

        const uint8_t* cidPtr = payload + hdr.dlc;
        const uint8_t* crcPtr = cidPtr + BKEL_CID_SIZE;

        uint16_t cid;
        std::memcpy(&cid, cidPtr, sizeof(cid));

        uint8_t expectedCrc =
            calc_crc8(rxBuffer.data() + BKEL_SOF_SIZE,
                        BKEL_HDR_SIZE + hdr.dlc + BKEL_CID_SIZE);

        if (expectedCrc != *crcPtr)
        {
            rxBuffer.erase(rxBuffer.begin());
            continue;
        }

        /* === 정상 프레임 === */
        BKEL_Frame frame;
        frame.sid  = hdr.sid;
        frame.type = hdr.type;
        frame.dlc  = hdr.dlc;
        frame.cid  = cid;
        frame.payload.assign(payload, payload + hdr.dlc);

        onFrame(frame);

        rxBuffer.erase(rxBuffer.begin(),
                        rxBuffer.begin() + frameLen);
    }
}

void PacketParser::onFrame(const BKEL_Frame& frame)
{
    // 지금은 로그, 나중에 RPC/DIAG 분기
    printf("[BKEL FRAME]\n");
    printf(" SID : 0x%02X\n", frame.sid);
    printf(" TYPE: 0x%02X\n", frame.type);
    printf(" DLC : %u\n", frame.dlc);
    printf(" CID : %u\n", frame.cid);

    if (!frame.payload.empty())
    {
        printf(" PAYLOAD: ");
        for (auto b : frame.payload)
            printf("%02X ", b);
        printf("\n");
    }
}