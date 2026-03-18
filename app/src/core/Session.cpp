#include <core/Session.hpp>
#include <transport/TcpTransport.hpp>
#include <protocol/PacketParser.hpp>
#include <utility>
#include <iostream>
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

// UART Rx 할당 로직에서 비교
uint8_t Session::getLastRequestedSid() const {
    return lastRequestedSid_;
}

void Session::enqueueToMcu(const BKEL_Frame& frame) {
    std::cout << "[Session] enqueueToMcu, CID=" << cid_ << " SID=0x" << std::hex << (int)frame.sid << std::endl;
    toMcuQueue_.push(frame);       // MCU로 보낼 패킷 목록에 담기
}

// MCU로 보낼 패킷을 꺼내는 함수
bool Session::popMcuFrame(BKEL_Frame& out)
{
    if (toMcuQueue_.empty())
        return false;

    out = toMcuQueue_.front();
    toMcuQueue_.pop();
    return true;
}

void Session::enqueueToClient(const BKEL_Frame& frame) {
    toClientQueue_.push(frame);        // 되돌려 받을 패킷 목록에 담기
}

void Session::setBlocked(bool blocked) {
    isBlocked_ = blocked;       // 악의적 행동 관련 차단 여부 정보 담기
}

bool Session::isBlocked() const {
    return isBlocked_;
}

void Session::sendToClient() { // MCU에서 받은 패킷 (toClientQueue) TCP로 보냄
    while (!toClientQueue_.empty()) {
        auto frame = toClientQueue_.front();
        auto data = PacketEncoder::build_frame(
            frame.sid,
            frame.type,
            frame.payload.data(),
            frame.dlc,
            frame.cid
        );
        transport_->sendData(data);
        toClientQueue_.pop();
    }
}