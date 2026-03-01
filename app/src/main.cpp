#include "main.h"
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
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        txTestThread.join();

    } catch (const std::exception& e) {
        std::cerr << "[Main] Exception: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "[Main] Exit" << std::endl;
    return 0;
}




// // ===== RX 처리 =====
// static void handle_uart_rx(UART& uart, PacketParser& parser)
// {
//     uint8_t rxBuf[512];
//     ssize_t n = uart.readData(rxBuf, sizeof(rxBuf));

//     if (n <= 0) {
//         printf("[RX] 응답 없음 (Timeout)\n");
//         return;
//     }

//     printf("[RX RAW] ");
//     for (ssize_t i = 0; i < n; i++)
//         printf("%02X ", rxBuf[i]);
//     printf("\n");

//     bool frame_found = false;

//     for (ssize_t i = 0; i < n; i++)
//     {
//         if (rxBuf[i] != 0xAA) continue;
//         if (i + 8 > n) break;

//         uint16_t payload_len =
//             (uint16_t)rxBuf[i + 3] |
//             ((uint16_t)rxBuf[i + 4] << 8);

//         if (payload_len > BKEL_MAX_PAYLOAD) continue;

//         size_t frame_len = 8u + payload_len;
//         if (i + (ssize_t)frame_len > n) continue;

//         const uint8_t* frame = &rxBuf[i];

//         uint8_t rx_crc   = frame[frame_len - 1];
//         uint8_t calc_crc = calc_crc8(frame + BKEL_SOF_SIZE,
//                                      BKEL_HDR_SIZE + payload_len + BKEL_CID_SIZE);

//         if (calc_crc != rx_crc) continue;

//         frame_found = true;

//         printf("[RX FRAME OK] ");
//         for (size_t j = 0; j < frame_len; j++)
//             printf("%02X ", frame[j]);
//         printf("\n");

//         parser.push(frame, frame_len);
//     }

//     if (!frame_found) {
//         printf("[RX] Data received but no valid BKEL frame detected\n");
//     }
// }

// static void dump_tx(const char* tag, const std::vector<uint8_t>& frame)
// {
//     printf("[TX] %s: ", tag);
//     for (uint8_t b : frame)
//         printf("%02X ", b);
//     printf("\n");
// }

// int main()
// {
//     UART uart("/dev/serial0", B115200);
//     PacketParser parser;

//     uint8_t  type    = 0x01;
//     uint16_t cid_val = 0x1200;

//     const uint8_t SID_RPC_LD2_CONTROL = 0x10;
//     const uint8_t SID_DIAG_LD2_STATE  = 0x26;

//     printf("\n=== RPC/DIAG End-to-End LED Test ===\n");

//     while (true)
//     {
//         // LED ON
//         uint8_t led_on = 0x01;

//         auto txOn = PacketEncoder::build_frame(
//             SID_RPC_LD2_CONTROL, type,
//             &led_on, 1, cid_val++
//         );

//         printf("\n[STEP 1] LED ON\n");
//         dump_tx("RPC LD2 ON", txOn);
//         uart.writeData(txOn.data(), txOn.size());

//         usleep(500000);
//         handle_uart_rx(uart, parser);

//         printf(">> LED ON 상태 유지 (2초)\n");
//         sleep(2);

//         //  DIAG 확인 (ON 상태 확인)
//         uint8_t dummy = 0x00;

//         auto txRead1 = PacketEncoder::build_frame(
//             SID_DIAG_LD2_STATE, type,
//             &dummy, 1, cid_val++
//         );

//         printf("\n[STEP 2] DIAG READ (ON 상태 확인)\n");
//         dump_tx("DIAG LD2 READ", txRead1);
//         uart.writeData(txRead1.data(), txRead1.size());

//         usleep(500000);
//         handle_uart_rx(uart, parser);

//         // LED OFF
//         uint8_t led_off = 0x00;

//         auto txOff = PacketEncoder::build_frame(
//             SID_RPC_LD2_CONTROL, type,
//             &led_off, 1, cid_val++
//         );

//         printf("\n[STEP 3] LED OFF\n");
//         dump_tx("RPC LD2 OFF", txOff);
//         uart.writeData(txOff.data(), txOff.size());

//         usleep(500000);
//         handle_uart_rx(uart, parser);

//         printf(">> LED OFF 상태 유지 (2초)\n");
//         sleep(2);

//         //  DIAG 확인 (OFF 상태 확인)
//         auto txRead2 = PacketEncoder::build_frame(
//             SID_DIAG_LD2_STATE, type,
//             &dummy, 1, cid_val++
//         );

//         printf("\n[STEP 4] DIAG READ (OFF 상태 확인)\n");
//         dump_tx("DIAG LD2 READ", txRead2);
//         uart.writeData(txRead2.data(), txRead2.size());

//         usleep(500000);
//         handle_uart_rx(uart, parser);

//         printf("\n==== 3초 후 반복 ====\n");
//         sleep(3);
//     }

//     return 0;
// }