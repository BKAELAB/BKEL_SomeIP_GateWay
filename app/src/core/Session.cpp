#include <core/Session.hpp>
#include <protocol/PacketEncoder.hpp>
#include <utility>
#include <queue>
Session::Session(uint16_t cid, std::string ip, std::unique_ptr<ITransport> transport)
    : cid_(cid),
      ipAddress_(std::move(ip)),
      maliciousScore_(0),
      isBlocked_(false),
      transport_(std::move(transport)) {}

void Session::startTransport() {  // transport start-> tx, rxLoop 시작됨
    if (transport_) transport_->start();
}

void Session::updateLastRequestedSid(uint8_t sid) {
    requestedSidQueue_.push(sid);  // 가장 최근 요청한 SID 정보 담기
}

// UART Rx 할당 로직에서 비교
uint8_t Session::getLastRequestedSid() const {
    if (requestedSidQueue_.empty()) {
        return 0xFF; 
    }
    return requestedSidQueue_.front();
}

void Session::popRequestedSid() {
    if (!requestedSidQueue_.empty()) {
        requestedSidQueue_.pop();
    }
}

void Session::enqueueToMcu(const BKEL_Frame& frame) {
    std::lock_guard<std::mutex> lock(mtx_);
    toMcuQueue_.push_back(frame);       // MCU로 보낼 패킷 목록에 담기
}

// MCU로 보낼 패킷을 꺼내는 함수
bool Session::popMcuFrame(BKEL_Frame& out)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (toMcuQueue_.empty())
        return false;

    out = toMcuQueue_.front();
    toMcuQueue_.erase(toMcuQueue_.begin());
    return true;
}


void Session::setBlocked(bool blocked) {
    isBlocked_ = blocked;       // 악의적 행동 관련 차단 여부 정보 담기
}

bool Session::isBlocked() const {
    return isBlocked_;
}

void Session::sendFrame(const BKEL_Frame& frame) {
    // BKEL_Frame을 바이트 직렬화하여 txQueue에 넣음 (txLoop가 실제 송신)
    auto data = PacketEncoder::build_frame(
        frame.sid,
        frame.type,
        frame.payload.data(),
        frame.dlc,
        frame.cid
    );
    transport_->sendData(data);
}