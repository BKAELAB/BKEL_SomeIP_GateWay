#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <sstream>
#include <iomanip>

// 로그 레벨 정의 (임시)
// INFO : 정상 동작 기록 (연결, 세션 생성 등)
// WARN : 주의가 필요한 상황 (비정상이지만 치명적이지 않은 경우)
// ERROR: 에러 발생 (accept 실패, send 실패 등)
enum class LogLevel { INFO, WARN, ERROR };

// 싱글턴 파일 로거
// 사용법:
//   1. main에서 Logger::getInstance().open("logs/gateway.log") 호출
//   2. 이후 LOG_INFO / LOG_WARN / LOG_ERROR 매크로로 로그 기록
class Logger {
public:
    static Logger& getInstance();

    // 로그 파일 열기 (append 모드) - main에서 최초 1회 호출
    bool open(const std::string& path);

    // 로그 기록 - 타임스탬프 + 레벨 + 메시지 형식으로 파일에 저장
    // 멀티스레드 환경에서 안전하게 동작 (mutex 보호)
    void log(LogLevel level, const std::string& msg);

private:
    Logger() = default;
    std::ofstream file_;
    std::mutex mtx_;

    std::string levelToStr(LogLevel level);
    std::string currentTime();
};

// 로그 매크로 - 문자열 연산으로 메시지 구성 후 전달
// 예시: LOG_INFO("[TcpServer] Client connected, CID=" + std::to_string(cid))
#define LOG_INFO(msg)  Logger::getInstance().log(LogLevel::INFO,  msg)
#define LOG_WARN(msg)  Logger::getInstance().log(LogLevel::WARN,  msg)
#define LOG_ERROR(msg) Logger::getInstance().log(LogLevel::ERROR, msg)

inline std::string cidToHex(uint16_t cid) {
    std::ostringstream o;
    o << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << cid;
    return o.str();
}