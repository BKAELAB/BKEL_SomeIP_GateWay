#include "main.h"

int main(int argc, char* argv[])
{   
    /* Config 로드 */    
    Config::getInstance().load("config/config.json");

    /* logger 초기화 */
    Logger::getInstance().open("logs/gateway.log");
    
    /* Main Function */
    Engine::Get().Run();
    
    /* Loop */
    while(1);

    return 0;
}

