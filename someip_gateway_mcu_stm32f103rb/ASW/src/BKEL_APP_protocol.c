/*
 * BKEL_APP_Protocol.c
 *
 *  Created on: Jan 4, 2026
 *      Author: sjkang
 */

#include "BKEL_APP_protocol.h"
#include "BKEL_APP_crc.h"
#include "BKEL_APP_cid.h"
#include "BKEL_externs.h"
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

    uint8_t is_valid_dlc = 0;

    /* SID별 DLC 유효성 검사 */
    switch (sid) {
        case SERVICE_ADVERTISE:     // 0x01: Variable
        	if (dlc > 0 && dlc <= 16) is_valid_dlc = 1;
				break;

        case RPC_LD2_CONTROL:       // 0x10: 1Byte
        case RPC_MCU_RESET:         // 0x11: 1Byte
        case DIAG_GPO_PINSTATE:     // 0x24: 1Byte
        case DIAG_GPI_PINSTATE:     // 0x25: 1Byte
        case DIAG_LD2_PINSTATE:     // 0x26: 1Byte
            if (dlc == 1) is_valid_dlc = 1;
            break;

        case RPC_PWM_SETOUT:        // 0x13: 2Byte
        case DIAG_PWM_OUTPUT_VALUE: // 0x20: 2Byte
        case DIAG_PWM_INPUT_VALUE:  // 0x21: 2Byte
        case DIAG_ADC1_GET_VALUE:   // 0x22: 2Byte
        case DIAG_ADC2_GET_VALUE:   // 0x23: 2Byte
            if (dlc == 2) is_valid_dlc = 1;
            break;

        case RPC_SPI_READ:          // 0x12: 5Byte
            if (dlc == 5) is_valid_dlc = 1;
            break;

        default:
            /* 정의되지 않은 SID */
            is_valid_dlc = 0;
            printf("[handle_frame_error] Undefined SID: 0x%02x\r\n", sid);
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
        uint32_t *packet_addr = (uint32_t *)&parsed_packet;

        /* SID에 따라 해당 태스크 깨우기 */
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

void handle_frame_Test(void)
{
    printf("\r\n--- [Handle Frame Test] Start ---\r\n");

    /* 초기화 */
    memset(&parsed_packet, 0, sizeof(BKEL_Common_Packet_t));

    /* Case 1: 정상 데이터 (SID 0x13, DLC 2) */
    uint8_t normal_payload[2] = {50, 100};
    handle_frame(0x13, 0x01, normal_payload, 2, 123);	//sid, type, payload, dlc, cid

    printf("[Case 1 - Valid DLC Test] Result -> SID: 0x%02X, Payload[0]: %d, Payload[1]: %d\r\n",
            parsed_packet.sid, parsed_packet.payload[0], parsed_packet.payload[1]);
    if (parsed_packet.sid == 0x13 && parsed_packet.payload[0] == 50) {
            printf("[Case 1] Success: PWM 0x13 handled correctly.\r\n");
	} else {
		printf("[Case 1] Failed: Data mismatch.\r\n");

	}

    /* Case 2: 비정상 데이터 (SID 0x13, DLC 3) */
    memset(&parsed_packet, 0, sizeof(BKEL_Common_Packet_t));
    handle_frame(0x13, 0x01, normal_payload, 3, 123);	//sid, type, payload, dlc, cid

    printf("[Case 2 - Invalid DLC Test] Result -> SID: 0x%02X, Payload[0]: %d, Payload[1]: %d\r\n",
		   parsed_packet.sid, parsed_packet.payload[0], parsed_packet.payload[1]);
    if (parsed_packet.sid == 0 && parsed_packet.payload[0] == 0) {
		   printf("[Case 2] Success: PWM 0x13 handled correctly.\r\n");
    } else {
   		printf("[Case 2] Failed: Data mismatch.\r\n");
    }

	/* Case 3: 비정상 데이터(DLC 0) */
    memset(&parsed_packet, 0, sizeof(BKEL_Common_Packet_t));
	handle_frame(0x13, 0x01, normal_payload, 0, 123);

	printf("[Case 3 - Zero DLC Test]] Result -> SID: 0x%02X, Payload[0]: %d, Payload[1]: %d\r\n",
	            parsed_packet.sid, parsed_packet.payload[0], parsed_packet.payload[1]);

	if (parsed_packet.sid == 0) {
		printf("[Case 3 - Zero DLC Test] Success: Zero length data was blocked correctly.\r\n");
	} else {
		// 만약 sid가 0이 아니라면, 어떤 값이든 상자에 담겼다는 뜻이므로 실패입니다.
		printf("[Case 3 - Zero DLC Test] Failed: Data was updated even with DLC 0! (SID: 0x%02X)\r\n", parsed_packet.sid);
	}
	 /* Case 4: 비정상 데이터 (SID 0x10, DLC 16) */
	memset(&parsed_packet, 0, sizeof(BKEL_Common_Packet_t));
	uint8_t long_payload[16] = {0xFF, };
	handle_frame(0x10, 0x01, long_payload, 16, 111);

	if (parsed_packet.sid == 0) {
		printf("[Case 4 - Overflow Protection] Success: Oversized DLC for SID 0x10 was blocked.\r\n");
	} else {
		printf("[Case 4 - Overflow Protection] Failed: Buffer might be corrupted!\r\n");
	}

    /* Case 5: 정상 데이터 LED ON 테스트(SID: 0x10, DLC: 1, Payload: 0x01) */
    memset(&parsed_packet, 0, sizeof(BKEL_Common_Packet_t));
    uint8_t led_on_payload[1] = {0x01}; // 0x01: LD2 OFF
    handle_frame(0x10, 0x01, led_on_payload, 1, 555);

    if (parsed_packet.sid == 0x10 && parsed_packet.payload[0] == 0x01) {
        printf("[Case 5 - LED ON] Success: SID 0x10 and Data 0x01 stored correctly.\r\n");
    } else {
        printf("[Case 5 - LED ON] Failed: Data mismatch.\r\n");
    }

    /* Case 6: SID 테스트 */
    memset(&parsed_packet, 0, sizeof(BKEL_Common_Packet_t));
    handle_frame(0xFF, 0x01, normal_payload, 1, 221);
    if (parsed_packet.sid == 0) {
        printf("[Case 6 - Unknown SID] Success: Undefined SID was ignored.\r\n");
    } else {
        printf("[Case 6 - Unknown SID] Failed: System accepted undefined SID\r\n");
    }


    printf("--- [Handle Frame Test] Finish ---\r\n");
}

