#include "main.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>

void test_req33_sid_memory()
{
    std::cout << "\n[테스트] Req-33 SID 저장 기능\n";

    SessionManager& mgr = SessionManager::getInstance();
    mgr.clearSessions();

    BKEL_Frame diag{};
    diag.sid = 0x22;
    diag.cid = 1001;

    mgr.onDiagRequestArrived(1001, "client1", diag);

    auto s = mgr.getSession(1001);

    if (s && s->getLastRequestedSid() == 0x22)
        std::cout << "성공: SID가 정상적으로 저장되었습니다.\n";
    else
        std::cout << "실패: SID 저장이 되지 않았습니다.\n";
}


void test_req31_routing()
{
    std::cout << "\n[테스트] Req-31 SID 기반 라우팅\n";

    SessionManager& mgr = SessionManager::getInstance();
    mgr.clearSessions();

    BKEL_Frame diag1{};
    diag1.sid = 0x22;
    diag1.cid = 1001;

    BKEL_Frame diag2{};
    diag2.sid = 0x23;
    diag2.cid = 1002;

    mgr.onDiagRequestArrived(1001, "client1", diag1);
    mgr.onDiagRequestArrived(1002, "client2", diag2);

    BKEL_Frame uartResp{};
    uartResp.sid = 0x22;

    mgr.routeUartRxBySid(uartResp);

    auto s1 = mgr.getSession(1001);
    auto s2 = mgr.getSession(1002);

    if (s1->clientQueueSize() == 1 && s2->clientQueueSize() == 0)
        std::cout << "성공: SID에 맞는 세션에만 패킷이 전달되었습니다.\n";
    else
        std::cout << "실패: SID 라우팅이 올바르지 않습니다.\n";
}


void test_req31_multiple_sessions()
{
    std::cout << "\n[테스트] Req-31 동일 SID 세션 여러개\n";

    SessionManager& mgr = SessionManager::getInstance();
    mgr.clearSessions();

    BKEL_Frame diag{};
    diag.sid = 0x22;

    mgr.onDiagRequestArrived(2001, "client3", diag);
    mgr.onDiagRequestArrived(2002, "client4", diag);

    BKEL_Frame uartResp{};
    uartResp.sid = 0x22;

    mgr.routeUartRxBySid(uartResp);

    auto s1 = mgr.getSession(2001);
    auto s2 = mgr.getSession(2002);

    if (s1->clientQueueSize() == 1 && s2->clientQueueSize() == 1)
        std::cout << "성공: 동일 SID를 가진 모든 세션에 패킷이 전달되었습니다.\n";
    else
        std::cout << "실패: 여러 세션 라우팅이 정상 동작하지 않습니다.\n";
}


void test_req35_uart_tx()
{
    std::cout << "\n[테스트] Req-35 UART 송신 큐\n";

    SessionManager& mgr = SessionManager::getInstance();
    mgr.clearSessions();

    BKEL_Frame frame{};
    frame.sid = 0x24;
    frame.cid = 3001;

    mgr.onDiagRequestArrived(3001, "client5", frame);

    BKEL_Frame out{};

    if (mgr.popNextMcuFrame(out))
    {
        if (out.sid == 0x24 && out.cid == 3001)
            std::cout << "성공: MCU로 보낼 패킷이 정상적으로 추출되었습니다.\n";
        else
            std::cout << "실패: 다른 패킷이 추출되었습니다.\n";
    }
    else
    {
        std::cout << "실패: MCU 송신 패킷이 존재하지 않습니다.\n";
    }
}

void test_blocked_session()
{
    std::cout << "\n[테스트] 차단된 세션 패킷 수신 방지\n";

    SessionManager& mgr = SessionManager::getInstance();
    mgr.clearSessions();

    BKEL_Frame diag{};
    diag.sid = 0x22;

    mgr.onDiagRequestArrived(4001, "clientA", diag);
    mgr.onDiagRequestArrived(4002, "clientB", diag);

    auto s1 = mgr.getSession(4001);
    auto s2 = mgr.getSession(4002);

    // 첫 번째 세션 차단
    s1->setBlocked(true);

    BKEL_Frame uartResp{};
    uartResp.sid = 0x22;

    mgr.routeUartRxBySid(uartResp);

    if (s1->clientQueueSize() == 0 && s2->clientQueueSize() == 1)
        std::cout << "성공: 차단된 세션은 패킷을 받지 않습니다\n";
    else
        std::cout << "실패: 차단된 세션이 패킷을 받았습니다.\n";
}

int main()
{
    test_req33_sid_memory();
    test_req31_routing();
    test_req31_multiple_sessions();
    test_req35_uart_tx();
    test_blocked_session();

    std::cout << "\n모든 테스트가 완료!!\n";

    return 0;
}