#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <queue>
#include <protocol/Packet.hpp> // BKEL_Frame
#include <transport/TcpTransport.hpp> // transport
class Session {
public:
    Session(uint16_t cid, std::string ip, std::unique_ptr<TcpTransport> transport);  // fd 가지고 있기 위해서 TcpTransport 객체 소유
    
    //rx, tx 루프 시작
    void startTransport();

    // 세션 상태 갱신: 최근 요청 SID 기록
    void updateLastRequestedSid(uint8_t sid);

    // UART Rx 프레임 할당 시 last SID와 비교
    uint8_t getLastRequestedSid() const;

    // 큐 적재
    void enqueueToMcu(const BKEL_Frame& frame);

    bool popMcuFrame(BKEL_Frame& out);

    // 짝 찾았을 때 큐에서 제거하는 함수
    void popRequestedSid();  

    // 악의적 행동 정보 보유용
    void setBlocked(bool blocked);
    bool isBlocked() const;

    // BKEL_Frame을 인코딩하여 TCP 전송
    void sendFrame(const BKEL_Frame& frame);
                 
    // 테스트/검증을 위한 최소 getter (Req-B-24/25 확인용)
    // uint8_t getLastRequestedSid() const { return lastRequestedSid_; }
    // size_t  getToMcuQueueSize() const { return toMcuQueue_.size(); }
    // size_t  getToClientQueueSize() const { return toClientQueue_.size(); }

private:
    // 세션이 담아야 하는 정보
    uint16_t cid_;
    std::string ipAddress_;
    std::queue<uint8_t> requestedSidQueue_;
    std::unique_ptr<TcpTransport> transport_; 

    std::vector<BKEL_Frame> toMcuQueue_;     // MCU로 보낼 패킷 목록
    mutable std::mutex mtx_;
    
    bool isBlocked_;
};