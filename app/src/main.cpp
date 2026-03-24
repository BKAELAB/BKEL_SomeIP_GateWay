#include "main.h"

int main(int argc, char* argv[])
{   
    /* Main Function */
    Engine::Get().Run();
    
    /* Loop */
    while(1);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}

