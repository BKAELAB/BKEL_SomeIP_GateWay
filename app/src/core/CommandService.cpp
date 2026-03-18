#include "core/CommandService.hpp"
#include "transport/UartTransport.hpp"
#include "protocol/PacketEncoder.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdio>

// Engine에서 호출하는 Get() 함수
CommandService& CommandService::Get() {
    static CommandService instance;
    return instance;
}

// 기능 제어 및 테스트


void CommandService::ControlLed(uint8_t mode) {
    if (!UART::Get().isOpened()) return;
    uint8_t payload = mode;
    auto frame = PacketEncoder::build_frame(0x10, 0x01, &payload, 1, currentCid++);
    std::cout << "[CONTROL] LED 명령 전송: " << (int)mode << std::endl;
    UART::Get().writeData(frame.data(), frame.size());
}

void CommandService::ResetMcu(uint8_t mode) {
    if (!UART::Get().isOpened()) return;
    uint8_t payload = mode;
    auto frame = PacketEncoder::build_frame(0x11, 0x01, &payload, 1, currentCid++);
    std::cout << "[CONTROL] MCU Reset 명령 전송" << std::endl;
    UART::Get().writeData(frame.data(), frame.size());
}

void CommandService::RequestSpiLoopback(uint8_t type, const std::vector<uint8_t>& data) {
    if (!UART::Get().isOpened()) return;
    std::vector<uint8_t> payload;
    payload.push_back(type);
    if (type == 0x01 && !data.empty()) payload.insert(payload.end(), data.begin(), data.end());
    auto frame = PacketEncoder::build_frame(0x12, 0x01, payload.data(), payload.size(), currentCid++);
    std::cout << "[TEST] SPI Loopback 테스트 전송" << std::endl;
    UART::Get().writeData(frame.data(), frame.size());
}

// MCU 상태값 요청 (Diagnostic/Monitoring)

void CommandService::RequestDiagnostic(uint8_t sid) {
    if (!UART::Get().isOpened()) return;

    // MCU에게 해당 SID의 상태를 보고하라는 요청 (데이터 조작 없음)
    uint8_t dummy = 0x00; 
    auto frame = PacketEncoder::build_frame(sid, 0x01, &dummy, 1, currentCid++);
    
    std::cout << "[DIAG-REQ] SID 0x" << std::hex << (int)sid << " 상태값 요청" << std::dec << std::endl;
    UART::Get().writeData(frame.data(), frame.size());
}

void CommandService::RunDiagnosticTest() {
    printf("\n[SYSTEM] === MCU 진단 시작 (0x20 ~ 0x26) ===\n");
    
    // 0x20(PWM), 0x21(ADC1), 0x22(ADC2) ... 0x26(LED Pin) 상태 요청
    for (uint8_t sid = 0x20; sid <= 0x26; ++sid) {
        this->RequestDiagnostic(sid); 
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    printf("[SYSTEM] 진단 완료.\n");
}
