# BKEL SOME/IP Gateway 프로젝트 정리 (Portfolio)

이 프로젝트는 **임베디드 제어 로직을 네트워크 서비스로 전환**해, STM32 ECU와 외부 클라이언트를 안정적으로 연결하는 통신 플랫폼을 구현한 사례입니다.  
MCU 펌웨어, Linux C++ 게이트웨이, Python GUI 클라이언트를 하나의 체계로 엮어 **기획-설계-구현-검증**을 End-to-End로 수행했습니다.  
UART와 TCP/TLS를 아우르는 브리지 구조 위에 세션 라우팅, 프레임 파싱, CRC 검증을 적용해 **현장 운용 가능한 신뢰성**을 확보했습니다.  
즉, 단순 PoC를 넘어 하드웨어 제어(LED/PWM/SPI/ADC/GPIO)와 운영 도구(UI/로그)를 함께 완성한 **실무형 풀스택 임베디드 프로젝트**입니다.

## 1) 프로젝트 한 줄 소개
- **목표**: 리소스 제한이 있는 MCU 환경에서 SOME/IP 유사 커스텀 프로토콜 기반의 **RPC + 진단 통신 시스템**을 구현
- **구성**: `MCU(STM32)` - `Gateway Bridge(Raspberry Pi/Linux C++)` - `Client App(Python GUI)` 3계층 아키텍처
- **통신 경로**: `MCU <-> UART <-> Gateway <-> TCP/TLS <-> Client`

---

## 2) 전체 구조와 역할 분담

### A. `app` (Gateway Bridge, C++/Linux)
**역할**
- MCU(UART)와 외부 클라이언트(TCP/TLS) 사이를 연결하는 **프로토콜 브리지/라우터**
- 세션 관리, 프레임 파싱/인코딩, 멀티스레드 송수신 처리

**핵심 세부 요소**
- **전송계층 분리 설계**
  - `UartTransport`: MCU와 직렬 통신 (RX/TX worker thread)
  - `TlsTransport`: 클라이언트와 TLS 소켓 통신 (readAll로 프레임 단위 수신 안정성 확보)
- **세션/라우팅 처리**
  - `Session`, `SessionManager`로 CID 기반 세션 라이프사이클 관리
  - 요청 SID 큐를 기반으로 UART 수신 데이터를 클라이언트에 재분배
  - 브로드캐스트 SID(0x01 서비스 광고) 전체 세션 전파
- **프로토콜 처리**
  - 공통 프레임 구조: `SOF | SID | TYPE | DLC | PAYLOAD | CID | CRC`
  - `PacketParser`에서 CRC 검증/프레임 resync 처리
  - `PacketEncoder`로 UART/TLS 공통 직렬화
- **운영성**
  - JSON 기반 TCP 설정(`ip/port/max_connections/timeout`)
  - 테스트용 명령 메뉴(`CommandService`)로 LED/Reset/SPI/Diag 시나리오 즉시 검증 가능

**부각할 장점**
- **브리지 아키텍처 완성도**: 임베디드-네트워크 경계를 명확히 분리한 실전형 구조
- **확장성**: Transport 인터페이스/모듈화로 UART->CAN 등 링크 교체가 용이
- **안정성 고려**: CRC 검증, 비정상 DLC 필터링, 프레임 재동기화, 세션 단위 관리

---

### B. `clientapp` (TestPC Client, Python/Tkinter)
**역할**
- 게이트웨이에 접속해 MCU 서비스 상태를 시각화하고 RPC/진단 요청을 보낼 수 있는 **운영/검증용 도구**

**핵심 세부 요소**
- **GUI 기반 운용 인터페이스**
  - MCU 목록(CID), 서비스 목록(SID), 페이로드 입력, 통신 로그, 진단 히스토리 분리
  - 더블클릭 기반 서비스 호출 UX
