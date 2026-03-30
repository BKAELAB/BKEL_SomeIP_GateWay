#include "util/Logger.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

bool Logger::open(const std::string& path) {
    std::lock_guard<std::mutex> lock(mtx_);
    // append 모드: 기존 로그 파일에 이어서 기록
    file_.open(path, std::ios::app);
    return file_.is_open();
}

void Logger::log(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!file_.is_open()) return;

    // 출력 형식: [2026-03-18 10:23:01] [INFO ] [TcpServer] Listening on port 9999
    file_ << "[" << currentTime() << "] [" << levelToStr(level) << "] " << msg << "\n";
    file_.flush(); // 즉시 디스크에 기록 (버퍼링 없이)
}

std::string Logger::levelToStr(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        default:              return "UNKNOWN";
    }
}

std::string Logger::currentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm); // thread-safe 버전 (localtime 대신 사용)
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}