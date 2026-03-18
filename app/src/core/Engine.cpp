#include "main.h"

/* Protocol 등 Main Funtion 처리 */
void Engine::Run()
{
    /* TCP 초기화 */
    TcpServer server = TcpServer{};
    /* UART 초기화 */
    UART uart = UART{};
    static PacketParser parser;
    // 1. 인프라 준비 (Engine의 책임)
    if (!UART::Get().start(&parser)) {
        std::cerr << "[ERROR] 통신 장치를 깨울 수 없습니다." << std::endl;
        return; 
    }

    // 2. 비즈니스 로직 실행 (Service에게 하청)
    // "장치가 열렸으니, 이제 진단 패킷 7종 세트(0x20~0x26)를 쏴라!"
    CommandService::Get().RunDiagnosticTest();
}