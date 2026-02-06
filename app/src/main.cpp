#include "main.h"

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
//         }
//         parser.push(rxBuf, (size_t)n);
//         fflush(stdout);
//     }
   
//     return 0;
// }

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "사용법: ./bkel_gateway server  또는  ./bkel_gateway client" << std::endl;
        return 0;
    }

    std::string mode = argv[1];
    TcpTransport node;

    if (mode == "server") {
        // --- 서버 모드 ---
        std::cout << "[서버] 9000번 포트 여는 중..." << std::endl;
        if (!node.Listen("0.0.0.0", 9000, 5)) return -1;

        std::cout << "[서버] 접속 대기 중..." << std::endl;
        auto client_conn = node.Accept(-1); // 접속 올 때까지 대기

        if (client_conn) {
            std::cout << "[서버] 클라이언트 접속함!" << std::endl;
            uint8_t buf[1024];
            int len = client_conn->RecvData(buf, sizeof(buf));
            if (len > 0) {
                std::cout << "[서버] 받은 메시지: " << std::string((char*)buf, len) << std::endl;
                client_conn->SendData((uint8_t*)"Hi Client!", 10);
            }
        }
    } 
    else if (mode == "client") {
        // --- 클라이언트 모드 ---
        std::cout << "[클라이언트] 서버 접속 시도 (127.0.0.1:9000)..." << std::endl;
        if (!node.Connect("127.0.0.1", 9000)) {
            std::cout << "[클라이언트] 접속 실패!" << std::endl;
            return -1;
        }

        std::cout << "[클라이언트] 접속 성공! 메시지 보냄..." << std::endl;
        node.SendData((uint8_t*)"Hello Server!", 13);

        uint8_t buf[1024];
        int len = node.RecvData(buf, sizeof(buf));
        if (len > 0) {
            std::cout << "[클라이언트] 서버 응답: " << std::string((char*)buf, len) << std::endl;
        }
    }

    return 0;
}