#include "transport/TlsTransport.hpp"
#include "core/SessionManager.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

TlsTransport::TlsTransport(SSL* ssl, int clientFd, uint16_t cid)
    : ssl_(ssl), clientFd_(clientFd), cid_(cid), running_(false) {}

TlsTransport::~TlsTransport() {
    if (running_) stop();
}

void TlsTransport::start() {
    running_ = true;
    rxThread_ = std::thread(&TlsTransport::rxLoop, this);
    txThread_ = std::thread(&TlsTransport::txLoop, this);
}

void TlsTransport::stop() {
    running_ = false;

    if (ssl_) {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
    if (clientFd_ >= 0) {
        ::shutdown(clientFd_, SHUT_RDWR);
        ::close(clientFd_);
        clientFd_ = -1;
    }

    txCv_.notify_all();

    if (rxThread_.joinable()) {
        if (rxThread_.get_id() != std::this_thread::get_id())
            rxThread_.join();
        else
            rxThread_.detach();
    }
    if (txThread_.joinable()) txThread_.join();
}

void TlsTransport::sendData(const std::vector<uint8_t>& data) {
    {
        std::lock_guard<std::mutex> lock(txMutex_);
        txQueue_.push(data);
    }
    txCv_.notify_one();
}

// recv의 MSG_WAITALL 역할 - 정확히 len 바이트를 읽을 때까지 반복
bool TlsTransport::readAll(void* buf, int len) {
    int total = 0;
    while (total < len) {
        int n = SSL_read(ssl_, (char*)buf + total, len - total);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

void TlsTransport::rxLoop() {
    const uint16_t cid = cid_;
    while (running_) {
        uint8_t sof;
        if (!readAll(&sof, BKEL_SOF_SIZE)) break;

        BKEL_Data_Frame_Header hdr{};
        if (!readAll(&hdr, BKEL_HDR_SIZE)) break;

        if (hdr.dlc > BKEL_MAX_PAYLOAD) {
            std::cerr << "[TlsTransport] invalid dlc=" << hdr.dlc << std::endl;
            break;
        }
        std::vector<uint8_t> payload(hdr.dlc);
        if (hdr.dlc > 0) {
            if (!readAll(payload.data(), hdr.dlc)) break;
        }

        uint16_t pktCid;
        if (!readAll(&pktCid, BKEL_CID_SIZE)) break;

        uint8_t crc;
        if (!readAll(&crc, BKEL_CRC_SIZE)) break;

        BKEL_Frame frame;
        frame.sid     = hdr.sid;
        frame.type    = hdr.type;
        frame.dlc     = hdr.dlc;
        frame.cid     = pktCid;
        frame.payload = std::move(payload);

        SessionManager::getInstance().forwardToMcu(cid, frame);
    }

    SessionManager::getInstance().removeSession(cid);
    std::cout << "[TlsTransport] rxLoop exited, CID=" << cid << std::endl;
}

void TlsTransport::txLoop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(txMutex_);
        txCv_.wait(lock, [this] {
            return !txQueue_.empty() || !running_;
        });

        while (!txQueue_.empty()) {
            auto data = txQueue_.front();
            txQueue_.pop();
            lock.unlock();

            if (ssl_) {
                size_t totalSent = 0;
                while (totalSent < data.size()) {
                    int sent = SSL_write(ssl_,
                                        data.data() + totalSent,
                                        data.size() - totalSent);
                    if (sent <= 0) {
                        std::cerr << "[TlsTransport] SSL_write failed, CID=" << cid_ << std::endl;
                        break;
                    }
                    totalSent += sent;
                }
            }
            lock.lock();
        }
    }
    std::cout << "[TlsTransport] txLoop exit, CID=" << cid_ << std::endl;
}