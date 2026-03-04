#include <iostream>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <protocol/PacketParser.hpp>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "connect failed" << std::endl;
        return -1;
    }
    std::cout << "[Client] Connected!" << std::endl;

    // CID 전송 (nc에서 1023 입력한 것과 동일)
    std::string cid = "1023\n";
    send(sock, cid.c_str(), cid.size(), 0);
    std::cout << "[Client] CID sent" << std::endl;
    
    PacketParser parser;
    parser.setCallback([](const BKEL_Frame& frame) {
        std::cout << "[Client] Frame received!" << std::endl;
        std::cout << "  SID : 0x" << std::hex << (int)frame.sid << std::endl;
        std::cout << "  TYPE: 0x" << std::hex << (int)frame.type << std::endl;
        std::cout << "  DLC : " << std::dec << frame.dlc << std::endl;
        std::cout << "  CID : " << frame.cid << std::endl;
        std::cout << "  PAYLOAD: ";
        for (auto b : frame.payload)
            std::cout << std::hex << (int)b << " ";
        std::cout << std::endl;
    });

    uint8_t buf[1024];
    while (true) {
        ssize_t n = recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) {
            std::cout << "[Client] Disconnected" << std::endl;
            break;
        }
        parser.push(buf, n);
    }

    close(sock);
    return 0;
}