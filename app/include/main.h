#pragma once

#include <transport/UartTransport.hpp>
#include <transport/TcpServer.hpp>
#include <transport/TcpTransport.hpp>
#include <protocol/PacketParser.hpp>
#include <protocol/Packet.hpp>
#include <protocol/PacketEncoder.hpp>
#include <core/Engine.hpp>
#include <core/Session.hpp>
#include <core/SessionManager.hpp>
#include <util/Config.hpp>
#include <util/Logger.hpp>

#include <core/Types.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <termios.h> 
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <vector>
#include <csignal> // 테스트 용으로 추가

#define DEFAULT_UART_DEVICE "/dev/ttyAMA0"