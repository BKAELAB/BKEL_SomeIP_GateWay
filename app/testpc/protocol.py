import struct
import time
from dataclasses import dataclass, field
from typing import Dict, Optional

SOF = 0xAA
HEADER_FMT = ">BBH"  # SID(1), DataType(1), DLC(2)
HEADER_SIZE = struct.calcsize(HEADER_FMT)
TAIL_SIZE = 3  # CID(2) + CRC8(1)
MAX_PAYLOAD = 4096

SID_INFO: Dict[int, str] = {
    0x01: "Service Advertise",
    0x10: "RPC_LD2_Control",
    0x11: "RPC_MCU_Reset",
    0x12: "RPC_SPI_ReadWrite",
    0x13: "RPC_PWM_SetOut",
    0x20: "Diag_PWM_Output_Value",
    0x21: "Diag_PWM_Input_Value",
    0x22: "Diag_ADC1_GetValue",
    0x23: "Diag_ADC2_GetValue",
    0x24: "Diag_GPO_PinState",
    0x25: "Diag_GPI_PinState",
    0x26: "Diag_LD2_PinState",
}

RPC_SIDS = {0x10, 0x11, 0x12, 0x13}
DIAG_SIDS = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26}


def crc8_placeholder(data: bytes) -> int:
    # Prototype placeholder only. Receiver discards CRC byte.
    crc = 0
    for b in data:
        crc ^= b
    return crc & 0xFF


@dataclass
class Frame:
    sid: int
    data_type: int
    payload: bytes
    cid_raw: int
    crc8: int
    timestamp: float = field(default_factory=time.time)

    @property
    def cid(self) -> int:
        return (self.cid_raw >> 4) & 0x0FFF

    @property
    def seq(self) -> int:
        return self.cid_raw & 0x000F


class FrameCodec:
    @staticmethod
    def encode(sid: int, data_type: int, payload: bytes, cid: int, seq: int) -> bytes:
        if len(payload) > MAX_PAYLOAD:
            raise ValueError("Payload too large")
        cid_raw = ((cid & 0x0FFF) << 4) | (seq & 0x0F)
        body = struct.pack(HEADER_FMT, sid, data_type, len(payload)) + payload + struct.pack(">H", cid_raw)
        crc = crc8_placeholder(bytes([SOF]) + body)
        return bytes([SOF]) + body + bytes([crc])

    @staticmethod
    def try_decode(buffer: bytearray) -> Optional[Frame]:
        while buffer and buffer[0] != SOF:
            del buffer[0]

        if len(buffer) < 1 + HEADER_SIZE + TAIL_SIZE:
            return None

        sid, data_type, dlc = struct.unpack_from(HEADER_FMT, buffer, 1)
        if dlc > MAX_PAYLOAD:
            del buffer[0]
            return None

        frame_len = 1 + HEADER_SIZE + dlc + TAIL_SIZE
        if len(buffer) < frame_len:
            return None

        raw = bytes(buffer[:frame_len])
        del buffer[:frame_len]
        payload_start = 1 + HEADER_SIZE
        payload = raw[payload_start : payload_start + dlc]
        cid_raw = struct.unpack(">H", raw[payload_start + dlc : payload_start + dlc + 2])[0]
        recv_crc = raw[-1]  # Discarded for now (only consumed).
        return Frame(sid=sid, data_type=data_type, payload=payload, cid_raw=cid_raw, crc8=recv_crc)
