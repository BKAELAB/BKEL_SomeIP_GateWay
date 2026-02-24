#include <protocol/PacketEncoder.hpp>
#include "main.h"

std::vector<uint8_t> PacketEncoder::build_frame(uint8_t sid,
                                                uint8_t type,
                                                const uint8_t* payload,
                                                uint16_t payload_len,
                                                uint16_t cid)
{
    // sanity check
    if (payload_len > BKEL_MAX_PAYLOAD) return {};
    if (payload_len > 0 && payload == nullptr) return {};

    // SOF(1) + HDR(4) + PAYLOAD(N) + CID(2) + CRC(1)
    const size_t totalLen =
        BKEL_SOF_SIZE + BKEL_HDR_SIZE + payload_len + BKEL_CID_SIZE + BKEL_CRC_SIZE;

    std::vector<uint8_t> out;
    out.reserve(totalLen);

    // SOF
    out.push_back(SOF_DATA_VALUE); // 0xAA

    // Header: SID(1), TYPE(1), DLC(2)
    out.push_back(sid);
    out.push_back(type);
    out.push_back(static_cast<uint8_t>(payload_len & 0xFF));
    out.push_back(static_cast<uint8_t>((payload_len >> 8) & 0xFF));

    // Payload
    if (payload_len > 0) {
        out.insert(out.end(), payload, payload + payload_len);
    }

    // CID(2, Little Endian)
    out.push_back(static_cast<uint8_t>(cid & 0xFF));
    out.push_back(static_cast<uint8_t>((cid >> 8) & 0xFF));

    // CRC (SOF 제외: HDR + PAYLOAD + CID)
    const uint8_t crc = calc_crc8(out.data() + BKEL_SOF_SIZE,
                                  BKEL_HDR_SIZE + payload_len + BKEL_CID_SIZE);
    out.push_back(crc);

    return out;
}