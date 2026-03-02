#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include "transport/TcpTransport.hpp"

// Req-B-20: TCP 연결 수락 및 TcpTransport 생성
// Accept 루프는 별도의 백그라운드 Thread에서 동작
class TcpServer {
public:
    explicit TcpServer(const std::string& ip, int port, TcpTransport::RxCallback rxCallback);
    ~TcpServer();

    void startup();   // AcceptThread 시작
    void shutdown();    // 서버 종료

    // main Test 를 위해 임시 함수 추가
    // SessionManager 완성 후 제거할 것.
    void sendToFirst(const std::vector<uint8_t>& data);
    std::mutex& getTransportsMutex() { return transportsMutex_; }
    const std::vector<std::shared_ptr<TcpTransport>>& getTransports() { return transports_; }
    
private:
    void acceptLoop();  // 백그라운드 Thread 진입점

    std::string ip_;
    int port_;
    int serverFd_;
    std::atomic<bool> running_;
    std::thread acceptThread_;

    int pendingFd_;         // accept 후 recv 대기 중인 fd
    std::mutex pendingMutex_;

    // 임시 추가
    TcpTransport::RxCallback rxCallback_; // TcpTransport 생성 시 넘겨줌
    
    // SessionManager 완성 후 교체
    std::vector<std::shared_ptr<TcpTransport>> transports_;
    std::mutex transportsMutex_;
};