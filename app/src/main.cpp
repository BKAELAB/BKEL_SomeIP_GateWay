#include "main.h"

// 클라이언트는 nc 대신 ./test_client <cid> 로
int main(int argc, char* argv[])
{   
    /* Core */
    Engine engine = Engine{};

    /* TCP 초기화 */
    TcpServer server = TcpServer{};

    /* UART 초기화 */
    UART uart = UART{};

    /* Loop */
    while(1);

    return 0;
}

