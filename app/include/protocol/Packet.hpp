#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

constexpr uint8_t  SOF_DATA_VALUE = 0xAA;
constexpr size_t   BKEL_SOF_SIZE  = 1;
constexpr size_t   BKEL_HDR_SIZE  = 4; // sid(1) + type(1) + dlc(2)
constexpr size_t   BKEL_CID_SIZE  = 2;
constexpr size_t   BKEL_CRC_SIZE  = 1;
constexpr size_t   BKEL_MAX_PAYLOAD = 256;

#pragma pack(push,1)
struct BKEL_Data_Frame_Header
{
    uint8_t  sid;
    uint8_t  type;
    uint16_t dlc;
};
#pragma pack(pop)

struct BKEL_Frame
{
    uint8_t  sid;
    uint8_t  type;
    uint16_t dlc;
    uint16_t cid;
    std::vector<uint8_t> payload;
};