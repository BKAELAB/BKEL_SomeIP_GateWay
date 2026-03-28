#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <optional>
#include <set>
#include "transport/TcpTransport.hpp"

// Req-B-20: TCP 연결 수락 및 TcpTransport 생성
// Accept 루프는 별도의 백그라운드 Thread에서 동작
class TcpServer {
public:
    static TcpServer& Get() {
        static TcpServer instance;
        return instance;
    }

    ~TcpServer();

    void startup();     // Config 로드 후 AcceptThread 시작
    void shutdown();    // 서버 종료

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

private:
    TcpServer();

    void acceptLoop();  // 백그라운드 Thread 진입점
    std::optional<uint16_t> receivedCid(int clientFd); // 클라이언트 처음 연결 시 cid 반환

    std::string ip_;
    int port_;
    int serverFd_;
    std::atomic<bool> running_;
    std::thread acceptThread_;

};