#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>
#include "transport/TcpTransport.hpp"

// Req-B-20: TCP 연결 수락 및 TcpTransport 생성
// Accept 루프는 별도의 백그라운드 Thread에서 동작
class TcpServer {
public:
    explicit TcpServer(int port);
    ~TcpServer();

    void startup();   // AcceptThread 시작
    void shutdown();    // 서버 종료

private:
    void acceptLoop();  // 백그라운드 Thread 진입점

    int port_;
    int serverFd_;
    std::atomic<bool> running_;
    std::thread acceptThread_;
};