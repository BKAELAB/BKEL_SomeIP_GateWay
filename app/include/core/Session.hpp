#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <protocol/Packet.hpp> // BKEL_Frame

class Session {
public:
    Session(uint16_t cid, std::string ip);

    // 세션 상태 갱신: 최근 요청 SID 기록
    void updateLastRequestedSid(uint8_t sid);

    // 큐 적재
    void enqueueToMcu(const BKEL_Frame& frame);
    void enqueueToClient(const BKEL_Frame& frame);

    // 악의적 행동 정보 보유용
    void setBlocked(bool blocked);
    bool isBlocked() const;

    // 테스트/검증을 위한 최소 getter (Req-B-24/25 확인용)
    // uint8_t getLastRequestedSid() const { return lastRequestedSid_; }
    // size_t  getToMcuQueueSize() const { return toMcuQueue_.size(); }
    // size_t  getToClientQueueSize() const { return toClientQueue_.size(); }

private:
    // 세션이 담아야 하는 정보
    uint16_t cid_;
    std::string ipAddress_;
    uint8_t lastRequestedSid_;

    std::vector<BKEL_Frame> toMcuQueue_;     // MCU로 보낼 패킷 목록
    std::vector<BKEL_Frame> toClientQueue_;  // 되돌려 받을 패킷 목록

    int maliciousScore_;
    bool isBlocked_;
};