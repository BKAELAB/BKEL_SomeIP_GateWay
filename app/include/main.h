#pragma once

#include <transport/UartTransport.hpp>
#include <transport/TcpTransport.hpp>
#include <protocol/PacketParser.hpp>
#include <protocol/Packet.hpp>
#include <protocol/PacketEncoder.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>
#include <termios.h> 
#include <cstring>