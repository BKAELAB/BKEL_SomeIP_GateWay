#include "transport/TcpTransport.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

TcpTransport::TcpTransport(int clientFd, const std::string& cid, RxCallback rxCallback)
    : clientFd_(clientFd), cid_(cid), rxCallback_(rxCallback), running_(false) {}

TcpTransport::~TcpTransport() {
    if (running_) {
        stop();
    }
}

void TcpTransport::start() {
    running_ = true;
    rxThread_= std::thread(&TcpTransport::rxLoop, this);
    txThread_= std::thread(&TcpTransport::txLoop, this);

}

void TcpTransport::stop() {
    running_ = false;

    // Rx: clientFd 닫아서 recv() 블로킹 해제
    if (clientFd_ >= 0) {
        ::shutdown(clientFd_, SHUT_RDWR);
        ::close(clientFd_);
        clientFd_ = -1;
    }

    //Tx: 큐에 데이터 없어도 TxThread 깨워서 종료
    txCv_.notify_all();

    if (rxThread_.joinable()) rxThread_.join();
    if (txThread_.joinable()) txThread_.join();

}

void TcpTransport::sendData(const std::vector<uint8_t>& data) {
    {
        std::lock_guard<std::mutex> lock(txMutex_);
        txQueue_.push(data);
    }
    txCv_.notify_one(); // txThread 깨움
}


void TcpTransport::rxLoop() {
    // Req-B-22: 백그라운드에서 실시간 패킷 수신
    uint8_t buf[1024];
    while (running_) {
        ssize_t n = recv(clientFd_, buf, sizeof(buf), 0);
        if (n <= 0) {
            // == 0 연결끊김, -1 에러
            std::cout << "[TcpTransport] CID=" << cid_ << "disconnected" << std::endl;
            running_ = false;
            break;
        }
        std::vector<uint8_t> data(buf, buf + n);

        // 패킷 포맷 확인 후 파싱 로직 교체
        if (rxCallback_) {
            rxCallback_(cid_, data);
        }
    }
    std::cout << "[TcpTransport] rxLoop exited, CID=" << cid_ << std::endl;
}

void TcpTransport::txLoop() {
    // Req-B-21: 큐에서 꺼내서 송신
    while (running_) {
        std::unique_lock<std::mutex> lock(txMutex_);

        // runnng_ 이 false 또는 큐에 값이 있을 때 깨어남
        txCv_.wait(lock, [this] {
            return !txQueue_.empty() || !running_;
        });

        while (!txQueue_.empty()) {
            auto data = txQueue_.front();
            txQueue_.pop();
            lock.unlock();

            if (clientFd_ >= 0) {
                send(clientFd_, data.data(), data.size(), 0);
            }

            lock.lock();
        }
    }
    std::cout << "[TcpTransport] txLoop exit, CID=" << cid_ << std::endl;
}