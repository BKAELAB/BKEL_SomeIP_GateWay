#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <cstdint>
#include "Session.hpp"

class SessionManager {
public:
    static SessionManager& getInstance();

    // 수명관리
    void addSession(uint16_t cid, const std::string& ip, std::unique_ptr<TcpTransport> transport);
    void removeSession(uint16_t cid);

    // 통신
    void broadcast(const BKEL_Frame& frame);
    void sendToSession(uint16_t cid, const BKEL_Frame& frame);

    // 프레임 수신 시 세션 상태 갱신(최근 SID 기록 등)
    void onFrameArrived(uint16_t cid, const std::string& ip, const BKEL_Frame& frame);
    
    // 연결관리
    size_t getSessionCount() const;
    // === 테스트/검증용 최소 함수 ===
    // size_t sessionCount();
    // size_t clientQueueSize(uint16_t cid);
    // size_t mcuQueueSize(uint16_t cid);
    // uint8_t lastSid(uint16_t cid);
    // void clearAllForTest(); // 테스트 반복 시 초기화용
    // void enqueueToMcu(uint16_t cid, const BKEL_Frame& frame);

private:
    SessionManager() = default;
    ~SessionManager() = default;

    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

private:
    std::shared_ptr<Session> findSessionNoLock_(uint16_t cid);

private:
    std::map<uint16_t, std::shared_ptr<Session>> sessions_;
    mutable std::mutex mtx_;
};