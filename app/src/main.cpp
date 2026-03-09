#include "main.h"
#include <unistd.h>
#include <cstdio>
#include <vector>
#include <csignal> // 테스트 용으로 추가
// #include <iostream>
// #include <vector>
// #include <unistd.h>
// #include <cstdio>
// #include <cstdint>

// int main(int argc, char* argv[])
// {
//     UART uart("/dev/serial0", B115200);
//     PacketParser parser;

//      // 싱글턴 SessionManager
//     SessionManager& mgr = SessionManager::getInstance();

//     // 파서가 프레임 완성하면 여기로 들어옴
//     parser.setCallback([&mgr](const BKEL_Frame& frame) {

//         // 세션 생성 및 최근 SID 기록(정보 담기)
//         // "UART" 대신 IP 주소 넣어야함
//         mgr.onFrameArrived(frame.cid, "UART", frame);

//         // Broad Cast / 1:1 통신 수행
//         if (frame.sid == 0x01) {
//             mgr.broadcast(frame);
//         } else {
//             mgr.sendToSession(frame.cid, frame);
//         }
//     });

//     uint8_t txBuf[128];
//     uint8_t rxBuf[128];

//     while (true)
//     {
//         ssize_t n = uart.readData(rxBuf, sizeof(rxBuf));
//         if (n > 0)
//         {
//             printf("[RAW RX %zd] ", n);
//             for (ssize_t i = 0; i < n; i++)
//                 printf("%02X ", rxBuf[i]);
//             printf("\n");

//             parser.push(rxBuf, (size_t)n);
//             fflush(stdout);
//         }
//     }
//     return 0;
// }
// TCP Server test
static std::atomic<bool> g_running(true);
static TcpServer* g_server = nullptr;

// --------
// 전송 함수
void send_command_packet(UART& uart, uint8_t sid, uint8_t type, uint8_t data, uint16_t cid) {
    auto frame = PacketEncoder::build_frame(sid, type, &data, 1, cid);
    
    printf("\n[TX] 전송 데이터: ");
    for(uint8_t b : frame) printf("%02X ", b);
    printf(" (SID: 0x%02X, CID: %u)\n", sid, cid);

    // 하위 계층의 전송 함수 호출
    uart.writeData(frame.data(), (uint32_t)frame.size());
}
// --------

void signalHandler(int signum) {
    std::cout << "\n[Main] Signal " << signum << " received, shutting down..." << std::endl;
    if (g_server) {
        g_server->shutdown();
    }
    g_running = false;
}
// 터미널에 아래와 같이 입력 
// nc 127.0.0.1 8080  
// cid 임의로 입력 후 test

