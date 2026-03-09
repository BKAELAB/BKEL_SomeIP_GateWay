#include "util/Config.hpp"
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    json j = json::parse(file);

    config_.tcp.ip                  = j["tcp"]["ip"];
    config_.tcp.port                = j["tcp"]["port"];
    config_.tcp.max_connections     = j["tcp"]["max_connections"];      // 최대 연결 수
    config_.tcp.timeout             = j["tcp"]["timeout"];              // 연결 타임아웃(초)    

    return true;
}

const AppConfig& Config::get() const {
    return config_;
}