#include <core/SessionManager.hpp>
#include <core/Types.hpp>
#include <iostream>

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

void SessionManager::forwardToMcu(uint16_t cid, const BKEL_Frame& frame) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::cout << "[onTcpFrameArrived] cid=" << cid << " frame.cid=" << frame.cid << std::endl;
    auto session = findSessionNoLock_(cid);
    if (!session) return;               // Req-B-34: 일치하는 CID 없을 시, 폐기
        session->enqueueToMcu(frame);   // 일치하는 CID 있을 경우, Session에 패킷 할당
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

// Diag 요청에 한하여 SID를 기억
// 클라이언트가 Diag 요청을 보냈다는 이벤트 시점에 호출
void SessionManager::onDiagRequestArrived(uint16_t cid, const std::string& ip, const BKEL_Frame& reqFrame) {
    std::lock_guard<std::mutex> lock(mtx_);

    // // 수명관리(Add): 없으면 생성
    // if (sessions_.find(cid) == sessions_.end()) {
    //     sessions_[cid] = std::make_shared<Session>(cid, ip);
    // }

    // auto& sess = sessions_[cid];
    
    auto sess = findSessionNoLock_(cid);
    if (!sess) return;  // session 없으면 무시

    // Diag 요청일 때만 lastSid 기억
    // reqFrame이 Diag가 아닐 수도 있으니 방어
    if (isDiagSid(reqFrame.sid)) {
        sess->updateLastRequestedSid(reqFrame.sid);
    }
    sess->enqueueToMcu(reqFrame);
}

// UART Rx 유효 프레임 할당
// uartFrame.sid 와 각 Session의 lastRequestedSid가 일치하는 곳에 할당
// 겹치는 Session이 존재하면 모두 할당
void SessionManager::routeUartRxBySid(const BKEL_Frame& uartFrame) {
    std::lock_guard<std::mutex> lock(mtx_);

    for (auto& kv : sessions_) {
        auto& sess = kv.second;
        if (sess->isBlocked()) continue;

        // UART SID == Session lastRequestedSid
        if (sess->getLastRequestedSid() == uartFrame.sid) {
            sess->enqueueToClient(uartFrame);
        }
    }
}

// 모든 Session을 순회하면서 MCU로 보낼 패킷이 있는지 확인하고 UART Tx 에게 전달
bool SessionManager::popNextMcuFrame(BKEL_Frame& out)
{
    std::lock_guard<std::mutex> lock(mtx_);

    for (auto& kv : sessions_) {
        if (kv.second->popMcuFrame(out))
            return true;
    }

    return false;
}

// std::shared_ptr<Session> SessionManager::getSession(uint16_t cid)
// {
//     std::lock_guard<std::mutex> lock(mtx_);
//     return findSessionNoLock_(cid);
// }

// void SessionManager::clearSessions()
// {
//     std::lock_guard<std::mutex> lock(mtx_);
//     sessions_.clear();
// }