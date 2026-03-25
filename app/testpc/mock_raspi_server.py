import random
import socket
import threading
import time
from typing import Optional

from .protocol import DIAG_SIDS, FrameCodec


class MockRaspiServer:
    def __init__(self, host: str = "127.0.0.1", port: int = 8888, cid: int = 0x101):
        self.host = host
        self.port = port
        self.cid = cid
        self.running = False
        self.server: Optional[socket.socket] = None
        self.client: Optional[socket.socket] = None
        self.seq = 0

    def start(self) -> None:
        self.running = True
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server.bind((self.host, self.port))
        self.server.listen(1)
        print(f"[MOCK] Listening on {self.host}:{self.port}")
        self.client, addr = self.server.accept()
        self.client.settimeout(0.3)
        print(f"[MOCK] Client connected: {addr}")
        threading.Thread(target=self._adv_loop, daemon=True).start()
        self._rx_loop()

    def _adv_loop(self) -> None:
        assert self.client is not None
        adv_services = [
            (0x10, "RPC_LD2_Control"),
            (0x11, "RPC_MCU_Reset"),
            (0x12, "RPC_SPI_ReadWrite"),
            (0x13, "RPC_PWM_SetOut"),
            (0x20, "Diag_PWM_Output_Value"),
            (0x21, "Diag_PWM_Input_Value"),
            (0x22, "Diag_ADC1_GetValue"),
            (0x23, "Diag_ADC2_GetValue"),
            (0x24, "Diag_GPO_PinState"),
            (0x25, "Diag_GPI_PinState"),
            (0x26, "Diag_LD2_PinState"),
        ]
        while self.running:
            # Service advertise is sent as a burst of multiple packets.
            for sid, name in adv_services:
                brief = f"0x{sid:02X}:{name}"
                pkt = FrameCodec.encode(0x01, 0x03, brief.encode("utf-8"), self.cid, self._next_seq())
                try:
                    self.client.sendall(pkt)
                except OSError:
                    self.running = False
                    break
                time.sleep(0.03)
            if not self.running:
                break
            time.sleep(5.0)

    def _rx_loop(self) -> None:
        assert self.client is not None
        buf = bytearray()
        while self.running:
            try:
                chunk = self.client.recv(2048)
                if not chunk:
                    break
                buf.extend(chunk)
                while True:
                    frame = FrameCodec.try_decode(buf)
                    if frame is None:
                        break
                    print(
                        f"[MOCK-RX] CID=0x{frame.cid:03X} SID=0x{frame.sid:02X} "
                        f"SEQ={frame.seq} PAYLOAD={frame.payload.hex(' ')}"
                    )
                    if frame.sid in DIAG_SIDS:
                        self._send_diag_response(frame.sid)
            except socket.timeout:
                continue
            except OSError:
                break
        self.stop()

    def _send_diag_response(self, sid: int) -> None:
        assert self.client is not None
        if sid in {0x20, 0x21}:
            payload = bytes([random.randint(0, 100), random.randint(1, 20)])
            data_type = 0x02
        elif sid in {0x22, 0x23}:
            payload = random.randint(0, 4095).to_bytes(2, "little")
            data_type = 0x02
        elif sid in {0x24, 0x25, 0x26}:
            payload = bytes([random.randint(0, 1)])
            data_type = 0x01
        else:
            payload = b""
            data_type = 0x01
        pkt = FrameCodec.encode(sid, data_type, payload, self.cid, self._next_seq())
        try:
            self.client.sendall(pkt)
            print(f"[MOCK-TX] DIAG SID=0x{sid:02X} PAYLOAD={payload.hex(' ')}")
        except OSError:
            self.running = False

    def _next_seq(self) -> int:
        self.seq = (self.seq + 1) & 0x0F
        return self.seq

    def stop(self) -> None:
        self.running = False
        if self.client:
            try:
                self.client.close()
            except OSError:
                pass
            self.client = None
        if self.server:
            try:
                self.server.close()
            except OSError:
                pass
            self.server = None
        print("[MOCK] Stopped")


def main() -> None:
    server = MockRaspiServer()
    try:
        server.start()
    except KeyboardInterrupt:
        pass
    finally:
        server.stop()


if __name__ == "__main__":
    main()
