import os
import ssl
import socket
import threading
from queue import Queue
from typing import Dict, Optional

from .protocol import FrameCodec


class TcpClient:
    def __init__(self, host: str, port: int, incoming: Queue, status: Queue):
        self.host = host
        self.port = port
        self.incoming = incoming
        self.status = status
        self.sock: Optional[socket.socket] = None
        self.running = False
        self.reader: Optional[threading.Thread] = None
        self.seq_by_cid: Dict[int, int] = {}

    def connect(self) -> None:
        self.sock = socket.create_connection((self.host, self.port), timeout=3.0)

        # TLS
        # clientapp/config/ 경로에 server.crt 파일 추가 후 실행
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        ctx.load_verify_locations(os.path.join(BASE_DIR, "config", "server.crt"))
        ctx.check_hostname = False

        self.sock = ctx.wrap_socket(self.sock)

        self.sock.settimeout(0.5)
        self.running = True
        self.reader = threading.Thread(target=self._reader_loop, daemon=True)
        self.reader.start()
        self.status.put(f"Connected: {self.host}:{self.port}")

    def close(self) -> None:
        self.running = False
        if self.sock:
            try:
                self.sock.close()
            except OSError:
                pass
            self.sock = None
        self.status.put("Disconnected")

    def next_seq(self, cid: int) -> int:
        curr = self.seq_by_cid.get(cid, -1)
        curr = (curr + 1) & 0x0F
        self.seq_by_cid[cid] = curr
        return curr

    def send(self, frame: bytes) -> None:
        if not self.sock:
            raise ConnectionError("Not connected")
        self.sock.sendall(frame)

    def _reader_loop(self) -> None:
        assert self.sock is not None
        buf = bytearray()
        while self.running:
            try:
                chunk = self.sock.recv(2048)
                if not chunk:
                    self.status.put("Connection closed by peer")
                    self.running = False
                    break
                buf.extend(chunk)
                while True:
                    frame = FrameCodec.try_decode(buf)
                    if frame is None:
                        break
                    self.incoming.put(frame)
            except socket.timeout:
                continue
            except OSError as exc:
                self.status.put(f"Socket error: {exc}")
                self.running = False
                break
