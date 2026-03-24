#pragma once

#include <string>

// #define UART_USE_DEBUG
// #define COMMAND_TEST
// #define UART_REQUIRED   // 정의 시 UART 장치 없으면 exit(1), 미정의 시 경고만 출력하고 계속 실행
struct TcpConfig {
    std::string ip;
    int port;
    int max_connections;
    int timeout;
};

struct AppConfig {
    TcpConfig tcp;
};

class Config {
public:
    static Config& getInstance();
    bool load(const std::string& path);
    const AppConfig& get() const;
    
private:
    Config() = default;
    AppConfig config_;
};