#include <iostream>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <protocol/PacketParser.hpp>
#include <protocol/PacketEncoder.hpp>

// 실행 방법: ./test_client cid(0~65535)

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << "<cid>" << std::endl;
        return -1;
    }
    uint16_t cid = static_cast<uint16_t>(std::stoul(argv[1]));

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "connect failed" << std::endl;
        return -1;
    }
    std::cout << "[Client] Connected!" << std::endl;


    // CID 등록 패킷 전송 (sid=0x30)
    // 서버 receivedCid()가 sid=0x30 을 기대함
    auto cidFrame = PacketEncoder::build_frame(0x30, 0x00, nullptr, 0, cid);
    send(sock, cidFrame.data(), cidFrame.size(), 0);
    std::cout << "[Client] CID register packet sent, CID=" << cid << std::endl;

    // 송신 스레드: 1초마다 패킷 반복 전송
    std::thread txThread([&]() {
        uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05};
        int seq = 0;
        while (true) {
            auto txData = PacketEncoder::build_frame(0x21, 0x01, payload, sizeof(payload), cid);
            if (send(sock, txData.data(), txData.size(), 0) <= 0) break;
            std::cout << "[Client] BKEL Frame sent, seq=" << seq++ << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    txThread.detach();

    // 서버 응답 수신
    PacketParser& parser = PacketParser::Get();
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

/*
    build_frame(uint8_t sid,
                uint8_t type,
                const uint8_t* payload,
                uint16_t payload_len,
                uint16_t cid);
*/