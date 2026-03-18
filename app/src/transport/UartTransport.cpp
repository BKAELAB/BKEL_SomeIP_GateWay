#include "transport/UartTransport.hpp"
#include "protocol/PacketParser.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>

UART::UART() : fd(-1), running(false), parser(nullptr) {
    // 장치명 및 보레이트 고정 설정
    const char* device = "/dev/ttyAMA0";
    int baudrate = B115200;

    fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("[UART] 장치를 열 수 없습니다");
        exit(1);
    }

    struct termios tty{};
    if (tcgetattr(fd, &tty) != 0) {
        perror("[UART] tcgetattr 실패");
        exit(1);
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
        exit(1);
    }
}

UART::~UART() {
    stop();
    if (fd >= 0) close(fd);
}

// void UART::start(PacketParser* p) {
//     if (running) return true;    // 처음에 false -> 아래 실행
//     this->parser = p;
    
//     // 스레드가 시작되기 전에 running 플래그를 먼저 활성화
//     running = true;

//     // Rx 워커 시작
//     rxThread = std::thread(&UART::rxWorker, this);

//     // Tx 워커 3개 시작 (Condition Variable에 의해 즉시 잠듦)
//     for (int i = 0; i < 3; ++i) {
//         txThreadPool.emplace_back(&UART::txWorker, this);
//     }
// }
bool UART::start(PacketParser* p) {
    // 1. 이미 실행 중이라면 성공(true)으로 간주하고 리턴
    if (running) {
        return true; 
    }

    this->parser = p;

    // 2. 장치 오픈 및 설정 (형님의 기존 open 코드가 여기에 들어감)
    // 만약 open에 실패했다면 여기서 return false;를 해줘야 Engine이 멈춥니다.
    /*
    fd = open(DEFAULT_UART_DEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        perror("UART Open Failed");
        return false; 
    }
    */

    // 3. 스레드 가동
    running = true;
    rxThread = std::thread(&UART::rxWorker, this);
    
    for (int i = 0; i < 3; ++i) {
        txThreadPool.push_back(std::thread(&UART::txWorker, this));
    }

    std::cout << "[UART] 서비스 시작 성공" << std::endl;

    // 4. 모든 준비가 끝났으므로 true 반환
    return true; 
}

void UART::stop() {
    if (!running) return;
    running = false;

    // 잠자고 있는 모든 워커를 깨워서 종료시킴
    txCv.notify_all();

    if (rxThread.joinable()) rxThread.join();
    for (auto& t : txThreadPool) {
        if (t.joinable()) t.join();
    }
    txThreadPool.clear();
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
            // 큐가 비어있으면 데이터가 들어올 때까지 여기서 잠듦 (CPU 점유 0%)
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
    uint8_t buf[256];
    while (running) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n > 0 && parser != nullptr) {
            // 수신 데이터는 즉시 파서의 큐로 전달
            parser->push(buf, (size_t)n);
        }
    }
}