#include "transport/TcpServer.hpp"
#include "core/SessionManager.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <arpa/inet.h>
/* SessionManager 구현 후 수정할 것
 * 1. TcpServer 생성자 - RxCallback
 */

TcpServer::TcpServer(const std::string& ip, int port, TcpTransport::RxCallback rxCallback)
    : ip_(ip), port_(port), serverFd_(-1), pendingFd_(-1), running_(false), rxCallback_(rxCallback) 
    {
        
    }

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
    addr.sin_port = htons(port_);

    // IP 설정
    if (inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr) <= 0)
        throw std::runtime_error("Invalid IP addrress");

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
        // ClientIp 추출
        char clientIpBuf[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIpBuf, sizeof(clientIpBuf));
        std::string clientIp(clientIpBuf);

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
        // Req-B-23: TCP 연결 시 Session 생성
        uint16_t cidNum = static_cast<uint16_t>(std::stoul(cid));  // (임시) 문자열로 받고 uint16_t 로 변경
        auto transport = std::make_unique<TcpTransport>(clientFd, cid, rxCallback_);
        SessionManager::getInstance().addSession(cidNum, clientIp, std::move(transport));   //Session 생성

        std::cout << "[TcpServer] Session count=" << SessionManager::getInstance().getSessionCount() << std::endl;
    }
    std::cout << "[TcpServer] acceptLoop exit" << std::endl;
}