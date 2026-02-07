#include "main.h"

#define BUF_SIZE 1024
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

    if (argc < 3) {
        std::cout 
            << "사용법:\n"
            << "./bkel_gateway server <PORT>\n"
            << "./bkel_gateway client <IP> <PORT>\n";
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "server") {
        uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
        return RunServer(port);
    } 
    if (mode == "client") {
        std::string ip = argv[2];
        uint16_t port = static_cast<uint16_t>(std::stoi(argv[3]));
        return RunClient(ip, port);
    }
}


int RunServer(uint16_t port)
{
    TcpTransport server;
    std::cout << "[서버] " << port << "번 포트 여는 중..." << std::endl;

    if (!server.Listen("0.0.0.0", port, 5)) return -1; // 접속 올 때까지 대기

    std::cout << "[서버] 접속 대기 중..." << std::endl;
    auto client_conn = server.Accept(-1); // 연결됐을 때 새로운 TcpTransport 객체 client_conn 생성

    if (!client_conn) {
        std::cerr << "[서버] Accept 실패\n";
        return 1; 
    }
    std::cout << "[서버] 클라이언트 접속함!\n" << std::endl;

    std::vector<uint8_t> buf(BUF_SIZE);
    while (true) {
        int len = client_conn->RecvData(buf.data(), buf.size());
        if (len == 0) {
            std::cout << "[서버] 클라이언트 연결 끊김\n";
            break;
        }
        if (len < 0) {
            std::cerr << "[서버] Recv Error\n";
            break;
        }

        int sent = client_conn->SendData(buf.data(), (size_t)len);
        if (sent < 0) {
            std::cerr << "[서버] Sent Error\n";
            break;
        }
    }
    return 0;  
}

int RunClient(const std::string& ip, uint16_t port)
{
    TcpTransport clnt;

    // --- 클라이언트 모드 ---
    if (!clnt.Connect(ip, port)) {
        std::cout << "[클라이언트] 접속 실패!" << std::endl;
        return -1;
    }

    std::cout << "[클라이언트] 서버 접속 시도 " << ip << ":" << port << std::endl;
    std::cout << "=== Type Msg and press Enter (q or Q to quit) ===\n";

    std::string msgLine;
    std::vector<uint8_t> recvBuf(BUF_SIZE);
    std::string acc; // 누적 수신 버퍼

    while (true) {
        std::cout << "[SEND] ";
        std::cout.flush();

        if (!std::getline(std::cin, msgLine)) break;
        if (msgLine == "q" || msgLine == "Q") break;
        
        msgLine.push_back('\n');

        int sent = clnt.SendData(reinterpret_cast<const uint8_t*>(msgLine.data()), msgLine.size());
        if (sent < 0) {
            std::cerr << "[클라이언트] Send Error\n";
            break;
        }

        // 메시지 받기
        while (acc.find('\n') == std::string::npos) {
            int rev_len = clnt.RecvData(recvBuf.data(), recvBuf.size());
            if (rev_len == 0) {
                std::cout << "[클라이언트] 서버 닫힘\n";
                return 0;
            }

            if (rev_len < 0) {
                std::cerr << "[클라이언트] Recv Error!\n";
                return -1;
            }
            acc.append(reinterpret_cast<const char*>(recvBuf.data()), (size_t)rev_len);
        }

        size_t pos = acc.find('\n');
        std::string line = acc.substr(0, pos + 1);
        acc.erase(0, pos + 1);

        std::cout << "[ECHO] " << line;
        std::cout.flush();
    }
    return 0;
}