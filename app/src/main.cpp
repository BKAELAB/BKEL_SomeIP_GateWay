#include "main.h"

int main(int argc, char* argv[])
{   
    /* Main Function */
    Engine::Get().Run();

    /* TCP 초기화 */
    TcpServer server = TcpServer{};
    /* UART 초기화 */
    UART uart = UART{};
    
    /* Loop */
    while(1);

    return 0;
}

