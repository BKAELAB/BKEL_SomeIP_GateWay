from dataclasses import dataclass, field
from typing import Dict


@dataclass
class McuInfo:
    cid: int
    last_seen: float
    brief: str = ""
    services: Dict[int, str] = field(default_factory=dict)
