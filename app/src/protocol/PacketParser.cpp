#include <core/SessionManager.hpp>
#include <protocol/PacketParser.hpp>
#include "main.h"
#include <iostream>
#include <string>

void PacketParser::push(const uint8_t* data, size_t len)
{
    std::lock_guard<std::mutex> lock(mtx);  // 버퍼 접근 전 잠금
    rxBuffer.insert(rxBuffer.end(), data, data + len);
    parse();
}


void PacketParser::parse()
{
    while (rxBuffer.size() >= (BKEL_SOF_SIZE + BKEL_HDR_SIZE))
    {
        // 1. 헤더(0x5A) 찾기
        if (rxBuffer[0] != SOF_DATA_VALUE)
        {
            rxBuffer.erase(rxBuffer.begin());
            continue; // 다음 바이트 검사
        }

        // 2. 헤더 정보 가상 읽기 (DLC 확인용)
        BKEL_Data_Frame_Header hdr;
        std::memcpy(&hdr, rxBuffer.data() + BKEL_SOF_SIZE, BKEL_HDR_SIZE);

        // DLC가 비정상적이면 이 0x5A는 가짜 헤더임
        if (hdr.dlc > BKEL_MAX_PAYLOAD)
        {
            rxBuffer.erase(rxBuffer.begin());
            continue;
        }

        // 3. 전체 프레임 완성 대기
        size_t frameLen = BKEL_SOF_SIZE + BKEL_HDR_SIZE + hdr.dlc + BKEL_CID_SIZE + BKEL_CRC_SIZE;
        if (rxBuffer.size() < frameLen)
        {
            break; // 데이터 더 올 때까지 대기
        }

        // 4. CRC 검증
        uint8_t receivedCrc = rxBuffer[frameLen - 1];
        uint8_t expectedCrc = calc_crc8(rxBuffer.data() + BKEL_SOF_SIZE, 
                                       BKEL_HDR_SIZE + hdr.dlc + BKEL_CID_SIZE);

        if (receivedCrc != expectedCrc)
        {
            // CRC 틀리면 이 0x5A는 버리고 다음 0x5A 찾기
            rxBuffer.erase(rxBuffer.begin());
            continue;
        }

        /* === 5. 정상 프레임 확정 === */
        BKEL_Frame frame;
        frame.sid  = hdr.sid;
        frame.type = hdr.type;
        frame.dlc  = hdr.dlc;
        
        // CID 추출 (바이트 위치 주의)
        std::memcpy(&frame.cid, rxBuffer.data() + BKEL_SOF_SIZE + BKEL_HDR_SIZE + hdr.dlc, 2);
        
        const uint8_t* payloadPtr = rxBuffer.data() + BKEL_SOF_SIZE + BKEL_HDR_SIZE;
        frame.payload.assign(payloadPtr, payloadPtr + hdr.dlc);
        
        // 데이터 출력!!
        onFrame(frame);

        // 처리한 만큼 버퍼에서 삭제
        rxBuffer.erase(rxBuffer.begin(), rxBuffer.begin() + frameLen);
    }
}

void PacketParser::onFrame(const BKEL_Frame frame) {

    if(frame.sid == 0x01) {
        SessionManager::getInstance().broadcast(frame);
    }
    else{
        SessionManager::getInstance().routeUartRxBySid(frame);
    }

   
    printf("\n[RX FRAME] SID: 0x%02X | TYPE: 0x%02X | DLC: %u | CID: %u\n", 
           frame.sid, frame.type, (uint32_t)frame.payload.size(), frame.cid);
 
    switch (frame.sid) {
        /* 0x20 & 0x21: PWM 데이터 (Duty 1B | Period 1B) */
        case 0x20:
        case 0x21:
            if (frame.payload.size() >= 2) {
                // 0~255 사이의 값을 0~100%로 환산
                float raw_duty = frame.payload[0];
                int duty_percent = (int)((raw_duty / 255.0f) * 100.0f); 
                
                uint8_t period = frame.payload[1];
                
                printf(" >> [%s] Duty: %d%% (Raw: %u), Period: %u KHz\n", 
                    (frame.sid == 0x20 ? "PWM OUT" : "PWM IN"), duty_percent, (uint8_t)raw_duty, period);
            }
            break;

        /* 0x22 & 0x23: ADC 데이터 (2Byte Little Endian) */
        case 0x22:
        case 0x23:
            if (frame.payload.size() >= 2) {
                uint16_t raw_adc = (frame.payload[1] << 8) | frame.payload[0];
                float percent = (raw_adc / 4095.0f) * 100.0f;
                printf(" >> [ADC%d] 값: %.2f %% (Raw: %u)\n", (frame.sid == 0x22 ? 1 : 2), percent, raw_adc);
            } else {
                printf(" [ERR] ADC 데이터 길이 부족 (DLC: %zu)\n", frame.payload.size());
            }
            break;

        /* 0x24 ~ 0x26: GPIO/LED 상태 (1Byte) */
        case 0x24:
        case 0x25:
        case 0x26:
            if (!frame.payload.empty()) {
                const char* target = (frame.sid == 0x24 ? "GPO" : (frame.sid == 0x25 ? "GPI" : "LD2"));
                printf(" >> [%s] 상태: %s (Raw: 0x%02X)\n", 
                       target, (frame.payload[0] == 0x01 ? "HIGH" : "LOW"), frame.payload[0]);
            } else {
                printf(" [ERR] GPIO 데이터 없음\n");
            }
            break;

        /* Broadcast 패킷이나 기타 패킷 처리 */
        default:
            // printf("[BKEL FRAME]\n");
            // printf(" SID : 0x%02X\n", frame.sid);
            // printf(" TYPE: 0x%02X\n", frame.type);
            // printf(" DLC : %u\n", frame.dlc);
            // printf(" CID : %u\n", frame.cid);
              if (!frame.payload.empty())
            {
                printf(" PAYLOAD: ");
                for (auto b : frame.payload)
                    printf("%02X ", b);
                printf("\n");
            }
            break;
    }
    printf("----------------------------------------------------------\n");
    
    
}
// void PacketParser::onFrame(const BKEL_Frame& frame)
// {
//     // 지금은 로그, 나중에 RPC/DIAG 분기
//     printf("[BKEL FRAME]\n");
//     printf(" SID : 0x%02X\n", frame.sid);
//     printf(" TYPE: 0x%02X\n", frame.type);
//     printf(" DLC : %u\n", frame.dlc);
//     printf(" CID : %u\n", frame.cid);

//     if (!frame.payload.empty())
//     {
//         printf(" PAYLOAD: ");
//         for (auto b : frame.payload)
//             printf("%02X ", b);
//         printf("\n------------------------------------------\n");
//         //printf("\n");
//     }

//     if (callback) {
//         callback(frame);
//     }
// }