#include "transport/TcpTransport.hpp"
#include "core/SessionManager.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

TcpTransport::TcpTransport(int clientFd, uint16_t cid)
    : clientFd_(clientFd), cid_(cid), running_(false) {}


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

    if (rxThread_.joinable()) {
        if (rxThread_.get_id() != std::this_thread::get_id()) {
            rxThread_.join(); // 외부 스레드에서 stop() 호출 시 rxThread 종료 대기
        } else {
            rxThread_.detach(); // rxLoop 내부에서 stop() 호출 시 자기 자신은 join 불가-> detach
        }                       // 클라이언트가 먼저 종료하는 경우
    }
    // txThread 는 자기 자신을 join 하는 경우 없음.
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
    // MSG_WAITALL로 각 필드를 순서대로 읽어 BKEL_Frame 조립 후 SessionManager에 직접 전달
    const uint16_t cid = cid_;  // removeSession 후 this가 파괴될 수 있으므로 미리 복사
    lastResetTime_ = std::chrono::steady_clock::now();
    packetCountInSecond_ = 0;

    while (running_) {
        // 1. SOF 수신
        uint8_t sof;
        if (recv(clientFd_, &sof, BKEL_SOF_SIZE, MSG_WAITALL) != BKEL_SOF_SIZE) break;


        // 보호 로직
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastResetTime_).count();

        // 1초(1000ms)가 지났는지 확인
        if (duration >= 5000) {
            // 1초가 지났으므로 카운트 리셋 및 기준 시간 갱신
            packetCountInSecond_ = 0;
            lastResetTime_ = now;
        }

        // 패킷 카운트 증가
        packetCountInSecond_++;

        if (packetCountInSecond_ > 100) {
            std::cerr << "\n[Security] DoS Detected! Banning IP: " << ipAddress_ 
                      << " (Count: " << packetCountInSecond_ << ")" << std::endl << std::flush;
            
            SessionManager::getInstance().addBlacklist(ipAddress_);
            break; // 루프 탈출 -> 세션 종료
        }

        // 2. Header 수신
        BKEL_Data_Frame_Header hdr{};
        if (recv(clientFd_, &hdr, BKEL_HDR_SIZE, MSG_WAITALL) != BKEL_HDR_SIZE) break;

        // 3. Payload 수신
        if (hdr.dlc > BKEL_MAX_PAYLOAD) {
            std::cerr << "[TcpTransport] invalid dlc=" << hdr.dlc << ", CID=" << cid << std::endl;
            break;
        }
        std::vector<uint8_t> payload(hdr.dlc);
        if (hdr.dlc > 0) {
            if (recv(clientFd_, payload.data(), hdr.dlc, MSG_WAITALL) != (ssize_t)hdr.dlc) break;
        }

        // 4. CID 수신
        uint16_t pktCid;
        if (recv(clientFd_, &pktCid, BKEL_CID_SIZE, MSG_WAITALL) != BKEL_CID_SIZE) break;

        // 5. CRC 수신
        uint8_t crc;
        if (recv(clientFd_, &crc, BKEL_CRC_SIZE, MSG_WAITALL) != BKEL_CRC_SIZE) break;

        // 6. BKEL_Frame 조립 후 SessionManager에 직접 전달
        BKEL_Frame frame;
        frame.sid     = hdr.sid;
        frame.type    = hdr.type;
        frame.dlc     = hdr.dlc;
        frame.cid     = pktCid;
        frame.payload = std::move(payload);

        // Req-B-34: TCP Rx 파싱 완료 시 CID로 Session 찾아 MCU 큐에 전달
        SessionManager::getInstance().forwardToMcu(cid, frame);
    }

    SessionManager::getInstance().removeSession(cid);
    // 주의: removeSession 이후 Session이 소멸되면 this도 파괴될 수 있음
    // cid_ 등 멤버 접근 금지, 로컬 변수 cid 사용
    ::close(clientFd_); // 소켓닫기
    std::cout << "[TcpTransport] rxLoop exited, CID=" << cid << std::endl;
    
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
                size_t totalSent = 0;
                while (totalSent < data.size()) {
                    ssize_t sent = send(clientFd_,
                                        data.data() + totalSent,    // 보낸 만큼 포인터 이동
                                        data.size() - totalSent,    // 남은 크기만큼
                                        0);
                    if (sent < 0) {
                        // 에러 처리
                        std::cerr << "[TcpTransport] send failed, CID=" << cid_ << std::endl;
                        // removeSession or disconnect 처리
                        break;
                    } 
                    totalSent += sent;
                }
            }
            lock.lock();
        }
    }
    std::cout << "[TcpTransport] txLoop exit, CID=" << cid_ << std::endl;
}