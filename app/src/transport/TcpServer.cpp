#include "transport/TcpServer.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>

/* SessionManager 구현 후 수정할 것
 * 1. TcpServer 생성자 - RxCallback
 * 2. 테스트용 함수 sendToFirst()
 */

TcpServer::TcpServer(int port, TcpTransport::RxCallback rxCallback)
    : port_(port), serverFd_(-1), pendingFd_(-1), running_(false), rxCallback_(rxCallback) {}

TcpServer::~TcpServer() {
    if (running_) {  // 아직 안 끝났을 때만 shutdown 호출
        shutdown();
    }
}

// SessionManager 완성 후 제거
void TcpServer::sendToFirst(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(transportsMutex_);
    if (!transports_.empty()) {
        transports_[0]->sendData(data);
    } else {
        std::cerr << "[TcpServer] sendToFirst: no client connected" << std::endl;
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

    // recv() 블로킹 해제
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        if (pendingFd_ >= 0) {
            ::shutdown(pendingFd_, SHUT_RDWR);
            ::close(pendingFd_);
            pendingFd_ = -1;
        }
    }

    if (acceptThread_.joinable()) {
        std::cout << "[TcpServer] waiting acceptThread..." << std::endl;
        acceptThread_.join();
        std::cout << "[TcpServer] acceptThread done" << std::endl;
    }
    
    {
        std::lock_guard<std::mutex> lock(transportsMutex_);
        for (auto& transport : transports_) {
            transport->stop();
        }
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

        // recv() 블로킹 전에 pendingFd_ 저장
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            pendingFd_ = clientFd;
        }

        // Req-B-20: 연결 시 CID를 클라이언트로부터 수신
        // 실제 CID 수신 프로토콜에 맞게 수정 필요.
        char cidBuf[64] = {};
        ssize_t n = recv(clientFd, cidBuf, sizeof(cidBuf) - 1, 0);

        // recv() 완료 후 pendingFd_ 초기화
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            pendingFd_ = -1;
        }

        if (n <= 0) {
            ::close(clientFd);
            continue;
        }
        std::string cid(cidBuf, n);
        // cid '\n' '\r' 제거
        while (!cid.empty() && (cid.back() == '\n' || cid.back() == '\r')) {
            cid.pop_back();
        }
        std::cout << "[TcpServer] Client connection, CID=" << cid << std::endl;

        auto transport = std::make_shared<TcpTransport>(clientFd, cid, rxCallback_);
        {
            std::lock_guard<std::mutex> lock(transportsMutex_);
            transports_.push_back(transport);
        }
        transport->start();
    }
    std::cout << "[TcpServer] acceptLoop exit" << std::endl;
}