#include "transport/UartTransport.hpp"
#include "protocol/PacketParser.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <thread>
#include <atomic>

// 통신 환경(termios) 설정
UART::UART(const char* device, int baudrate) 
    : fd(-1), running(false), parser(nullptr)
{
    fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("[UART] Open failed");
        exit(1);
    }

    struct termios tty{};
    if (tcgetattr(fd, &tty) != 0) {
        perror("[UART] tcgetattr failed");
        exit(1);
    }

    cfsetospeed(&tty, baudrate);
    cfsetispeed(&tty, baudrate);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD); 
    tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);      
    tty.c_lflag = 0;                             
    tty.c_oflag = 0;                             

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 10; 

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("[UART] tcsetattr failed");
        exit(1);
    }
    std::cout << "[UART] Connected to: " << device << " at " << baudrate << " bps" << std::endl;
}

UART::~UART() {
    stop(); 
    if (fd >= 0) close(fd);
}

void UART::start(PacketParser* p) {
    if (running) return;
    this->parser = p;
    running = true;
    rxThread = std::thread(&UART::rxWorker, this);
}

void UART::rxWorker() {
    uint8_t buf[256];
    while (running) {
        ssize_t n = read(fd, buf, sizeof(buf)); 
        if (n > 0 && parser != nullptr) {
            parser->push(buf, (size_t)n); // 수신 데이터를 파서로 전달
        }
    }
}

void UART::stop() {
    if (!running) return;
    running = false; 
    if (rxThread.joinable()) rxThread.join(); 
}

uint32_t UART::writeData(const uint8_t* data, uint32_t len) {
    
    if (fd < 0 || data == nullptr || len == 0) return 0;
    // 락 걸기 전 로그
    // printf("[TRY] Thread ID: %ld\n", std::this_thread::get_id());
    std::lock_guard<std::mutex> lock(txMtx);    // 여러 스레드가 동시에 write 하지 못하게
    // 락 통과 후 로그
    // printf("  [LOCKED] Thread ID: %ld 전송 중\n", std::this_thread::get_id());
    ssize_t sent = write(fd, data, len);
    // 전송 끝
    // printf("  [UNLOCKED] Thread ID: %ld 완료\n", std::this_thread::get_id());
    return (sent < 0) ? 0 : (uint32_t)sent;
}