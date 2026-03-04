#include <core/SessionManager.hpp>
#include <iostream> // debug , log
SessionManager& SessionManager::getInstance() {
    static SessionManager instance;
    return instance;
}

std::shared_ptr<Session> SessionManager::findSessionNoLock_(uint16_t cid) {
    auto it = sessions_.find(cid);
    if (it == sessions_.end()) return nullptr;
    return it->second;
}

void SessionManager::addSession(uint16_t cid, const std::string& ip, std::unique_ptr<TcpTransport> transport) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (sessions_.find(cid) == sessions_.end()) {
        sessions_[cid] = std::make_shared<Session>(cid, ip, std::move(transport));
        sessions_[cid]->startTransport();
    }
}

void SessionManager::removeSession(uint16_t cid) {
    std::lock_guard<std::mutex> lock(mtx_);
    sessions_.erase(cid);
    std::cout << "[SessionManager] Session removed, CID=" << cid << " count=" << sessions_.size() << std::endl;
}

void SessionManager::onFrameArrived(uint16_t cid, const std::string& ip, const BKEL_Frame& frame) {
    std::lock_guard<std::mutex> lock(mtx_);

    // // 수명관리(Add): 없으면 생성
    // if (sessions_.find(cid) == sessions_.end()) {
    //     sessions_[cid] = std::make_shared<Session>(cid, ip);
    // }
    // Session 생성 시, transport(fd)객체 같이 받아야 하므로 cid 만으로는 생성 불가
    // Session 없으면 무시 (생성은 TCP 연결 시 addSession에서만)
    auto session = findSessionNoLock_(cid);
    if (!session) return;

    // Req-B-25: 최근 요청 SID 기록
    sessions_[cid]->updateLastRequestedSid(frame.sid);
}

void SessionManager::broadcast(const BKEL_Frame& frame) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& kv : sessions_) {
        kv.second->enqueueToClient(frame);
    }
}

void SessionManager::sendToSession(uint16_t cid, const BKEL_Frame& frame) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto s = findSessionNoLock_(cid);
    if (!s) return;

    s->enqueueToClient(frame);
    s->sendToClient();  // Req-B-32: 패킷 TCP 전송
}

// 연결된 세션 수 확인
size_t SessionManager::getSessionCount() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return sessions_.size();
}

// 테스트 코드 확인용
// size_t SessionManager::sessionCount() {
//     std::lock_guard<std::mutex> lock(mtx_);
//     return sessions_.size();
// }

// size_t SessionManager::clientQueueSize(uint16_t cid) {
//     std::lock_guard<std::mutex> lock(mtx_);
//     auto s = findSessionNoLock_(cid);
//     return s ? s->getToClientQueueSize() : 0;
// }

// size_t SessionManager::mcuQueueSize(uint16_t cid) {
//     std::lock_guard<std::mutex> lock(mtx_);
//     auto s = findSessionNoLock_(cid);
//     return s ? s->getToMcuQueueSize() : 0;
// }

// uint8_t SessionManager::lastSid(uint16_t cid) {
//     std::lock_guard<std::mutex> lock(mtx_);
//     auto s = findSessionNoLock_(cid);
//     return s ? s->getLastRequestedSid() : 0;
// }

// void SessionManager::clearAllForTest() {
//     std::lock_guard<std::mutex> lock(mtx_);
//     sessions_.clear();
// }

// void SessionManager::enqueueToMcu(uint16_t cid, const BKEL_Frame& frame) {
//     std::lock_guard<std::mutex> lock(mtx_);
//     auto s = findSessionNoLock_(cid);
//     if (!s) return;
//     s->enqueueToMcu(frame);
// }