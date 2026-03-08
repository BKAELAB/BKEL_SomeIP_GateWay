#pragma once
#include <cstdint>

constexpr uint8_t DIAG_SID_MIN = 0x20;
constexpr uint8_t DIAG_SID_MAX = 0x26;

// RPC의 경우 UART Return이 오지 않으므로 Diag 요청에 한하여 SID를 기억
inline bool isDiagSid(uint8_t sid)
{
    return sid >= DIAG_SID_MIN && sid <= DIAG_SID_MAX;
}