#include "transport/TcpServer.hpp"
#include "core/SessionManager.hpp"
#include "transport/TlsTransport.hpp"
#include "util/Config.hpp"
#include "util/Logger.hpp"
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>

TcpServer::TcpServer()
    : ip_(""), port_(0), serverFd_(-1), running_(false) 
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) throw std::runtime_error("SSL_CTX_new() failed");

    if (SSL_CTX_use_certificate_file(ctx, "config/server.crt", SSL_FILETYPE_PEM) <= 0)
        throw std::runtime_error("Failed to load server.crt");

    if (SSL_CTX_use_PrivateKey_file(ctx, "config/server.key", SSL_FILETYPE_PEM) <= 0)
        throw std::runtime_error("Failed to load server.key");

    sslCtx_ = ctx;
}


TcpServer::~TcpServer() {
    if (running_) {  // 아직 안 끝났을 때만 shutdown 호출
        shutdown();
    }
}

void TcpServer::startup() {
    Config::getInstance().load("config/config.json");
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
    
    LOG_INFO("[TcpServer] Listening on port " + std::to_string(port_));
}


void TcpServer::shutdown() {
    running_ = false;

    if (serverFd_ >= 0) {
        ::shutdown(serverFd_, SHUT_RDWR);
        ::close(serverFd_);
        serverFd_ = -1;
    }

    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }

    if (sslCtx_) {
        SSL_CTX_free(sslCtx_);
        sslCtx_ = nullptr;
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
            LOG_ERROR("[TcpServer] accept() failed");
            continue;
        }
        // 블랙리스트 체크
        if (SessionManager::getInstance().isBanned(clientIp)) {
            std::cerr << "[Security] Connection denied. BANNED IP: " << clientIp << std::endl << std::flush;
            ::close(clientFd);
            continue;
        }

        SSL* ssl = SSL_new(sslCtx_);
        SSL_set_fd(ssl, clientFd);

        if (SSL_accept(ssl) <= 0) {
        std::cerr << "[TcpServer] SSL_accept failed" << std::endl;
        SSL_free(ssl);
        ::close(clientFd);
        continue;
        }

        auto cid = receivedCidTls(ssl);

        if (!cid) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ::close(clientFd);
        continue;
        }

        LOG_INFO("[TcpServer] Client connected, CID=" + cidToHex(*cid) + " IP=" + clientIp);

        auto transport = std::make_unique<TlsTransport>(ssl, clientFd, *cid);
        SessionManager::getInstance().addSession(*cid, clientIp, std::move(transport));

        LOG_INFO("[TcpServer] Session count=" + std::to_string(SessionManager::getInstance().getSessionCount()));
    }
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

std::optional<uint16_t> TcpServer::receivedCidTls(SSL* ssl) {
    uint8_t sof = 0;
    if (SSL_read(ssl, &sof, 1) != 1) return std::nullopt;
    if (sof != SOF_DATA_VALUE) return std::nullopt;

    BKEL_Data_Frame_Header hdr{};
    if (SSL_read(ssl, &hdr, BKEL_HDR_SIZE) != BKEL_HDR_SIZE) return std::nullopt;
    if (hdr.sid != 0x30) return std::nullopt;

    if (hdr.dlc > 0) {
        std::vector<uint8_t> dummy(hdr.dlc);
        if (SSL_read(ssl, dummy.data(), hdr.dlc) != hdr.dlc) return std::nullopt;
    }

    uint16_t raw_cid = 0;
    if (SSL_read(ssl, &raw_cid, BKEL_CID_SIZE) != BKEL_CID_SIZE) return std::nullopt;
    uint16_t cid = raw_cid >> 4;

    uint8_t crc = 0;
    if (SSL_read(ssl, &crc, BKEL_CRC_SIZE) != BKEL_CRC_SIZE) return std::nullopt;

    return cid;
}
