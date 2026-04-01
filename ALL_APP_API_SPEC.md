# BKEL_SomeIP_GateWay 통합 API 명세서

## 1. 문서 목적
- 본 문서는 `app`, `clientapp`, `someip_gateway_mcu_stm32f103rb` 3개 애플리케이션의 외부 연동 API를 통합 정의한다.
- 여기서 API는 REST가 아닌, **커스텀 바이너리 프레임 기반 서비스 인터페이스(SID)** 및 주요 호출 함수를 의미한다.
- 대상 독자: 임베디드/게이트웨이/클라이언트 개발자, 검증 담당자, 유지보수 담당자.

---

## 2. 시스템/API 경계

## 2.1 End-to-End 경로
`ClientApp(Python)` <-> `Gateway app(C++ TLS/TCP)` <-> `MCU FW(STM32 UART)`

## 2.2 역할별 API 관점
- `clientapp`: 프레임 생성/송신, 수신 프레임 파싱, UI 기반 서비스 호출
- `app`: 세션 관리, 프레임 라우팅, UART-TLS 브리지
- `someip_gateway_mcu_stm32f103rb`: SID별 RPC/DIAG 실행 및 응답 프레임 생성

---

## 3. 공통 프로토콜 명세

## 3.1 프레임 포맷
`SOF(1) | SID(1) | TYPE(1) | DLC(2) | PAYLOAD(N) | CID(2) | CRC8(1)`

- `SOF`: `0xAA`
- `DLC`: Payload 길이 (`uint16`)
- `CID`: Client 식별자(상위 12bit) + Seq(하위 4bit)로 운용
- `CRC8`: 현재 프로토타입 기준 XOR placeholder 기반(클라이언트는 수신 시 소비만 하고 엄격 비교는 생략)

## 3.2 데이터 타입(TYPE)
- `0x01`: `uint8` 중심 payload
- `0x02`: `uint16` 중심 payload
- `0x03`: `char/string` payload (예: 서비스 광고 문자열)

## 3.3 엔디언
- 헤더/DLC/CID: Little Endian 기준 운용
- ADC(0x22/0x23) 등 `uint16` payload: Little Endian

---

## 4. 서비스 API (SID) 명세

## 4.1 공통 SID 맵
- `0x01`: Service Advertise (Broadcast)
- `0x10`: RPC_LD2_Control
- `0x11`: RPC_MCU_Reset
- `0x12`: RPC_SPI_ReadWrite
- `0x13`: RPC_PWM_SetOut
- `0x20`: Diag_PWM_Output_Value
- `0x21`: Diag_PWM_Input_Value
- `0x22`: Diag_ADC1_GetValue
- `0x23`: Diag_ADC2_GetValue
- `0x24`: Diag_GPO_PinState
- `0x25`: Diag_GPI_PinState
- `0x26`: Diag_LD2_PinState

## 4.2 RPC 상세

### SID 0x10 - LD2 제어
- **Request**
  - `TYPE`: `0x01`
  - `DLC`: `1`
  - `Payload[0]`:
    - `0x00`: OFF
    - `0x01`: ON
    - `0x02`: TOGGLE
- **Response**
  - 명시적 ACK 없음(장치 상태/진단값으로 간접 확인)

### SID 0x11 - MCU 리셋
- **Request**
  - `TYPE`: `0x01`
  - `DLC`: `1`
  - `Payload[0]`: `0x01` (reset)
- **Response**
  - 즉시 리셋 동작 수행, 연결 재수립 필요 가능

### SID 0x12 - SPI Read/Write
- **Request**
  - `TYPE`: `0x01`
  - `DLC`: `5`
  - `Payload[0]`: opcode (`0x00` Read, `0x01` Write)
  - `Payload[1..4]`: Write 시 TX 바이트(4byte), Read 시 dummy
- **Response**
  - 디버그 출력 중심, 필요 시 확장 응답 프레임 정의 가능

### SID 0x13 - PWM 설정
- **Request**
  - `TYPE`: `0x02`
  - `DLC`: `2`
  - `Payload[0]`: duty(0~100)
  - `Payload[1]`: period/freq 파라미터(kHz 기반 운용)
- **Response**
  - 명시적 ACK 없음(진단 SID 0x20/0x21로 확인 권장)

## 4.3 DIAG 상세

### SID 0x20 - PWM Output 상태
- **Request**: 진단 요청 프레임(더미 payload 허용)
- **Response**
  - `TYPE`: `0x02`
  - `DLC`: `2`
  - `Payload[0]`: duty
  - `Payload[1]`: period/freq

### SID 0x21 - PWM Input 상태
- **Response 형식**: `0x20`과 동일 (`duty`, `period/freq`)

### SID 0x22 - ADC1 값
- **Response**
  - `TYPE`: `0x02`
  - `DLC`: `2`
  - `Payload[0..1]`: ADC raw `uint16(LE)`

### SID 0x23 - ADC2 값
- **Response 형식**: `0x22`와 동일

