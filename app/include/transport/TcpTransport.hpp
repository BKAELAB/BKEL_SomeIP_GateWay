#pragma once

#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <protocol/Packet.hpp>
#include "transport/ITransport.hpp"

class TcpTransport : public ITransport {
public:
    TcpTransport(int clientFd, uint16_t cid);
    ~TcpTransport();

    void start();
    void stop();

    void sendData(const std::vector<uint8_t>&data); // Tx 큐에 데이터 추가

    uint16_t getCid() const { return cid_; }
    
    void setIpAddress(std::string ip) { ipAddress_ = std::move(ip); } // IP 설정 함수

private:
    void rxLoop();  // Req-B-22: 백그라운드 Rx Thread
    void txLoop();  // Req-B-21: 백그라운드 Tx Thread

    int clientFd_;
    uint16_t cid_;

    std::atomic<bool> running_;
    std::thread rxThread_;
    std::thread txThread_;

    std::queue<std::vector<uint8_t>> txQueue_;
    std::mutex txMutex_;
    std::condition_variable txCv_;  // 큐에 데이터 들어오면 TxThread 깨움

    int packetCountInSecond_ = 0;
    std::chrono::steady_clock::time_point lastResetTime_ = std::chrono::steady_clock::now();

    std::string ipAddress_; // IP 저장
    bool isBlocked_ = false; // 차단 플래그
};