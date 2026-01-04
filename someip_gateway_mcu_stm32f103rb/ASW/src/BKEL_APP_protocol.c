/*
 * BKEL_APP_Protocol.c
 *
 *  Created on: Jan 4, 2026
 *      Author: sjkang
 */

#include "BKEL_APP_protocol.h"

size_t build_frame(uint8_t *out_buf,
                 size_t   out_buf_size,
                 uint8_t  sid,
                 uint8_t  type,
                 const uint8_t *payload,
                 uint16_t payload_len)
{
    size_t total_len =
        sizeof(BKEL_Data_Frame_Header_t) +
        payload_len +
        sizeof(uint16_t) +  // CID
        sizeof(uint8_t);    // CRC

    if (out_buf_size < total_len)
        return 0;   // buffer overflow

    BKEL_Data_Frame_Header_t hdr;
    hdr.sid  = sid;
    hdr.type = type;
    hdr.dlc  = payload_len;

    size_t offset = 0;

    memcpy(out_buf + offset, &hdr, sizeof(hdr));
    offset += sizeof(hdr);

    memcpy(out_buf + offset, payload, payload_len);
    offset += payload_len;

    uint16_t cid = make_cid();
    memcpy(out_buf + offset, &cid, sizeof(cid));
    offset += sizeof(cid);

    uint8_t crc = calc_crc(out_buf, offset);
    out_buf[offset++] = crc;

    return offset;
}
