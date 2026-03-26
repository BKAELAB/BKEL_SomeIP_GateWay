import time
import tkinter as tk
from queue import Empty, Queue
from tkinter import messagebox, ttk
from typing import Dict, Optional

from .model import McuInfo
from .network import TcpClient
from .protocol import DIAG_PAYLOAD_UINT16_ORDER, DIAG_SIDS, RPC_SIDS, Frame, FrameCodec, SID_INFO

OFFLINE_TIMEOUT_SEC = 10


class PrototypeApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("BKEL TestPC Prototype")
        self.incoming_queue: Queue = Queue()
        self.status_queue: Queue = Queue()
        self.client: Optional[TcpClient] = None
        self.mcus: Dict[int, McuInfo] = {}
        self.selected_cid: Optional[int] = None
        self.selected_sid: Optional[int] = None
        self.log_mode = tk.StringVar(value="parsed")
        self.mcu_index_to_cid: Dict[int, int] = {}
        self._last_presence_refresh = 0.0

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

        ttk.Label(conn_frame, text="Client CID").grid(row=0, column=4, padx=4, pady=4)
        self.client_cid_var = tk.StringVar(value="0x700")
        ttk.Entry(conn_frame, textvariable=self.client_cid_var, width=10).grid(row=0, column=5, padx=4, pady=4)

        ttk.Button(conn_frame, text="Connect", command=self.connect).grid(row=0, column=6, padx=4, pady=4)
        ttk.Button(conn_frame, text="Disconnect", command=self.disconnect).grid(row=0, column=7, padx=4, pady=4)

        body = ttk.Frame(self.root)
        body.pack(fill="both", expand=True, padx=8, pady=6)

        mcu_frame = ttk.LabelFrame(body, text="MCU List (CID)")
        mcu_frame.pack(side="left", fill="both", expand=True, padx=(0, 4))
        self.mcu_list = tk.Listbox(mcu_frame, height=18)
        self.mcu_list.pack(fill="both", expand=True, padx=4, pady=4)
        self.mcu_list.bind("<<ListboxSelect>>", self.on_mcu_select)
        self.mcu_list.bind("<ButtonRelease-1>", self.on_mcu_select)
        self.mcu_list.bind("<Double-Button-1>", self.on_mcu_select)

        svc_frame = ttk.LabelFrame(body, text="Service List")
        svc_frame.pack(side="left", fill="both", expand=True, padx=(4, 4))
        self.service_list = tk.Listbox(svc_frame, height=18)
        self.service_list.pack(fill="both", expand=True, padx=4, pady=4)
        self.service_list.bind("<<ListboxSelect>>", self.on_service_select)
        self.service_list.bind("<Double-Button-1>", self.on_service_double_click)
        self.service_list.configure(state="disabled")

        payload_frame = ttk.LabelFrame(body, text="Payload Input")
        payload_frame.pack(side="left", fill="y", padx=(0, 0))
        ttk.Label(payload_frame, text="Selected Service").pack(anchor="w", padx=6, pady=(6, 2))
        self.selected_service_label = tk.StringVar(value="-")
        ttk.Label(payload_frame, textvariable=self.selected_service_label).pack(anchor="w", padx=6, pady=(0, 6))
        ttk.Label(payload_frame, text="Payload HEX (e.g. 01 AA FF)").pack(anchor="w", padx=6, pady=(0, 2))
        self.payload_hex_var = tk.StringVar(value="")
        ttk.Entry(payload_frame, textvariable=self.payload_hex_var, width=28).pack(anchor="w", padx=6, pady=(0, 6))
        ttk.Button(payload_frame, text="Send Selected Service", command=self.send_selected_service).pack(
            anchor="w", padx=6, pady=(0, 8)
        )
        ttk.Label(payload_frame, text="* empty payload means default").pack(anchor="w", padx=6, pady=(0, 6))

        bottom = ttk.Frame(self.root)
        bottom.pack(fill="both", expand=True, padx=8, pady=(0, 8))

        log_frame = ttk.LabelFrame(bottom, text="Communication Log")
        log_frame.pack(side="left", fill="both", expand=True, padx=(0, 4))
        mode_row = ttk.Frame(log_frame)
        mode_row.pack(fill="x", padx=4, pady=(4, 0))
        ttk.Label(mode_row, text="View").pack(side="left")
        ttk.Radiobutton(mode_row, text="Parsed Data", value="parsed", variable=self.log_mode).pack(side="left", padx=6)
        ttk.Radiobutton(mode_row, text="Hex Raw Packet", value="raw", variable=self.log_mode).pack(side="left")
        self.log = tk.Text(log_frame, height=12, state="disabled")
        self.log.pack(fill="both", expand=True, padx=4, pady=4)

        diag_frame = ttk.LabelFrame(bottom, text="DIAG Return History")
        diag_frame.pack(side="left", fill="both", expand=True, padx=(4, 0))
        self.diag_history = tk.Text(diag_frame, height=12, state="disabled")
        self.diag_history.pack(fill="both", expand=True, padx=4, pady=4)

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
            self._send_client_cid_announce()
        except OSError as exc:
            messagebox.showerror("Connect Error", str(exc))
            self.client = None

    def disconnect(self) -> None:
        if self.client:
            self.client.close()
            self.client = None

    def on_mcu_select(self, _event) -> None:
        idx = self.mcu_list.curselection()
        if not idx:
            return
        cid = self.mcu_index_to_cid.get(idx[0])
        if cid is None:
            return
        if self._is_offline(cid):
            self._append_log(f"[UI] {self._mcu_name(cid)} is offline (>10s no advertise).")
            self.selected_cid = None
            self.service_list.configure(state="disabled")
            return
        self.selected_cid = cid
        self.refresh_service_list(cid)
        self.service_list.configure(state="normal")
        self._append_log(f"[UI] Selected MCU CID=0x{cid:03X}")

    def on_service_select(self, _event) -> None:
        idx = self.service_list.curselection()
        if not idx:
            return
        entry = self.service_list.get(idx[0])
        sid = int(entry.split()[0], 16)
        self.selected_sid = sid
        self.selected_service_label.set(entry)

    def on_service_double_click(self, _event) -> None:
        self.send_selected_service()

    def send_selected_service(self) -> None:
        if self.selected_cid is None or self.selected_sid is None:
            return
        self.send_service_request(self.selected_cid, self.selected_sid)

    def send_service_request(self, cid: int, sid: int) -> None:
        if not self.client:
            messagebox.showwarning("Not Connected", "Connect first.")
            return

        payload = self.default_payload_for_sid(sid)
        payload_hex = self.payload_hex_var.get().strip()
        if payload_hex:
            try:
                payload = self._parse_hex_payload(payload_hex)
            except ValueError as exc:
                messagebox.showerror("Payload Error", str(exc))
                return

        data_type = self.default_data_type_for_sid(sid)
        seq = self.client.next_seq(cid)
        try:
            data = FrameCodec.encode(sid=sid, data_type=data_type, payload=payload, cid=cid, seq=seq)
            self.client.send(data)
            self._append_tx_log(cid=cid, sid=sid, seq=seq, payload=payload, raw=data, data_type=data_type)
        except Exception as exc:
            messagebox.showerror("Send Error", str(exc))

    def _send_client_cid_announce(self) -> None:
        if not self.client:
            return
        try:
            client_cid = self._parse_cid(self.client_cid_var.get().strip())
        except ValueError as exc:
            messagebox.showerror("Client CID Error", str(exc))
            return

        sid = 0x30
        data_type = 0x01
        payload = b""
        seq = self.client.next_seq(client_cid)
        try:
            packet = FrameCodec.encode(sid=sid, data_type=data_type, payload=payload, cid=client_cid, seq=seq)
            self.client.send(packet)
            self._append_log(f"[TX] CLIENT_CID_ANNOUNCE SID:0x30 CID:0x{client_cid:03X} SEQ:{seq}")
        except Exception as exc:
            messagebox.showerror("CID Announce Error", str(exc))

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
        prev_selected_cid: Optional[int] = None
        selected_idx = self.mcu_list.curselection()
        if selected_idx:
            prev_selected_cid = self.mcu_index_to_cid.get(selected_idx[0])

        self.mcu_list.delete(0, tk.END)
        self.mcu_index_to_cid.clear()
        row = 0
        selected_row: Optional[int] = None
        for cid, info in sorted(self.mcus.items()):
            age = int(time.time() - info.last_seen)
            offline = self._is_offline(cid)
            state_text = "OFFLINE" if offline else "ONLINE"
            label = f"{self._mcu_name(cid)} [{state_text}] (last {age}s)"
            self.mcu_list.insert(tk.END, label)
            if offline:
                self.mcu_list.itemconfig(row, fg="gray")
            self.mcu_index_to_cid[row] = cid
            if prev_selected_cid is not None and cid == prev_selected_cid:
                selected_row = row
            row += 1

        if selected_row is not None:
            self.mcu_list.selection_set(selected_row)
            self.mcu_list.activate(selected_row)

    def refresh_service_list(self, cid: int) -> None:
        prev_selected_sid = self.selected_sid
        self.service_list.delete(0, tk.END)
        mcu = self.mcus.get(cid)
        if not mcu:
            self.selected_sid = None
            self.selected_service_label.set("-")
            return
        selected_row: Optional[int] = None
        row = 0
        for sid, name in sorted(mcu.services.items()):
            kind = "RPC" if sid in RPC_SIDS else "DIAG" if sid in DIAG_SIDS else "OTHER"
            self.service_list.insert(tk.END, f"0x{sid:02X} [{kind}] {name}")
            if prev_selected_sid is not None and sid == prev_selected_sid:
                selected_row = row
            row += 1
        if selected_row is not None:
            self.service_list.selection_set(selected_row)
            self.service_list.activate(selected_row)
            self.selected_sid = prev_selected_sid
            entry = self.service_list.get(selected_row)
            self.selected_service_label.set(entry)
        else:
            self.selected_sid = None
            self.selected_service_label.set("-")

    def _append_log(self, text: str) -> None:
        self.log.configure(state="normal")
        self.log.insert(tk.END, f"{time.strftime('%H:%M:%S')} {text}\n")
        self.log.see(tk.END)
        self.log.configure(state="disabled")

    def _append_diag_history(self, text: str) -> None:
        self.diag_history.configure(state="normal")
        self.diag_history.insert(tk.END, f"{time.strftime('%H:%M:%S')} {text}\n")
        self.diag_history.see(tk.END)
        self.diag_history.configure(state="disabled")

    @staticmethod
    def _mcu_name(cid: int) -> str:
        return f"ECU_0x{cid:03X}"

    def _append_tx_log(self, cid: int, sid: int, seq: int, payload: bytes, raw: bytes, data_type: int) -> None:
        if self.log_mode.get() == "raw":
            self._append_log(f"[TX-RAW] {raw.hex(' ')}")
            return
        self._append_log(
            f"[TX] {self._mcu_name(cid)} SID:0x{sid:02X}({SID_INFO.get(sid, 'Unknown')}) "
            f"DataType:{data_type:#04x} DLC:{len(payload)} Payload:{self._parsed_payload_text(sid, payload)} "
            f"SEQ:{seq}"
        )

    def _append_rx_log(self, frame: Frame, prefix: str = "RX") -> None:
        if self.log_mode.get() == "raw":
            self._append_log(f"[{prefix}-RAW] {frame.raw_packet.hex(' ')}")
            return
        self._append_log(
            f"[{prefix}] {self._mcu_name(frame.cid)} SID:0x{frame.sid:02X}({SID_INFO.get(frame.sid, 'Unknown')}) "
            f"DataType:{frame.data_type:#04x} DLC:{len(frame.payload)} Payload:{self._parsed_payload_text(frame.sid, frame.payload)} "
            f"SEQ:{frame.seq} CRC:0x{frame.crc8:02X}"
        )

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
            self._append_rx_log(frame, prefix="RX-ADV")
            return

        self._append_rx_log(frame, prefix="RX")
        if frame.sid in DIAG_SIDS:
            self._append_diag_history(f"{self._mcu_name(frame.cid)} {self._diag_to_text(frame.sid, frame.payload)}")

    def _add_services_from_brief(self, mcu: McuInfo, brief: str) -> None:
        found = set()
        parts = brief.replace(";", ",").replace("|", ",").split(",")
        for part in parts:
            p = part.strip().lower()
            if p.startswith("0x"):
                try:
                    sid_token = p.split(":", 1)[0]
                    sid = int(sid_token, 16)
                    found.add(sid)
                    custom_name = part.strip().split(":", 1)
                    if len(custom_name) == 2 and custom_name[1].strip():
                        mcu.services[sid] = custom_name[1].strip()
                except ValueError:
                    pass
        if not found:
            return
        for sid in sorted(found):
            if sid not in mcu.services:
                mcu.services[sid] = SID_INFO.get(sid, f"Service_0x{sid:02X}")

    @staticmethod
    def _parsed_payload_text(sid: int, payload: bytes) -> str:
        if sid == 0x01:
            return payload.decode("utf-8", errors="replace")
        if sid in {0x10, 0x11, 0x24, 0x25, 0x26} and payload:
            return f"{payload[0]} (0x{payload[0]:02X})"
        if sid in {0x20, 0x21} and len(payload) >= 2:
            return f"Duty={payload[0]}, Period/Freq={payload[1]}"
        if sid in {0x22, 0x23} and len(payload) >= 2:
            raw = int.from_bytes(payload[:2], DIAG_PAYLOAD_UINT16_ORDER)
            return f"ADC={raw} ({(raw / 4095.0) * 100.0:.2f}%)"
        return payload.hex(" ")

    @staticmethod
    def _diag_to_text(sid: int, payload: bytes) -> str:
        if sid in {0x20, 0x21} and len(payload) >= 2:
            duty, period = payload[0], payload[1]
            return f"SID=0x{sid:02X}: Duty={duty}, Period/Freq={period}"
        if sid in {0x22, 0x23} and len(payload) >= 2:
            val = int.from_bytes(payload[:2], DIAG_PAYLOAD_UINT16_ORDER)
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

        now = time.time()
        if now - self._last_presence_refresh >= 1.0:
            self.refresh_mcu_list()
            self._last_presence_refresh = now
        if self.selected_cid is not None and self._is_offline(self.selected_cid):
            self._append_log(f"[NET] {self._mcu_name(self.selected_cid)} moved to offline state.")
            self.selected_cid = None
            self.service_list.configure(state="disabled")

        self.root.after(100, self._tick)

    def _is_offline(self, cid: int) -> bool:
        mcu = self.mcus.get(cid)
        if not mcu:
            return True
        return (time.time() - mcu.last_seen) > OFFLINE_TIMEOUT_SEC

    @staticmethod
    def _parse_hex_payload(payload_hex: str) -> bytes:
        cleaned = payload_hex.replace(",", " ").replace("-", " ")
        parts = [p for p in cleaned.split() if p]
        if not parts:
            return b""
        try:
            values = [int(p, 16) for p in parts]
        except ValueError as exc:
            raise ValueError("Payload must be hex bytes separated by spaces.") from exc
        if any(v < 0 or v > 0xFF for v in values):
            raise ValueError("Each payload byte must be 00..FF.")
        return bytes(values)

    @staticmethod
    def _parse_cid(text: str) -> int:
        if not text:
            raise ValueError("Client CID is required.")
        try:
            value = int(text, 16)
        except ValueError as exc:
            raise ValueError("Client CID must be hex (example: 700 or 0x700).") from exc
        if value < 0 or value > 0x0FFF:
            raise ValueError("Client CID must be in range 0x000..0xFFF.")
        return value


def run_gui() -> None:
    root = tk.Tk()
    root.geometry("980x680")
    app = PrototypeApp(root)
    root.protocol("WM_DELETE_WINDOW", lambda: (app.disconnect(), root.destroy()))
    root.mainloop()
