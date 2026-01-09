/*
 * BKEL_APP_Protocol.c
 *
 *  Created on: Jan 4, 2026
 *      Author: sjkang
 */

#include "BKEL_APP_protocol.h"
#include "BKEL_APP_crc.h"
#include <string.h>

// Defines
#define BKEL_HDR_SIZE 	4U
#define BKEL_MAX_DLC	500U
#define BKEL_CID_SIZE	2U
#define BKEL_CRC_SIZE	1U

// Proto
static void handle_frame(uint8_t sid, uint8_t type,const uint8_t *payload, uint16_t dlc,uint16_t cid);
static BKEL_PARSE_RESULT_e parse_one_frame(const uint8_t *buf,size_t buf_len,size_t *consumed);


/*
 * Brief : Message Frame Build
 * e.g. ) sid : 0x01 , type : 0x03 , payload : Service List ...
 *
 * <Result>
 * e.g. Frame : [sof][0x01][type][dlc][payload][cid][crc8]
 */


size_t build_frame(uint8_t *out_buf,
                 size_t   out_buf_size,
                 uint8_t  sid,
                 uint8_t  type,
                 const uint8_t *payload,
                 uint16_t payload_len)
{
    size_t total_len =
    	1 +	/* SOF */
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
    out_buf[offset++] = SOF_DATA_VALUE;

    memcpy(out_buf + offset, &hdr, sizeof(hdr));
    offset += sizeof(hdr);

    memcpy(out_buf + offset, payload, payload_len);
    offset += payload_len;

    uint16_t cid = make_cid();
    memcpy(out_buf + offset, &cid, sizeof(cid));
    offset += sizeof(cid);

    uint8_t crc = calc_crc8(out_buf + 1, offset - 1);
    out_buf[offset++] = crc;

    return offset;
}

void parse_packet(uint8_t *buf, size_t *len)
{
    size_t offset = 0;

    while (offset < *len)
    {
        size_t consumed = 0;

        BKEL_PARSE_RESULT_e r = parse_one_frame(
            buf + offset,
            *len - offset,
            &consumed
        );

        if (r == PARSE_INCOMPLETE)
            break;

        if (r == PARSE_INVALID)
        {
            offset += consumed;
            continue;
        }

        // PARSE_OK
        offset += consumed;
    }

    if (offset > 0)
    {
        memmove(buf, buf + offset, *len - offset);
        *len -= offset;
    }
}


static void handle_frame(uint8_t sid, uint8_t type,
                         const uint8_t *payload, uint16_t dlc,
                         uint16_t cid)
{
	// SID 기반 분기처리.

    bool is_valid_dlc = false;

    /* SID별 DLC 유효성 검사 */
    switch (sid) {
        case SERVICE_ADVERTISE:     // 0x01: Variable
        	if (dlc > 0 && dlc <= 16) is_valid_dlc = true;
				break;

        case RPC_LD2_CONTROL:       // 0x10: 1Byte
        case RPC_MCU_RESET:         // 0x11: 1Byte
        case DIAG_GPO_PINSTATE:     // 0x24: 1Byte
        case DIAG_GPI_PINSTATE:     // 0x25: 1Byte
        case DIAG_LD2_PINSTATE:     // 0x26: 1Byte
            if (dlc == 1) is_valid_dlc = true;
            break;

        case RPC_PWM_SETOUT:        // 0x13: 2Byte
        case DIAG_PWM_OUTPUT_VALUE: // 0x20: 2Byte
        case DIAG_PWM_INPUT_VALUE:  // 0x21: 2Byte
        case DIAG_ADC1_GET_VALUE:   // 0x22: 2Byte
        case DIAG_ADC2_GET_VALUE:   // 0x23: 2Byte
            if (dlc == 2) is_valid_dlc = true;
            break;

        case RPC_SPI_READ:          // 0x12: 5Byte
            if (dlc == 5) is_valid_dlc = true;
            break;

        default:
            /* 정의되지 않은 SID */
            is_valid_dlc = false;
            printf("SID: 0x%02x\n", sid);
            break;
    }

    /* 유효한 데이터 복사, Notify 전송 */
    if (is_valid_dlc) {
        /* 구조체 */
    	parsed_packet.sid  = sid;
    	parsed_packet.type = type;
    	parsed_packet.dlc  = dlc;
    	parsed_packet.cid  = cid;

        /* payload 복사(배열 크기 16 넘지 않도록) */
        uint16_t copy_len = (dlc > 16) ? 16 : dlc;
        if (payload != NULL && dlc > 0) {
            memcpy(parsed_packet.payload, payload, copy_len);
        }

        /* 32비트 포인터 */
        uint32_t packet_addr = (uint32_t)&parsed_packet;

        /* SID에 따라 해당 태스크 깨우기 */
//        if (sid == SERVICE_ADVERTISE) {
//			printf("Service Advertise Received: %s\n", parsed_packet.payload);
//        }

        if (sid >= 0x10 && sid <= 0x1F) {
            /* RPC */
            if (hRPCTask != NULL) {
                xTaskNotify(hRPCTask, packet_addr, eSetValueWithOverwrite);
            }
        }
        else if (sid >= 0x20 && sid <= 0x2F) {
            /* DIAG */
            if (hSendDataTask != NULL) {
                xTaskNotify(hSendDataTask, packet_addr, eSetValueWithOverwrite);
            }
        }
    }

    else {
        /* DLC가 일치하지 않거나 정의되지 않은 SID일 경우 로그 출력 */
        // printf("Invalid Frame: SID[0x%02X], DLC[%d]\n", sid, dlc);
    }
}


static BKEL_PARSE_RESULT_e parse_one_frame(const uint8_t *buf,
										   size_t buf_len,
										   size_t *consumed)
{
    *consumed = 0;

    if (buf[0] != SOF_DATA_VALUE) {
        *consumed = 1;
        return PARSE_INVALID;
    }

    if (buf_len < BKEL_HDR_SIZE)
        return PARSE_INCOMPLETE;

    BKEL_Data_Frame_Header_t hdr;
    memcpy(&hdr, buf, BKEL_HDR_SIZE);

    // dlc sanity check
    if (hdr.dlc > BKEL_MAX_DLC) {
        *consumed = 1;
        return PARSE_INVALID;
    }

    // Calc Full Frame Length
    size_t frame_len = BKEL_HDR_SIZE + (size_t)hdr.dlc + BKEL_CID_SIZE + BKEL_CRC_SIZE;
    if (buf_len < frame_len)
        return PARSE_INCOMPLETE;

    // CRC Check [HDR + PAYLOAD + CID]
    const uint8_t *payload = buf + BKEL_HDR_SIZE;
    const uint8_t *cid_ptr = payload + hdr.dlc;
    const uint8_t *crc_ptr = cid_ptr + BKEL_CID_SIZE;
    uint16_t cid;
    memcpy(&cid, cid_ptr, sizeof(cid));
    uint8_t expected = calc_crc8(buf + 1, BKEL_HDR_SIZE + (size_t)hdr.dlc + BKEL_CID_SIZE);
    uint8_t got      = *crc_ptr;

	// resync
    if (expected != got) {
        *consumed = 1;
        return PARSE_INVALID;
    }

    handle_frame(hdr.sid, hdr.type, payload, hdr.dlc, cid);

    *consumed = frame_len;
    return PARSE_OK;
}

