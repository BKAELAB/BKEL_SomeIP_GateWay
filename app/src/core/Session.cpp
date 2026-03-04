#include <core/Session.hpp>
#include <transport/TcpTransport.hpp>
#include <protocol/PacketParser.hpp>
#include <utility>
#include <iostream> // debug 용으로 추가
Session::Session(uint16_t cid, std::string ip, std::unique_ptr<TcpTransport> transport)
    : cid_(cid),
      ipAddress_(std::move(ip)),
      lastRequestedSid_(0),
      maliciousScore_(0),
      isBlocked_(false),
      transport_(std::move(transport)) {}

void Session::startTransport() {  // transport start-> tx, rxLoop 시작됨
    if (transport_) transport_->start();
}

void Session::updateLastRequestedSid(uint8_t sid) {
    lastRequestedSid_ = sid;    // 가장 최근 요청한 SID 정보 담기
}

void Session::enqueueToMcu(const BKEL_Frame& frame) {
    toMcuQueue_.push_back(frame);       // MCU로 보낼 패킷 목록에 담기
}

void Session::enqueueToClient(const BKEL_Frame& frame) {
    toClientQueue_.push_back(frame);        // 되돌려 받을 패킷 목록에 담기
}

void Session::setBlocked(bool blocked) {
    isBlocked_ = blocked;       // 악의적 행동 관련 차단 여부 정보 담기
}

bool Session::isBlocked() const {
    return isBlocked_;
}

void Session::sendToClient() { // MCU에서 받은 패킷 (toClientQueue) TCP로 보냄
    for (auto& frame : toClientQueue_) {
        auto data = PacketEncoder::build_frame(
            frame.sid,
            frame.type,
            frame.payload.data(),
            frame.dlc,
            frame.cid
        );
        std::cout << "[sendToClient] data size=" << data.size() << std::endl;
        transport_->sendData(data);
    }
    toClientQueue_.clear();
}