#include "core/Engine.hpp"
#include "main.h"

/* Protocol 등 Main Funtion 처리 */
void Engine::Run()
{
    /* TCP 초기화 */
    TcpServer server = TcpServer{};
    /* UART 초기화 */
    UART uart = UART{};
}