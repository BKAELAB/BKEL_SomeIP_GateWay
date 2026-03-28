#include "transport/TcpServer.hpp"
#include "core/SessionManager.hpp"
#include "util/Config.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>

TcpServer::TcpServer()
    : ip_(""), port_(0), serverFd_(-1), running_(false) {}

TcpServer::~TcpServer() {
    if (running_) {  // 아직 안 끝났을 때만 shutdown 호출
        shutdown();
    }
}

void TcpServer::startup() {
    const auto& cfg = Config::getInstance().get().tcp;
    ip_   = cfg.ip;
    port_ = cfg.port;

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
        char clientIpBuf[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIpBuf, sizeof(clientIpBuf));
        std::string clientIp(clientIpBuf); // 여기서 변수 생성

        bool banned = SessionManager::getInstance().isBanned(clientIp);
        std::cout << "[Check] IP: " << clientIp << " | IsBanned: " << (banned ? "YES" : "NO") << std::endl << std::flush;

        if (clientFd < 0) {
            if (!running_)  break;  // shutdown() 호출 시 정상 종료
            std::cerr << "[TcpServer] accept() failed" << std::endl;    // 로그 남기기 시간찍어서,
            continue;
        }
        // 블랙리스트 체크
        if (SessionManager::getInstance().isBanned(clientIp)) {
            std::cerr << "[Security] Connection denied. BANNED IP: " << clientIp << std::endl << std::flush;
            ::close(clientFd);
            continue; 
        }

        // 5초 안에 CID 패킷 안 보내면 연결 끊음
        struct timeval tv { .tv_sec = 5, .tv_usec = 0 };
        setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        // Req-B-20: 연결 시 CID를 클라이언트로부터 수신
        auto cid = receivedCid(clientFd);

        if (!cid) {     // nullopt 체크, (이후 cid 역참조 안전)
            ::close(clientFd);
            continue;
        }

        // 타임아웃 해제 - TcpTransport rxLoop의 recv()에 영향 주지 않도록
        struct timeval noTimeout { .tv_sec = 0, .tv_usec = 0 };
        setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &noTimeout, sizeof(noTimeout));
        std::cout << "[TcpServer] Client connection, CID=" << *cid << std::endl;

        // Req-B-23: TCP 연결 시 Session 생성
        auto transport = std::make_unique<TcpTransport>(clientFd, *cid);                  // transport 생성
        transport->setIpAddress(clientIp);
        SessionManager::getInstance().addSession(*cid, clientIp, std::move(transport));   // Session 생성

        std::cout << "[TcpServer] Session count=" << SessionManager::getInstance().getSessionCount() << std::endl;
    }
    std::cout << "[TcpServer] acceptLoop exit" << std::endl;
}

// Req-B-20 추가: 클라이언트 접속 후, SID:0X30 Register CID 등록 패킷 받아옴
// 패킷 수신 후 CID 반환
std::optional<uint16_t> TcpServer::receivedCid(int clientFd) {
    uint8_t sof = 0;
    
    // 1. SOF 확인 (1바이트)
    if (recv(clientFd, &sof, 1, MSG_WAITALL) != 1) return std::nullopt;
    if (sof != SOF_DATA_VALUE) return std::nullopt;

    BKEL_Data_Frame_Header hdr{};
    if (recv(clientFd, &hdr, BKEL_HDR_SIZE, MSG_WAITALL) != BKEL_HDR_SIZE) {
        std::cerr << "[Debug] Header Recv Failed (Expected " << (int)BKEL_HDR_SIZE << " bytes)" << std::endl;
        return std::nullopt;
    }

    // 3. SID 검증 (0x30)
    if (hdr.sid != 0x30) {
        std::cerr << "[Debug] SID Mismatch: 0x" << std::hex << (int)hdr.sid << std::dec << " (Expected 0x30)" << std::endl;
        return std::nullopt;
    }

    // 4. Payload 건너뛰기
    if (hdr.dlc > 0) {
        std::vector<uint8_t> dummy(hdr.dlc);
        if (recv(clientFd, dummy.data(), hdr.dlc, MSG_WAITALL) != hdr.dlc) return std::nullopt;
    }

    // 5. CID 수신 (BKEL_CID_SIZE)
    uint16_t raw_cid = 0;
    if (recv(clientFd, &raw_cid, BKEL_CID_SIZE, MSG_WAITALL) != BKEL_CID_SIZE) {
        std::cerr << "[Debug] CID Recv Failed" << std::endl;
        return std::nullopt;
    }
    
    // 6. CRC 수신 (BKEL_CRC_SIZE)
    uint8_t crc = 0;
    if (recv(clientFd, &crc, BKEL_CRC_SIZE, MSG_WAITALL) != BKEL_CRC_SIZE) {
        std::cerr << "[Debug] CRC Recv Failed" << std::endl;
        return std::nullopt;
    }

    uint16_t cid = raw_cid >> 4;

    return cid;
}