int main(int argc, char* argv[])
{
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // kill

      // 통신 객체 및 파서 초기화
    UART uart(DEFAULT_UART_DEVICE, B115200);
    PacketParser parser;
    uint16_t cid_counter = 1000;
   
    // 호출 시 rxWorker 수신 스레드가 생성, 실시간 Rx 감시 시작
    uart.start(&parser);
    printf("UART 수신 스레드\n");

    // 임시 추가
    auto rxCallback = [](const std::string& cid, const std::vector<uint8_t>& data) {
        std::string msg(data.begin(), data.end());
        std::cout << "[RxHandler] CID=" << cid << " msg=" << msg << std::endl;
    };

    TcpServer server(8080, rxCallback);
    g_server = &server;

    try {
        server.startup();
        std::cout << "[Main] Server running. Press Ctrl+C to stop." << std::endl;

        // Tx 테스트
        std::thread txTestThread([&]() {
            // 10 초 대기
            //std::this_thread::sleep_for(std::chrono::seconds(10));

            // 클라이언트 연결될 때까지 대기
            while (g_running) {
                {
                    std::lock_guard<std::mutex> lock(server.getTransportsMutex());
                    if (!server.getTransports().empty()) break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            if (!g_running) return;

            std::string msg = "hello from Server!!\n";
            std::vector<uint8_t> data(msg.begin(), msg.end());
            std::cout << "[Main] Sending test Message.." << std::endl;
            server.sendToFirst(data);
        });

        // 메인 스레드 대기
        while (g_running) {
            // // [B-11 Tx 테스트] 5초마다 MCU에 명령 전송
            // send_command_packet(uart, 0x10, 0x01, 0x55, 1000);
            
            // // [B-10 Thread 테스트] 아래 대기 중에도 UART 로그 찍혀야 함
            // for(int i=0; i<5; i++) {
            //     if(!g_running) break;
            //     std::this_thread::sleep_for(std::chrono::seconds(2));
            // }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        txTestThread.join();

    } catch (const std::exception& e) {
        std::cerr << "[Main] Exception: " << e.what() << std::endl;
        return 1;
    }
   
    uart.stop();  // UART 스레드 종료
    std::cout << "[Main] Exit" << std::endl; 
    return 0;
}

// #include <cassert>
// #include <cstdio>

// // 테스트용 프레임 생성 함수
// static BKEL_Frame makeFrame(uint8_t sid, uint16_t cid)
// {
//     BKEL_Frame f{};
//     f.sid = sid;
//     f.cid = cid;
//     return f;
// }

// int main(void)
// {
//     // 싱글턴 테스트
//     SessionManager& mgr1 = SessionManager::getInstance();
//     SessionManager& mgr2 = SessionManager::getInstance();

//     assert(&mgr1 == &mgr2);
//     printf("[확인] 싱글턴: SessionManager는 하나의 인스턴스만 존재\n");

//     // 테스트 반복 실행 대비 초기화
//     mgr1.clearAllForTest();

//     // 수명관리 Add/Remove 테스트
//     mgr1.addSession(1001, "UART");
//     mgr1.addSession(1002, "UART");

//     assert(mgr1.sessionCount() == 2);
//     printf("[확인] 세션 생성 완료: 현재 세션 개수 = %zu\n",
//            mgr1.sessionCount());

//     mgr1.removeSession(1002);

//     assert(mgr1.sessionCount() == 1);
//     printf("[확인] 세션 제거 완료: 현재 세션 개수 = %zu\n",
//            mgr1.sessionCount());

//     // 최근 SID 저장 테스트
//     mgr1.onFrameArrived(1001, "UART", makeFrame(0x22, 1001));

//     assert(mgr1.lastSid(1001) == 0x22);
//     printf("[확인] 최근 SID 저장 성공: SID = 0x%02X\n",
//            mgr1.lastSid(1001));

//     // 1:1 통신 테스트
//     mgr1.sendToSession(1001, makeFrame(0x30, 1001));

//     assert(mgr1.clientQueueSize(1001) == 1);
//     printf("[확인] 1:1 통신 성공: CID=1001, Client 큐 크기 = %zu\n",
//            mgr1.clientQueueSize(1001));

//     // 브로드캐스트 테스트
//     mgr1.addSession(1002, "UART");
//     mgr1.broadcast(makeFrame(0x01, 0xFFFF));

//     assert(mgr1.clientQueueSize(1001) == 2);
//     assert(mgr1.clientQueueSize(1002) == 1);

//     printf("[확인] 브로드캐스트 성공: CID=1001 큐=%zu, CID=1002 큐=%zu\n",
//            mgr1.clientQueueSize(1001),
//            mgr1.clientQueueSize(1002));

//     // MCU로 보낼 패킷 목록 테스트
//     mgr1.enqueueToMcu(1001, makeFrame(0x55, 1001));

//     assert(mgr1.mcuQueueSize(1001) == 1);
//     printf("[확인] MCU 큐 적재 성공: CID=1001, MCU 큐 크기 = %zu\n",
//            mgr1.mcuQueueSize(1001));

//     printf("\n Req-B-24 / Req-B-25 테스트 모두 통과\n\n");

//     return 0;
// }
