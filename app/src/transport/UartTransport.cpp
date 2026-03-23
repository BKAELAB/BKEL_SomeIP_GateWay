#include "transport/UartTransport.hpp"
#include "protocol/PacketParser.hpp"
#include "core/SessionManager.hpp"
#include "util/Config.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>

// 생성자 함수로 빼기
UART::UART() : fd(-1), running(true) {
   UARTInit();
   start();
// running = true;
   #ifdef UART_USE_DEBUG
        std::cout << "현재 running 상태: " << running << std::endl;
   #endif
}

UART::~UART() {
    stop();
    if (fd >= 0) close(fd);
}

void UART::start() {
    rxThread = std::thread(&UART::rxWorker, this);
    txThread = std::thread(&UART::txWorker, this);
    #ifdef UART_USE_DEBUG
    std::cout << "[UART] 서비스 시작 성공" << std::endl;
    #endif
}

void UART::stop() {
    #ifdef UART_USE_DEBUG
        printf("stop 실행됨");
    #endif
    if (!running) return;
    running = false;

    if (rxThread.joinable()) rxThread.join();
    if (txThread.joinable()) txThread.join();
}

uint32_t UART::writeData(const uint8_t* data, uint32_t len) {
    if (fd < 0 || !data || len == 0) return 0;

    {
        // 큐에 데이터를 넣는 동안만 Lock
        std::lock_guard<std::mutex> lock(txMtx);
        txQueue.push(std::vector<uint8_t>(data, data + len));
    }
    
    // 잠자고 있는 Tx 워커 중 한 명을 깨움
    txCv.notify_one();
    
    return len;
}

void UART::txWorker() {
    while (running) {
        std::vector<uint8_t> data;
        {
            std::unique_lock<std::mutex> lock(txMtx);
            // 큐가 비어있으면 데이터가 들어올 때까지 대기
            txCv.wait(lock, [this] { return !txQueue.empty() || !running; });

            if (!running && txQueue.empty()) return;

            data = std::move(txQueue.front());
            txQueue.pop();
        }

        // 실제 전송 (write는 시스템 콜이므로 내부적으로 동기화됨)
        if (!data.empty()) {
            write(fd, data.data(), data.size());
        }
    }
}


void UART::rxWorker() {
     #ifdef UART_USE_DEBUG
        printf("rxWorker 들어옴");
        std::cout << "현재 running 상태: " << running << std::endl;
        #endif
    uint8_t buf[256];
    while (running) {
        #ifdef UART_USE_DEBUG
        printf("rxWorker read 전");
        #endif
        ssize_t n = read(fd, buf, sizeof(buf));
        #ifdef UART_USE_DEBUG
        printf("rxWorker read 끝");
        #endif
        if (n > 0) {
            // 수신 데이터는 즉시 파서의 큐로 전달
            #ifdef UART_USE_DEBUG
            std::string debugstr((char*)buf, n);
            std::cout << debugstr << std::endl;
            #endif
            PacketParser::Get().push(buf, (size_t)n);
        }
    }
}

void UART::UARTInit(){
     // 장치명 및 보레이트 고정 설정
    const char* device = "/dev/ttyAMA0";
    int baudrate = B115200;

    fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("[UART] 장치를 열 수 없습니다");
#ifdef UART_REQUIRED
        exit(1);
#else
        return;
#endif
    }

    struct termios tty{};
    if (tcgetattr(fd, &tty) != 0) {
        perror("[UART] tcgetattr 실패");
#ifdef UART_REQUIRED
        exit(1);
#else
        return;
#endif
    }

    // 기본 시리얼 통신 설정 (8N1)
    cfsetospeed(&tty, baudrate);    // 출력 속도 115200bps
    cfsetispeed(&tty, baudrate);    // 입력 속도 115200bps
    // 데이터 규격 설정: 8N1(8-bit, No Parity, 1 Stop bit)
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8비트 데이터 전송
    tty.c_cflag |= (CLOCAL | CREAD);                // 수신 가능 모드 활성화
    tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);    // 패리티 없음(N), 정지 비트 1개(1), 흐름 제어 없음
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("[UART] tcsetattr 실패");
#ifdef UART_REQUIRED
        exit(1);
#else
        return;
#endif
    }
}