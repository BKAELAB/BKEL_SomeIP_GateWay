#include "main.h"

int main(int argc, char* argv[])
{   
    /* Config 로드 */    
    Config::getInstance().load("config/config.json");

    /* Main Function */
    Engine::Get().Run();
    
    /* Loop */
    while(1);

    return 0;
}

