#include "core/Engine.hpp"

/* Protocol 등 Main Funtion 처리 */
void Engine::Run()
{
    /* TCP 초기화 */
    TcpServer server;
    server.startup();

    /* UART 초기화 */
    // UART uart = UART{};

    while(1);

}