#include "main.h"

/* Protocol 등 Main Funtion 처리 */
void Engine::Run()
{
    /* Config 로드 */
    Config::getInstance().load("config/config.json");

    /* TCP 초기화 */
    TcpServer::Get().startup();

    /* UART 초기화 */
    UART::Get();
    #ifdef COMMAND_TEST
    CommandService::Get().ShowMenu();
    #endif
  
}