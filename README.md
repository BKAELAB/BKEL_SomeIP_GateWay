# BKEL_SomeIP_GateWay

- 본 저장소는, SOME/IP를 모사하여 제작한 커스텀 프로토콜을 사용합니다.
- 제한적인 리소스를 가진 MCU 내부에서 RPC, 진단통신을 지원합니다.
- 개인적인 스터디 모임에서 제작한 관계로 MCU - 브릿지 사이의 통신은 UART로 채택하였습니다. (CAN 으로 확장성 염두)
- 대략적인 통신흐름은 다음과 같습니다.
  
  > [MCU] <--UART--> [BRIDGE] <--TCP(TLS)--> [CLIENT]

## Source Code
  1. [STM32F103RB F/W](https://github.com/BKAELAB/BKEL_SomeIP_GateWay/tree/mcu) : move to mcu branch
  2. [Gateway Bridge APP](https://github.com/BKAELAB/BKEL_SomeIP_GateWay/tree/raspiapp) : move to gateway branch
     
## Communication Concept
  1. CLIENT accept to Well-Known IP (Ref. DDS)
  2. MCU Send ALL Service ID&Info via Gateway

## Features
  1. RPC (Remote Procedure Call)
  2. Diagnosis

## To be update
  1. To prepare for expandability, the existing UART communication method will be **replaced with a CAN BUS configuration.**
  2. Client authentication system scheduled to be introduced for security

## Docs
- [WIKIPAGE](https://github.com/BKAELAB/tsw_bringup_f103rb/wiki)