### SID 0x24 - GPO Pin 상태
- **Response**
  - `TYPE`: `0x01`
  - `DLC`: `1`
  - `Payload[0]`: `0` Low / `1` High

### SID 0x25 - GPI Pin 상태
- **Response 형식**: `0x24`와 동일

### SID 0x26 - LD2 Pin 상태
- **Response 형식**: `0x24`와 동일

## 4.4 Service Advertise (SID 0x01)
- MCU/게이트웨이가 주기적으로 서비스 목록을 공지
- ClientApp은 해당 payload를 파싱해 MCU 리스트/서비스 리스트를 동적으로 갱신

---

## 5. 앱별 API 명세

## 5.1 `clientapp` API (Python)

### 5.1.1 네트워크 API (`testpc/network.py`)
- `TcpClient.connect()`: TLS 연결 수립, 수신 스레드 시작
- `TcpClient.close()`: 연결 종료
- `TcpClient.next_seq(cid) -> int`: CID별 4bit sequence 증가/관리
- `TcpClient.send(frame: bytes)`: 직렬화된 프레임 송신

### 5.1.2 프로토콜 API (`testpc/protocol.py`)
- `FrameCodec.encode(sid, data_type, payload, cid, seq) -> bytes`
- `FrameCodec.try_decode(buffer: bytearray) -> Optional[Frame]`
- `Frame.cid` / `Frame.seq`: `cid_raw` 분해 프로퍼티

### 5.1.3 GUI 서비스 호출 API (`testpc/gui.py`)
- `send_service_request(cid, sid)`: 선택 서비스 요청 송신
- `_send_client_cid_announce()`: `SID=0x30` 클라이언트 CID 공지
- `_handle_frame(frame)`: 광고/진단/일반 수신 프레임 처리
- `_diag_to_text()`, `_parsed_payload_text()`: 진단 데이터 해석/표시

---

## 5.2 `app` API (Gateway Bridge, C++)

### 5.2.1 엔진/런타임 API
- `Engine::Run()`: TCP 서버 및 UART 서비스 초기화
- `TcpServer::Get().startup()`: 클라이언트 수신 대기 시작

### 5.2.2 세션/라우팅 API (`core/SessionManager`)
- `addSession(cid, ip, transport)`: 세션 생성 및 transport 시작
- `removeSession(cid)`: 세션 제거
- `broadcast(frame)`: 모든 세션으로 프레임 전송
- `forwardToMcu(cid, reqFrame)`: 클라이언트 요청을 MCU 송신 큐로 전달
- `routeUartRxBySid(uartFrame)`: 수신 SID 기준 세션 응답 라우팅
- `popNextMcuFrame(out)`: UART 송신 대상 프레임 선택

### 5.2.3 Transport API
- `UartTransport`
  - `start()/stop()`
  - `writeData(const uint8_t* data, uint32_t len)`
  - `rxWorker()/txWorker()`
- `TlsTransport`
  - `start()/stop()`
  - `sendData(const std::vector<uint8_t>& data)`
  - `rxLoop()/txLoop()`

### 5.2.4 개발/검증 API (`CommandService`)
- `ControlLed(mode)`, `ResetMcu(mode)`, `RequestSpiRead()`, `RequestSpiWrite(data)`
- `RequestDiagnostic(sid)`, `RunDiagnosticTest()`

---

## 5.3 `someip_gateway_mcu_stm32f103rb` API (MCU FW)

### 5.3.1 프로토콜 API
- `build_frame(...)`: 응답/광고 프레임 생성
- `parse_packet(...)`: 수신 버퍼 파싱 및 프레임 디스패치

### 5.3.2 RPC 실행 API (`ASW/src/BKEL_APP_rpc.c`)
- `BKEL_RPC_LD2_Control(packet)`
- `BKEL_RPC_MCU_Reset(packet)`
- `BKEL_RPC_SPI_Read(packet)`
- `BKEL_RPC_PWM_Setout(packet)`

### 5.3.3 진단 송신 API (`ASW/src/BKEL_APP_sendDiagData.c`)
- `AppSendDiagPWMOut()`
- `AppSendDiagPWMIn()`
- `AppSendDiagADC1Val()`
- `AppSendDiagADC2Val()`
- `AppSendDiagGPOPinState()`
- `AppSendDiagGPIPinState()`
- `AppSendDiagLD2PinState()`

---

## 6. 오류/예외 처리 규칙(현행)
- SOF 불일치/깨진 프레임: 1바이트씩 버리며 재동기화
- 비정상 DLC: 해당 후보 프레임 폐기
- CRC 불일치: 프레임 무효 처리 후 재동기화
- 세션 미존재 CID: 요청 무시
- 연결 종료/소켓 오류: 상태 큐/로그로 전달 후 세션 정리

---

## 7. 버전 및 호환성 메모
- 본 명세는 현재 `main` 브랜치 구현 기준이다.
- 추후 UART->CAN 전환, 인증 계층 추가 시 전송/보안 섹션 업데이트가 필요하다.

