cmake_minimum_required(VERSION 3.16)
project(BKEL_SomeIP_GateWay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# nlohmann/json 추가
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(bkel_gateway
    src/main.cpp
    src/core/Engine.cpp
    src/core/Session.cpp
    src/core/SessionManager.cpp
    src/core/CommandService.cpp
    src/transport/UartTransport.cpp
    src/transport/TcpTransport.cpp
    src/transport/TcpServer.cpp
    src/transport/TlsTransport.cpp
    src/protocol/PacketParser.cpp
    src/protocol/PacketEncoder.cpp
    src/util/Config.cpp
)

target_include_directories(bkel_gateway PRIVATE include)

target_link_libraries(bkel_gateway
    pthread
    ssl
    crypto
    nlohmann_json::nlohmann_json
)

# 테스트 클라이언트
add_executable(test_client
    src/test_client.cpp
    src/protocol/PacketParser.cpp
    src/protocol/PacketEncoder.cpp
)

target_include_directories(test_client PRIVATE include)

target_link_libraries(test_client
    pthread
)