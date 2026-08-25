#!/usr/bin/env python3
"""Deterministic classic-CAN CANopen wire-level harness for vcan regression tests.

This program is not firmware and does not replace CANopenNode. It exercises the
same CiA 301 frame contracts on SocketCAN so CI can validate host tools and
integration assumptions before an STM32 physical-CAN job is available.
"""
from __future__ import annotations

import argparse
import signal
import sys
import time
from pathlib import Path

HOST = Path(__file__).resolve().parents[4] / "tests" / "host"
sys.path.insert(0, str(HOST))
from can_socket import Frame, open_socket, receive, send  # noqa: E402

ABORT_READONLY = 0x06010002
ABORT_NO_OBJECT = 0x06020000
ABORT_UNSUPPORTED = 0x05040001


class Device:
    def __init__(self, interface: str, node_id: int, heartbeat_ms: int, emcy_on_start: bool) -> None:
        self.socket = open_socket(interface)
        self.node_id = node_id
        self.heartbeat_ms = heartbeat_ms
        self.state = 0x7F  # pre-operational
        self.running = True
        self.next_heartbeat = time.monotonic()
        self.segment_upload: bytes | None = None
        self.segment_download = bytearray()
        self.segment_index: tuple[int, int] | None = None
        self.mapping: dict[tuple[int, int], int] = {(0x1600, 0): 0}
        self.values: dict[tuple[int, int], bytes] = {
            (0x1017, 0): heartbeat_ms.to_bytes(2, "little"),
            (0x1018, 1): (0x12345678).to_bytes(4, "little"),
            (0x1018, 2): (0x0000F767).to_bytes(4, "little"),
            (0x1018, 3): (0x00010000).to_bytes(4, "little"),
            (0x1018, 4): (0x00000001).to_bytes(4, "little"),
            (0x2000, 0): (0).to_bytes(4, "little"),
            (0x2001, 0): b"CANopenNode SocketCAN segmented regression payload.",
            (0x6000, 1): b"\x00\x00",
            (0x6200, 1): b"\x00\x00",
        }
        if emcy_on_start:
            send(self.socket, 0x080 + node_id, bytes((0x10, 0x23, 0x10, 0, 0, 0, 0, 0)))
        send(self.socket, 0x700 + node_id, b"\x00")  # CANopen boot-up

    def _publish_nmt_state(self, state: int, bootup: bool = False) -> None:
        self.state = state
        send(self.socket, 0x700 + self.node_id, bytes((0x00 if bootup else state,)))
        self.next_heartbeat = time.monotonic() + (self.heartbeat_ms / 1000.0)

    def _heartbeat(self) -> None:
        now = time.monotonic()
        if now >= self.next_heartbeat:
            send(self.socket, 0x700 + self.node_id, bytes((self.state,)))
            self.next_heartbeat = now + (self.heartbeat_ms / 1000.0)

    def _abort(self, index: int, sub: int, code: int) -> None:
        send(self.socket, 0x580 + self.node_id, bytes((0x80, index & 0xFF, index >> 8, sub)) + code.to_bytes(4, "little"))

    def _value(self, index: int, sub: int) -> bytes | None:
        if (index, sub) in self.mapping:
            value = self.mapping[(index, sub)]
            return value.to_bytes(1 if sub == 0 else 4, "little")
        return self.values.get((index, sub))

    def _is_writable(self, index: int) -> bool:
        return index in (0x1017, 0x1600, 0x2000, 0x6200)

    def _write_value(self, index: int, sub: int, payload: bytes) -> bool:
        if not self._is_writable(index):
            return False
        if index == 0x1600:
            if sub > 8 or len(payload) not in (1, 4):
                return False
            self.mapping[(index, sub)] = int.from_bytes(payload, "little")
        else:
            self.values[(index, sub)] = payload
        if index == 0x1017 and sub == 0 and len(payload) == 2:
            self.heartbeat_ms = int.from_bytes(payload, "little")
        return True

    def _sdo_upload(self, index: int, sub: int) -> None:
        value = self._value(index, sub)
        if value is None:
            self._abort(index, sub, ABORT_NO_OBJECT)
            return
        if len(value) <= 4:
            command = 0x43 | ((4 - len(value)) << 2)
            send(self.socket, 0x580 + self.node_id, bytes((command, index & 0xFF, index >> 8, sub)) + value.ljust(4, b"\x00"))
            return
        self.segment_upload = value
        send(self.socket, 0x580 + self.node_id, bytes((0x41, index & 0xFF, index >> 8, sub)) + len(value).to_bytes(4, "little"))

    def _sdo_download_expedited(self, command: int, index: int, sub: int, data: bytes) -> None:
        if not self._is_writable(index):
            self._abort(index, sub, ABORT_READONLY)
            return
        length = 4 - ((command >> 2) & 0x03)
        if not self._write_value(index, sub, data[:length]):
            self._abort(index, sub, ABORT_UNSUPPORTED)
            return
        send(self.socket, 0x580 + self.node_id, bytes((0x60, index & 0xFF, index >> 8, sub, 0, 0, 0, 0)))

    def _sdo_segment(self, command: int, data: bytes) -> None:
        # Client requests the next upload segment with CCS=3 (0x60 or 0x70).
        if (command & 0xE0) == 0x60 and self.segment_upload is not None:
            toggle = command & 0x10
            chunk, self.segment_upload = self.segment_upload[:7], self.segment_upload[7:]
            last = not self.segment_upload
            response = toggle | ((7 - len(chunk)) << 1) | (1 if last else 0)
            send(self.socket, 0x580 + self.node_id, bytes((response,)) + chunk.ljust(7, b"\x00"))
            if last:
                self.segment_upload = None
            return
        # Client sends a segmented download with CCS=0.
        if (command & 0xE0) == 0x00 and self.segment_index is not None:
            length = 7 - ((command >> 1) & 0x07)
            self.segment_download.extend(data[:length])
            if command & 0x01:
                index, sub = self.segment_index
                if not self._write_value(index, sub, bytes(self.segment_download)):
                    self._abort(index, sub, ABORT_UNSUPPORTED)
                else:
                    send(self.socket, 0x580 + self.node_id, bytes((0x20 | (command & 0x10), 0, 0, 0, 0, 0, 0, 0)))
                self.segment_index = None
                self.segment_download.clear()
            else:
                send(self.socket, 0x580 + self.node_id, bytes((0x20 | (command & 0x10), 0, 0, 0, 0, 0, 0, 0)))

    def _handle_sdo(self, frame: Frame) -> None:
        command = frame.data[0]
        index = frame.data[1] | (frame.data[2] << 8)
        sub = frame.data[3]
        if command == 0x40:
            self._sdo_upload(index, sub)
        elif (command & 0xE0) == 0x20 and (command & 0x02):
            self._sdo_download_expedited(command, index, sub, frame.data[4:8])
        elif (command & 0xE0) == 0x20:
            if not self._is_writable(index):
                self._abort(index, sub, ABORT_READONLY)
            else:
                self.segment_index = (index, sub)
                self.segment_download.clear()
                send(self.socket, 0x580 + self.node_id, bytes((0x60, index & 0xFF, index >> 8, sub, 0, 0, 0, 0)))
        else:
            self._sdo_segment(command, frame.data[1:8])

    def _handle_lss(self, frame: Frame) -> None:
        # Fastscan response for the reference vendor identity. This model is
        # deliberately bounded to the discovery response used by CI.
        if len(frame.data) == 8 and frame.data[0] == 0x51:
            send(self.socket, 0x7E4, bytes((0x4F, 0, 0, 0, 0, 0, 0, 0)))

    def handle(self, frame: Frame) -> None:
        if frame.arbitration_id == 0x000 and len(frame.data) == 2:
            command, target = frame.data[0], frame.data[1]
            if target in (0, self.node_id):
                if command == 0x01:
                    self._publish_nmt_state(0x05)
                elif command == 0x02:
                    self._publish_nmt_state(0x04)
                elif command == 0x80:
                    self._publish_nmt_state(0x7F)
                elif command == 0x81:
                    self._publish_nmt_state(0x7F, bootup=True)
                elif command == 0x82:
                    self._publish_nmt_state(0x7F)
        elif frame.arbitration_id == 0x600 + self.node_id and len(frame.data) == 8:
            self._handle_sdo(frame)
        elif frame.arbitration_id == 0x200 + self.node_id and len(frame.data) >= 2:
            self.values[(0x6200, 1)] = frame.data[:2]
        elif frame.arbitration_id == 0x080 and len(frame.data) == 0 and self.state == 0x05:
            send(self.socket, 0x180 + self.node_id, self.values[(0x6200, 1)])
        elif frame.arbitration_id == 0x7E5:
            self._handle_lss(frame)

    def run(self) -> None:
        while self.running:
            self._heartbeat()
            frame = receive(self.socket, 0.01)
            if frame is not None:
                self.handle(frame)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--interface", default="vcan0")
    parser.add_argument("--node-id", type=lambda value: int(value, 0), default=0x0A)
    parser.add_argument("--heartbeat-ms", type=int, default=100)
    parser.add_argument("--emcy-on-start", action="store_true")
    args = parser.parse_args()
    if not 1 <= args.node_id <= 127 or args.heartbeat_ms <= 0:
        raise SystemExit("node ID must be 1..127 and heartbeat period must be positive")
    device = Device(args.interface, args.node_id, args.heartbeat_ms, args.emcy_on_start)
    signal.signal(signal.SIGTERM, lambda *_: setattr(device, "running", False))
    signal.signal(signal.SIGINT, lambda *_: setattr(device, "running", False))
    device.run()


if __name__ == "__main__":
    main()
