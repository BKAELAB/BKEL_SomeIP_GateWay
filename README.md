# BKEL_SomeIP_GateWay

- MCU F/W Architecture
<img width="956" height="782" alt="image" src="https://github.com/user-attachments/assets/4cd000f9-aa29-4725-988b-1b371489c427" />

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
