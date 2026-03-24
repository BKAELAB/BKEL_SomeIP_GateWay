import time
import tkinter as tk
from queue import Empty, Queue
from tkinter import messagebox, ttk
from typing import Dict, Optional

from .model import McuInfo
from .network import TcpClient
from .protocol import DIAG_SIDS, RPC_SIDS, Frame, FrameCodec, SID_INFO


class PrototypeApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("BKEL TestPC Prototype")
        self.incoming_queue: Queue = Queue()
        self.status_queue: Queue = Queue()
        self.client: Optional[TcpClient] = None
        self.mcus: Dict[int, McuInfo] = {}
        self.selected_cid: Optional[int] = None

        self._build_ui()
        self.root.after(100, self._tick)

    def _build_ui(self) -> None:
        conn_frame = ttk.LabelFrame(self.root, text="TCP Connection (TestPC <-> Raspi)")
        conn_frame.pack(fill="x", padx=8, pady=6)

        ttk.Label(conn_frame, text="Host").grid(row=0, column=0, padx=4, pady=4)
        self.host_var = tk.StringVar(value="127.0.0.1")
        ttk.Entry(conn_frame, textvariable=self.host_var, width=18).grid(row=0, column=1, padx=4, pady=4)

        ttk.Label(conn_frame, text="Port").grid(row=0, column=2, padx=4, pady=4)
        self.port_var = tk.StringVar(value="8888")
        ttk.Entry(conn_frame, textvariable=self.port_var, width=8).grid(row=0, column=3, padx=4, pady=4)

        ttk.Button(conn_frame, text="Connect", command=self.connect).grid(row=0, column=4, padx=4, pady=4)
        ttk.Button(conn_frame, text="Disconnect", command=self.disconnect).grid(row=0, column=5, padx=4, pady=4)

        body = ttk.Frame(self.root)
        body.pack(fill="both", expand=True, padx=8, pady=6)

        mcu_frame = ttk.LabelFrame(body, text="MCU List (CID)")
        mcu_frame.pack(side="left", fill="both", expand=True, padx=(0, 4))
        self.mcu_list = tk.Listbox(mcu_frame, height=18)
        self.mcu_list.pack(fill="both", expand=True, padx=4, pady=4)
        self.mcu_list.bind("<Double-Button-1>", self.on_mcu_double_click)

        svc_frame = ttk.LabelFrame(body, text="Service List")
        svc_frame.pack(side="left", fill="both", expand=True, padx=(4, 0))
        self.service_list = tk.Listbox(svc_frame, height=18)
        self.service_list.pack(fill="both", expand=True, padx=4, pady=4)
        self.service_list.bind("<Double-Button-1>", self.on_service_double_click)

        log_frame = ttk.LabelFrame(self.root, text="Logs / DIAG Result")
        log_frame.pack(fill="both", expand=True, padx=8, pady=(0, 8))
        self.log = tk.Text(log_frame, height=12, state="disabled")
        self.log.pack(fill="both", expand=True, padx=4, pady=4)

    def connect(self) -> None:
        host = self.host_var.get().strip()
        try:
            port = int(self.port_var.get().strip())
        except ValueError:
            messagebox.showerror("Port Error", "Invalid port number.")
            return

        try:
            self.client = TcpClient(host, port, self.incoming_queue, self.status_queue)
            self.client.connect()
        except OSError as exc:
            messagebox.showerror("Connect Error", str(exc))
            self.client = None

    def disconnect(self) -> None:
        if self.client:
            self.client.close()
            self.client = None

    def on_mcu_double_click(self, _event) -> None:
        idx = self.mcu_list.curselection()
        if not idx:
            return
        line = self.mcu_list.get(idx[0])
        cid = int(line.split()[1], 16)
        self.selected_cid = cid
        self.refresh_service_list(cid)
        self._append_log(f"[UI] Selected MCU CID=0x{cid:03X}")

    def on_service_double_click(self, _event) -> None:
        if self.selected_cid is None:
            return
        idx = self.service_list.curselection()
        if not idx:
            return
        entry = self.service_list.get(idx[0])
        sid = int(entry.split()[0], 16)
        self.send_service_request(self.selected_cid, sid)

    def send_service_request(self, cid: int, sid: int) -> None:
        if not self.client:
            messagebox.showwarning("Not Connected", "Connect first.")
            return

        payload = self.default_payload_for_sid(sid)
        data_type = self.default_data_type_for_sid(sid)
        seq = self.client.next_seq(cid)
        try:
            data = FrameCodec.encode(sid=sid, data_type=data_type, payload=payload, cid=cid, seq=seq)
            self.client.send(data)
            self._append_log(f"[TX] CID=0x{cid:03X} SID=0x{sid:02X} SEQ={seq} Payload={payload.hex(' ')}")
        except Exception as exc:
            messagebox.showerror("Send Error", str(exc))

    @staticmethod
    def default_data_type_for_sid(sid: int) -> int:
        if sid in {0x13, 0x20, 0x21, 0x22, 0x23}:
            return 0x02
        return 0x01

    @staticmethod
    def default_payload_for_sid(sid: int) -> bytes:
        defaults = {
            0x10: b"\x02",
            0x11: b"\x01",
            0x12: b"\x00\x00\x00\x00\x00",
            0x13: b"\x32\x0A",
            0x20: b"\x00\x00",
            0x21: b"\x00\x00",
            0x22: b"\x00\x00",
            0x23: b"\x00\x00",
            0x24: b"\x00",
            0x25: b"\x00",
            0x26: b"\x00",
        }
        return defaults.get(sid, b"")

    def refresh_mcu_list(self) -> None:
        self.mcu_list.delete(0, tk.END)
        for cid, info in sorted(self.mcus.items()):
            age = int(time.time() - info.last_seen)
            label = f"CID 0x{cid:03X}  (last {age}s)  {info.brief}"
            self.mcu_list.insert(tk.END, label)

    def refresh_service_list(self, cid: int) -> None:
        self.service_list.delete(0, tk.END)
        mcu = self.mcus.get(cid)
        if not mcu:
            return
        for sid, name in sorted(mcu.services.items()):
            kind = "RPC" if sid in RPC_SIDS else "DIAG" if sid in DIAG_SIDS else "OTHER"
            self.service_list.insert(tk.END, f"0x{sid:02X} [{kind}] {name}")

    def _append_log(self, text: str) -> None:
        self.log.configure(state="normal")
        self.log.insert(tk.END, f"{time.strftime('%H:%M:%S')} {text}\n")
        self.log.see(tk.END)
        self.log.configure(state="disabled")

    def _handle_frame(self, frame: Frame) -> None:
        if frame.sid == 0x01:
            brief = frame.payload.decode("utf-8", errors="replace").strip()
            mcu = self.mcus.get(frame.cid)
            if not mcu:
                mcu = McuInfo(cid=frame.cid, last_seen=frame.timestamp)
                self.mcus[frame.cid] = mcu
            mcu.last_seen = frame.timestamp
            mcu.brief = brief
            self._add_services_from_brief(mcu, brief)
            self.refresh_mcu_list()
            if self.selected_cid == frame.cid:
                self.refresh_service_list(frame.cid)
            self._append_log(f"[RX-ADV] CID=0x{frame.cid:03X} SEQ={frame.seq} '{brief}'")
            return

        sid_name = SID_INFO.get(frame.sid, "Unknown")
        self._append_log(
            f"[RX] CID=0x{frame.cid:03X} SID=0x{frame.sid:02X}({sid_name}) "
            f"SEQ={frame.seq} Payload={frame.payload.hex(' ')} CRC=0x{frame.crc8:02X}"
        )
        if frame.sid in DIAG_SIDS:
            self._append_log(f"[DIAG] {self._diag_to_text(frame.sid, frame.payload)}")

    def _add_services_from_brief(self, mcu: McuInfo, brief: str) -> None:
        found = set()
        parts = brief.replace(";", ",").replace("|", ",").split(",")
        for part in parts:
            p = part.strip().lower()
            if p.startswith("0x"):
                try:
                    found.add(int(p, 16))
                except ValueError:
                    pass
        if not found:
            found = set(RPC_SIDS | DIAG_SIDS)
        for sid in sorted(found):
            mcu.services[sid] = SID_INFO.get(sid, f"Service_0x{sid:02X}")

    @staticmethod
    def _diag_to_text(sid: int, payload: bytes) -> str:
        if sid in {0x20, 0x21} and len(payload) >= 2:
            duty, period = payload[0], payload[1]
            return f"SID=0x{sid:02X}: Duty={duty}, Period/Freq={period}"
        if sid in {0x22, 0x23} and len(payload) >= 2:
            val = int.from_bytes(payload[:2], "big")
            pct = (val / 4095.0) * 100.0
            return f"SID=0x{sid:02X}: ADC Raw={val}, Percent={pct:.2f}%"
        if sid in {0x24, 0x25, 0x26} and payload:
            return f"SID=0x{sid:02X}: PinState={'High' if payload[0] else 'Low'}"
        return f"SID=0x{sid:02X}: Payload={payload.hex(' ')}"

    def _tick(self) -> None:
        while True:
            try:
                msg = self.status_queue.get_nowait()
                self._append_log(f"[NET] {msg}")
            except Empty:
                break

        while True:
            try:
                frame = self.incoming_queue.get_nowait()
                self._handle_frame(frame)
            except Empty:
                break

        self.root.after(100, self._tick)


def run_gui() -> None:
    root = tk.Tk()
    root.geometry("980x680")
    app = PrototypeApp(root)
    root.protocol("WM_DELETE_WINDOW", lambda: (app.disconnect(), root.destroy()))
    root.mainloop()
