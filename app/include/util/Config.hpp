#pragma once

#include <string>

// #define _UART_DEBUG_

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