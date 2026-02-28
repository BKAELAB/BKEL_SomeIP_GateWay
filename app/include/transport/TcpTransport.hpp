#pragma once

#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

class TcpTransport {
public:
    // 수신 콜백 타입: CID, 수신 데이터
    using RxCallback = std::function<void(const std::string&, const std::vector<uint8_t>&)>;

    // 생성자에 RxCallback 추가, SessionManager 완성 후 교체 예정.
    TcpTransport(int clientFd, const std::string& cid, RxCallback rxCallback);
    ~TcpTransport();

    void start();
    void stop();

    void sendData(const std::vector<uint8_t>&data); // Tx 큐에 데이터 추가

    const std::string& getCid() const { return cid_; }

private:
    void rxLoop();  // Req-B-22: 백그라운드 Rx Thread
    void txLoop();  // Req-B-21: 백그라운드 Tx Thread

    int clientFd_;
    std::string cid_;
    RxCallback rxCallback_;

    std::atomic<bool> running_;
    std::thread rxThread_;
    std::thread txThread_;

    std::queue<std::vector<uint8_t>> txQueue_;
    std::mutex txMutex_;
    std::condition_variable txCv_;  // 큐에 데이터 들어오면 TxThread 깨움
};