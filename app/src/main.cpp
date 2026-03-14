#include "main.h"

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

