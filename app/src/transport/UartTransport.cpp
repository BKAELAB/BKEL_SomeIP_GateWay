#include "transport/UartTransport.hpp"
#include "protocol/PacketParser.hpp"
#include "protocol/PacketEncoder.hpp"
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
    // 직접 write를 호출하는 방식으로 남겨둠
    if (fd < 0 || !data || len == 0) return 0;
    return write(fd, data, len);
}

void UART::txWorker() {
    while (running) {
        BKEL_Frame frame;

        // SessionManager를 통해 송신할 프레임 확인
        if (SessionManager::getInstance().popNextMcuFrame(frame)) {
            #ifdef UART_USE_DEBUG
            std::cout << "[UART TX] Data retrieved from session. SID: 0x" 
                      << std::hex << (int)frame.sid << std::dec << std::endl;
            #endif

            auto data = PacketEncoder::build_frame(
                frame.sid, frame.type, frame.payload.data(), frame.dlc, frame.cid
            );

            if (!data.empty()) {
                ssize_t sentBytes = write(fd, data.data(), data.size());
                
                #ifdef UART_USE_DEBUG
                if (sentBytes > 0) {
                    std::cout << "[UART TX] Hardware write success. Size: " << sentBytes << " bytes" << std::endl;
                } else {
                    perror("[UART TX] Hardware write failed");
                }
                #endif
            }
            continue; // 데이터가 존재하면 지연 없이 다음 프레임 확인
        }

        // 송신 데이터가 없을 경우 CPU 점유율 방지를 위해 대기
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
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