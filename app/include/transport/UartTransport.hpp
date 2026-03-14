#pragma once

#include <cstdint>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>


class PacketParser;

/* 1. 싱글턴 , 쓰는 곳에서 writeData 직접 호출하세요 .*/
/* 2. Tx 쓰레드 3개 만들고 , 얘를 재워놓고 , writeData 로 깨워서 보내기 */
class UART
{
public:
    explicit UART(const char* device, int baudrate);
    ~UART();

    // main.cpp에서 호출하는 함수 추가
    void start(PacketParser* p);
    void stop();

    uint32_t writeData(const uint8_t* data, uint32_t len);
    uint32_t readData(uint8_t* buf, uint32_t len); // 유지 할건지?

private:
    void rxWorker(); // 백그라운드 수신 스레드 루틴

    int fd;
    std::atomic<bool> running; // 스레드 실행 제어 플래그
    std::thread rxThread;      // 수신 전용 스레드
    PacketParser* parser;      // 데이터를 넘겨줄 파서 객체 포인터

    std::mutex txMtx;
};