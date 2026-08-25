#!/usr/bin/env python3
"""Small, classic-CAN SocketCAN helper used only by host integration tests."""
from __future__ import annotations

import select
import socket
import struct
from dataclasses import dataclass

_FRAME = struct.Struct("=IB3x8s")
CAN_EFF_FLAG = 0x80000000
CAN_RTR_FLAG = 0x40000000
CAN_ERR_FLAG = 0x20000000
CAN_SFF_MASK = 0x7FF


@dataclass(frozen=True)
class Frame:
    arbitration_id: int
    data: bytes


def open_socket(interface: str) -> socket.socket:
    sock = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
    sock.setsockopt(socket.SOL_CAN_RAW, socket.CAN_RAW_RECV_OWN_MSGS, 1)
    sock.bind((interface,))
    return sock


def send(sock: socket.socket, arbitration_id: int, data: bytes) -> None:
    if arbitration_id < 0 or arbitration_id > CAN_SFF_MASK:
        raise ValueError("only 11-bit CAN identifiers are supported")
    if len(data) > 8:
        raise ValueError("classic CAN payload may not exceed 8 bytes")
    sock.send(_FRAME.pack(arbitration_id, len(data), data.ljust(8, b"\x00")))


def receive(sock: socket.socket, timeout: float) -> Frame | None:
    readable, _, _ = select.select([sock], [], [], timeout)
    if not readable:
        return None
    wire = sock.recv(_FRAME.size)
    if len(wire) != _FRAME.size:
        raise RuntimeError("short SocketCAN frame")
    arbitration_id, dlc, payload = _FRAME.unpack(wire)
    if arbitration_id & (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_ERR_FLAG):
        return None
    if dlc > 8:
        raise RuntimeError("invalid classic-CAN DLC")
    return Frame(arbitration_id & CAN_SFF_MASK, payload[:dlc])