- **네트워크/TLS 클라이언트**
  - `TcpClient`에서 TLS 소켓 연결, 백그라운드 수신 스레드 운영
  - CID별 시퀀스 관리(4-bit seq)
- **프로토콜 코덱**
  - `FrameCodec`에서 encode/decode, 스트림 버퍼에서 프레임 단위 파싱
  - 서비스 광고(SID 0x01) 수신 시 동적 서비스 목록 구성
- **관측성과 디버깅 편의**
  - Parsed View / Raw Hex View 전환
  - 오프라인 타임아웃 기반 장치 상태(ONLINE/OFFLINE) 표시
  - DIAG 데이터(ADC, PWM, GPIO)를 사람이 읽기 쉬운 형태로 변환

**부각할 장점**
- **개발 생산성**: 펌웨어/게이트웨이와 병행 개발 시 즉시 검증 가능한 테스트 프론트엔드
- **현장 친화성**: 단순 패킷 송수신이 아닌 상태 기반 UI와 로그 히스토리 제공
- **프로토콜 가시화**: Raw/Parsed 동시 지원으로 문제 분석 시간이 크게 단축

---

### C. `someip_gateway_mcu_stm32f103rb` (STM32 MCU Firmware, C/FreeRTOS)
**역할**
- RPC 요청 실행, 진단 데이터 생성/응답, 하드웨어 제어를 담당하는 **실제 ECU 측 로직**

**핵심 세부 요소**
- **RTOS 기반 태스크 구조**
  - 시스템 초기화 후 FreeRTOS 스케줄러 구동
  - SID 범위에 따라 RPC/DIAG 태스크로 분기 notify
- **프로토콜 엔진**
  - 프레임 빌드/파싱, CRC8 검증, DLC 유효성 검사
  - 잘못된 프레임에 대한 재동기화 및 방어 로직
- **RPC 서비스 구현**
  - `0x10`: LD2 제어(ON/OFF/TOGGLE)
  - `0x11`: MCU Reset
  - `0x12`: SPI Read/Write
  - `0x13`: PWM 출력 설정(Period/Duty)
- **진단 서비스 구현**
  - `0x20~0x26`: PWM 출력/입력, ADC1/ADC2, GPIO/LED 상태 전송
  - 실제 HAL 레지스터/주변장치 연동(BSW 계층)

**부각할 장점**
- **하드웨어 밀착 구현력**: LED/PWM/SPI/ADC/GPIO를 통신 서비스로 추상화
- **실시간성/분리도**: FreeRTOS 태스크 notify 구조로 기능 경계 명확
- **임베디드 품질 관점**: SID-DLC 유효성 체크, CRC 검증, 최대 DLC 제한 등 방어적 설계

---

## 3) 포트폴리오에서 강조하면 좋은 공통 성과
- **End-to-End 오너십**: MCU 펌웨어부터 게이트웨이, PC GUI까지 전 구간을 단일 저장소에서 주도적으로 완성
- **프로토콜 아키텍처 역량**: SOME/IP 개념을 경량 커스텀 프레임으로 재해석해 제한 리소스 환경에 최적화
- **이기종 통합 실행력**: `C(임베디드) + C++(브리지) + Python(클라이언트)` 스택을 유기적으로 연결
- **확장 가능 설계**: UART->CAN 전환, 인증 체계 추가 등 제품화 관점의 다음 단계가 선명

---

## 4) 면접/소개용 짧은 멘트 예시
> "STM32 ECU의 제어·진단 기능을 네트워크 서비스로 확장하기 위해, UART-TCP/TLS 브리지 아키텍처를 직접 설계하고 MCU/게이트웨이/GUI 클라이언트를 End-to-End로 구현했습니다. 프로토콜 설계, 세션 라우팅, 실시간 하드웨어 제어, 운영용 가시화 도구까지 완성해 기술 깊이와 제품화 관점을 동시에 증명한 프로젝트입니다."

