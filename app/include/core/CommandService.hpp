#ifndef COMMAND_SERVICE_HPP
#define COMMAND_SERVICE_HPP

#include <vector>
#include <cstdint>

class CommandService {
public:
    // 싱글톤 인스턴스 가져오기
    static CommandService& Get();

    // Req-B-41 ~ 43: 기능 및 테스트
    void ControlLed(uint8_t mode);
    void ResetMcu(uint8_t mode);
    void RequestSpiLoopback(uint8_t type, const std::vector<uint8_t>& data);

    // Req-B-44: 진단 데이터 관련 (0x20 ~ 0x26)
    void RequestDiagnostic(uint8_t sid); // 개별 요청 전송
    void RunDiagnosticTest();           // 0x20~0x26 순차적 자동 요청
    void PerformFullDiagnostic();       // 전체 시나리오 실행 (선택사항)

private:
    // 생성자 숨기기 (싱글톤)
    CommandService() : currentCid(0) {}
    
    // 복사 및 할당 금지 (객체지향 안전장치)
    CommandService(const CommandService&) = delete;
    CommandService& operator=(const CommandService&) = delete;

    uint16_t currentCid;
};

#endif // COMMAND_SERVICE_HPP