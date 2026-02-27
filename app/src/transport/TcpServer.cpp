#include "transport/TcpServer.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>

TcpServer::TcpServer(int port)
    : port_(port), serverFd_(-1), running_(false) {}

TcpServer::~TcpServer() {
    if (running_) {  // 아직 안 끝났을 때만 shutdown 호출
        shutdown();
    }
}

void TcpServer::startup() {
    serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0) throw std::runtime_error("socket() failed");

    // Port 재사용 옵션
    int opt = 1;
    setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(serverFd_, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");

    if (listen(serverFd_, 10) < 0)
        throw std::runtime_error("listen() failed");

    running_ = true;

    // Req-B-20: Accept를 백그라운드 Thread에서 처리
    acceptThread_ = std::thread(&TcpServer::acceptLoop, this);
    std::cout << "[TcpServer] Listening on port " << port_ << std::endl;
}


void TcpServer::shutdown() {
    std::cout << "[TcpServer] shutdown() called" << std::endl;
    running_ = false;

    if (serverFd_ >= 0) {
        ::shutdown(serverFd_, SHUT_RDWR);
        ::close(serverFd_);
        serverFd_ = -1;
    }
    std::cout << "[TcpServer] serverFd closed" << std::endl;

    if (acceptThread_.joinable()) {
        std::cout << "[TcpServer] waiting acceptThread..." << std::endl;
        acceptThread_.join();
        std::cout << "[TcpServer] acceptThread done" << std::endl;
    }
}

void TcpServer::acceptLoop() {
    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);


        int clientFd = accept(serverFd_, (sockaddr*)&clientAddr, &addrLen);
        if (clientFd < 0) {
            if (!running_)  break;  // shutdown() 호출 시 정상 종료
            std::cerr << "[TcpServer] accept() failed" << std::endl;
            continue;
        }

        // Req-B-20: 연결 시 CID를 클라이언트로부터 수신
        // 실제 CID 수신 프로토콜에 맞게 수정 필요.
        char cidBuf[64] = {};
        ssize_t n = recv(clientFd, cidBuf, sizeof(cidBuf) - 1, 0);
        if (n <= 0) {
            ::close(clientFd);
            continue;
        }
        std::string cid(cidBuf, n);
        std::cout << "[TcpServer] Client connection, CID=" << cid << std::endl;
    }
    std::cout << "[TcpServer] acceptLoop exit" << std::endl;
}