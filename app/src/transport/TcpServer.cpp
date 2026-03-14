#include "transport/TcpServer.hpp"
#include "core/SessionManager.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <arpa/inet.h>

TcpServer::TcpServer(int port)  // 파라미터 없이 리팩토링
    : port_(port), serverFd_(-1), pendingFd_(-1), running_(false) {}

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
#ifdef _TCP_DEBUG_
    std::cout << "[TcpServer] Listening on port " << port_ << std::endl;
#endif
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
            std::cerr << "[TcpServer] accept() failed" << std::endl;    // 로그 남기기 시간찍어서,
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
        auto cid = receivedCid(clientFd);       // 이놈을 실시간으로 바꿔야함.

        // recv() 완료 후 pendingFd_ 초기화
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            pendingFd_ = -1;
        }

        if (!cid) {     // nullopt 체크, (이후 cid 역참조 안전)
            ::close(clientFd);
            continue;
        }
        std::cout << "[TcpServer] Client connection, CID=" << *cid << std::endl;

        // Req-B-23: TCP 연결 시 Session 생성
        auto transport = std::make_unique<TcpTransport>(clientFd, *cid);                  // transport 생성
        SessionManager::getInstance().addSession(*cid, clientIp, std::move(transport));   // Session 생성

        std::cout << "[TcpServer] Session count=" << SessionManager::getInstance().getSessionCount() << std::endl;
    }
    std::cout << "[TcpServer] acceptLoop exit" << std::endl;
}

// Req-B-20 추가: 클라이언트 접속 후, SID:0X30 Register CID 등록 패킷 받아옴
// 패킷 수신 후 CID 반환
std::optional<uint16_t> TcpServer::receivedCid(int clientFd) {
    // SOF 확인
    uint8_t sof = 0;
    if (recv(clientFd, &sof, 1, MSG_WAITALL) != 1) return std::nullopt;
    if (sof != SOF_DATA_VALUE) return std::nullopt;

    // Header 수신
    BKEL_Data_Frame_Header hdr{};
    if (recv(clientFd, &hdr, BKEL_HDR_SIZE, MSG_WAITALL) != BKEL_HDR_SIZE) return std::nullopt;

    // sid 검증 0x30: Register CID
    if (hdr.sid != 0x30) return std::nullopt;

    // payload skip
    if (hdr.dlc > 0) {
        std::vector<uint8_t> dummy(hdr.dlc);
        if (recv(clientFd, dummy.data(), hdr.dlc, MSG_WAITALL) != hdr.dlc)
            return std::nullopt;
    }

    // CID 수신
    uint16_t cid = 0;
    if (recv(clientFd, &cid, BKEL_CID_SIZE, MSG_WAITALL) != BKEL_CID_SIZE) return std::nullopt;

    return cid;
}
