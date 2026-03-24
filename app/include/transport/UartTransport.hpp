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

class UART {
public:
    // 전역 어디서든 접근 가능한 싱글톤 객체 반환
    static UART& Get() {
        static UART instance;
        return instance;
    }

    ~UART();

    // 장치가 정상적으로 열렸는지 확인 (에러 방지용)
    bool isOpened() const { return fd != -1; }


    void start();
    
    // 모든 스레드 종료 및 리소스 해제
    void stop();

    // 데이터를 큐에 넣고 워커를 깨움 (비동기 전송)
    uint32_t writeData(const uint8_t* data, uint32_t len);

private:
    // 생성자 private 설정 (싱글톤)
    UART();
    void UARTInit();
    void rxWorker(); // 수신 전용 워커
    void txWorker(); // 전송 전용 워커

    int fd;
    int running;

    // 스레드 관리
    std::thread rxThread;
    std::thread txThread;

    UART(const UART&) = delete;
    UART& operator=(const UART&) = delete;
};