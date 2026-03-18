#include "core/CommandService.hpp"
#include "transport/UartTransport.hpp"
#include "protocol/PacketEncoder.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdio>
#include <vector>

CommandService& CommandService::Get() {
    static CommandService instance;
    return instance;
}

// LED 제어
void CommandService::ControlLed(uint8_t mode) {
    if (!UART::Get().isOpened()) return;
    uint16_t cid = currentCid++;
    uint8_t payload = mode;
    auto frame = PacketEncoder::build_frame(0x10, 0x01, &payload, 1, cid);
    printf("[CONTROL] LED 명령 전송: %d (CID: %u)\n", mode, cid);
    UART::Get().writeData(frame.data(), frame.size());
}

// MCU 리셋
void CommandService::ResetMcu(uint8_t mode) {
    if (!UART::Get().isOpened()) return;
    uint16_t cid = currentCid++;
    uint8_t payload = mode; 
    auto frame = PacketEncoder::build_frame(0x11, 0x01, &payload, 1, cid);
    printf("[CONTROL] MCU Reset 명령 전송 (Mode: %d, CID: %u)\n", mode, cid);
    UART::Get().writeData(frame.data(), frame.size());
}

// SPI Read
void CommandService::RequestSpiRead() {
    if (!UART::Get().isOpened()) return;
    uint16_t cid = currentCid++;
    uint8_t payload[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    auto frame = PacketEncoder::build_frame(0x12, 0x01, payload, 5, cid);
    printf("[CONTROL] SPI Read 요청 전송 (CID: %u)\n", cid);
    UART::Get().writeData(frame.data(), frame.size());
}

// SPI Write
void CommandService::RequestSpiWrite(const std::vector<uint8_t>& data) {
    if (!UART::Get().isOpened() || data.empty()) return;
    uint16_t cid = currentCid++;
    uint8_t payload[5] = {0x01, data[0], data[1], data[2], data[3]};
    auto frame = PacketEncoder::build_frame(0x12, 0x01, payload, 5, cid);
    printf("[CONTROL] SPI Write 명령 전송 (Data: %02X %02X %02X %02X, CID: %u)\n", 
            payload[1], payload[2], payload[3], payload[4], cid);
    UART::Get().writeData(frame.data(), frame.size());
}

// SPI 루프백 (Write -> Read)
void CommandService::RequestSpiLoopback(const std::vector<uint8_t>& data) {
    this->RequestSpiWrite(data);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    this->RequestSpiRead();
}

// 진단 요청
void CommandService::RequestDiagnostic(uint8_t sid) {
    if (!UART::Get().isOpened()) return;
    uint16_t cid = currentCid++;
    uint8_t dummy = 0x00;
    auto frame = PacketEncoder::build_frame(sid, 0x01, &dummy, 1, cid);
    std::cout << "[DIAG-REQ] SID 0x" << std::hex << (int)sid << " 상태값 요청 (CID: " << std::dec << cid << ")" << std::endl;
    UART::Get().writeData(frame.data(), frame.size());
}

// 진단 전체 테스트 (0x20 ~ 0x26)
void CommandService::RunDiagnosticTest() {
    printf("\n[SYSTEM] === MCU 진단 시작 (0x20 ~ 0x26) ===\n");
    for (uint8_t sid = 0x20; sid <= 0x26; ++sid) {
        this->RequestDiagnostic(sid);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void CommandService::ShowMenu() {
    while (true) {
        printf("\n==========================================");
        printf("\n[BKEL GATEWAY TEST MENU]");
        printf("\n 1: LED ON / 2: LED OFF / 3: LED Toggle");
        printf("\n 4: MCU Reset");
        printf("\n 5: SPI Read / 6: SPI Write");
        printf("\n 7: SPI Loopback Test");
        printf("\n 8: Diagnostic (0x20~0x26) All Test");
        printf("\n 0: Exit");
        printf("\n==========================================");
        printf("\n선택: ");

        int choice;

        if (scanf(" %d", &choice) != 1) { 
            while(getchar() != '\n'); // 잘못된 입력 청소
            continue; 
        }

        if (choice == 0) {
            printf("[SYSTEM] 메뉴를 종료합니다.\n");
            return; 
        }

        switch (choice) {
            case 1: this->ControlLed(0x01); break;
            case 2: this->ControlLed(0x00); break;
            case 3: this->ControlLed(0x02); break;
            case 4: this->ResetMcu(0x01); break;
            case 5: this->RequestSpiRead(); break;
            case 6: this->RequestSpiWrite({0xAA, 0xBB, 0xCC, 0xDD}); break;
            case 7: this->RequestSpiLoopback({0xDE, 0xAD, 0xBE, 0xEF}); break;
            case 8: this->RunDiagnosticTest(); break;
            default: printf(" [!] 잘못된 번호입니다.\n"); break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}