#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <openssl/ssl.h>
#include <protocol/Packet.hpp>
#include "transport/ITransport.hpp"

class TlsTransport : public ITransport {
public:
    TlsTransport(SSL* ssl, int clientFd, uint16_t cid);
    ~TlsTransport();

    void start() override;
    void stop() override;
    void sendData(const std::vector<uint8_t>& data) override;
    uint16_t getCid() const override { return cid_; }

private:
    void rxLoop();
    void txLoop();
    bool readAll(void* buf, int len);

    SSL*     ssl_;
    int      clientFd_;
    uint16_t cid_;

    std::atomic<bool> running_;
    std::thread rxThread_;
    std::thread txThread_;

    std::queue<std::vector<uint8_t>> txQueue_;
    std::mutex txMutex_;
    std::condition_variable txCv_;
};
