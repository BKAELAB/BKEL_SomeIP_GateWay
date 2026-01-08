cmake_minimum_required(VERSION 3.16)
project(BKEL_SomeIP_GateWay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(bkel_gateway
    src/main.cpp
    src/core/Session.cpp
    src/core/SessionManager.cpp
    src/transport/UartTransport.cpp
    src/transport/TcpTransport.cpp
    src/transport/TlsTransport.cpp
    src/protocol/PacketParser.cpp
)

target_include_directories(bkel_gateway PRIVATE include)

target_link_libraries(bkel_gateway
    pthread
    ssl
    crypto
)
