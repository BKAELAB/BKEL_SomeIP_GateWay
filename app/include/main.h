#pragma once

#include <transport/UartTransport.hpp>
#include <protocol/PacketParser.hpp>
#include <protocol/Packet.hpp>
#include <protocol/PacketEncoder.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <termios.h> 
#include <cstring>