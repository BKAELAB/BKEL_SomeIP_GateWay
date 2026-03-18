#pragma once

#include <cstdint>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <termios.h>

#define DEFAULT_UART_DEVICE "/dev/ttyAMA0"
#define DEFAULT_BAUDRATE     B115200

class PacketParser;

/**
 * 싱글톤 패턴: 시스템 내 단 하나의 UART 인스턴스 보장
 * Tx 스레드 풀: 비동기로 전송 대기열(Queue) 처리
 * 효율적 동기화: Condition Variable 사용
 */
class UART {
public:
    // 전역 어디서든 접근 가능한 싱글톤 객체 반환
    static UART& Get() {
        static UART instance;
        return instance;
    }

    ~UART();

    // [중요] 장치가 정상적으로 열렸는지 확인 (에러 방지용)
    // fd가 -1이 아니면 장치가 성공적으로 open된 상태입니다.
    bool isOpened() const { return fd != -1; }

    // 서비스 시작 (Rx 스레드 1개, Tx 스레드 워커들 생성)
    // 리턴 타입을 bool로 하여 시작 성공 여부를 확인할 수 있게 합니다.
    bool start(PacketParser* p);
    
    // 모든 스레드 종료 및 리소스 해제
    void stop();

    // 데이터를 큐에 넣고 워커를 깨움 (비동기 전송)
    uint32_t writeData(const uint8_t* data, uint32_t len);

private:
    // 생성자 private 설정 (싱글톤)
    UART();

    void rxWorker(); // 수신 전용 워커
    void txWorker(); // 전송 전용 워커

    int fd;
    std::atomic<bool> running;
    PacketParser* parser;

    // 스레드 관리
    std::thread rxThread;
    std::vector<std::thread> txThreadPool;

    // Tx 동기화 도구
    std::mutex txMtx;
    std::condition_variable txCv;
    std::queue<std::vector<uint8_t>> txQueue;

    // 복사 방지 (객체지향 안전장치)
    UART(const UART&) = delete;
    UART& operator=(const UART&) = delete;
};