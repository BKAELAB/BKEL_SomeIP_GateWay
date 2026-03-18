#include "main.h"

/* Protocol 등 Main Funtion 처리 */
void Engine::Run()
{
    /* TCP 초기화 */

    /* UART 초기화 */
    UART::Get();
    #ifdef COMMAND_TEST
    CommandService::Get().ShowMenu();
    #endif
  
}