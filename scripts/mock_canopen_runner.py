#!/usr/bin/env python3
"""Deterministic user-space CANopen frame mock for the Inventus test profile.

This runner deliberately does not open a SocketCAN device. It models a single
CANopen node and a lossless in-process bus, records every transmitted frame,
and exercises the wire-level exchanges needed to smoke-test the Inventus
profile when the host kernel does not provide vcan0.

The `Frame`, `MockBus`, and `MockNode` layers form a reusable transport-neutral
pattern for future opt-in personalities; only the catalog adapter and scenario
assertions are Inventus-specific. The object values and PDO mappings are loaded
from the checked-in Inventus CSV catalogs. This is a protocol smoke test, not a
substitute for firmware execution, CAN physical-layer qualification, or HIL
testing.
"""

from __future__ import annotations

import argparse
import csv
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "product" / "inventus_battery_od.csv"
sys.path.insert(0, str(ROOT / "scripts"))
from inventus_battery_catalog import D000_SOURCE, EXTENSION_SOURCE, PDO_MAPPINGS, RECORDS  # noqa: E402


SDO_ABORT_TOGGLE = 0x05030000
SDO_ABORT_UNKNOWN_OBJECT = 0x06020000
SDO_ABORT_WRITE_RO = 0x06010002
SDO_ABORT_SUB_NOT_EXIST = 0x06090011
SDO_ABORT_TYPE_MISMATCH = 0x06070010


@dataclass(frozen=True)
class Frame:
    cob_id: int
    data: bytes

    def __post_init__(self) -> None:
        if not 0 <= self.cob_id <= 0x7FF:
            raise ValueError(f"standard CAN identifier out of range: {self.cob_id:#x}")
        if not 0 <= len(self.data) <= 8:
            raise ValueError("CAN data must contain at most eight bytes")

    def pretty(self) -> str:
        payload = " ".join(f"{byte:02X}" for byte in self.data)
        return f"0x{self.cob_id:03X} [{len(self.data)}] {payload}".rstrip()


@dataclass
class ObjectValue:
    index: int
    sub_index: int
    width: int
    access: str
    value: bytearray
    name: str

    def read(self) -> bytes:
        return bytes(self.value)

    def write(self, value: bytes) -> None:
        if len(value) != self.width:
            raise ValueError(f"{self.index:#06x}:{self.sub_index:#04x} expects {self.width} bytes")
        self.value[:] = value


def _parse_integer(raw: str) -> int:
    return int(raw.strip() or "0", 0)


def _default_bytes(raw: str, width: int, *, visible_string: bool = False, signed: bool = False) -> bytearray:
    if visible_string:
        encoded = raw.encode("ascii")
        if len(encoded) > width:
            raise ValueError(f"identity default exceeds declared width {width}")
        return bytearray(encoded + b"\x00" * (width - len(encoded)))
    return bytearray(_parse_integer(raw).to_bytes(width, "little", signed=signed))


def _ctype_width(ctype: str) -> int:
    return {"int8_t": 1, "uint8_t": 1, "int16_t": 2, "uint16_t": 2, "int32_t": 4, "uint32_t": 4}[ctype]


class InventusObjectDictionary:
    """Minimal byte-oriented OD model derived from the checked-in catalog."""

    def __init__(self) -> None:
        self.objects: dict[tuple[int, int], ObjectValue] = {}
        with SOURCE.open(newline="", encoding="utf-8") as stream:
            for row in csv.DictReader(stream):
                index = int(row["index"], 0)
                width = int(row["bytes"], 10)
                kind = row["kind"].strip()
                if kind == "diagnostic_array":
                    self.add(index, 0, 1, "ro", bytes([0xFE]), f"{row['name']} count")
                    for sub_index in range(1, 0xFF):
                        self.add(index, sub_index, 1, row["access"].strip(), b"\x00", row["name"])
                elif kind == "identity":
                    self.add(index, 0, width, "ro", _default_bytes(row["default"], width, visible_string=True), row["name"])
                else:
                    self.add(index, 0, width, row["access"].strip(), _default_bytes(row["default"], width), row["name"])

        with EXTENSION_SOURCE.open(newline="", encoding="utf-8") as stream:
            extension_records: dict[int, list[dict[str, str]]] = {}
            for row in csv.DictReader(stream):
                index = int(row["index"], 0)
                sub_index = int(row["sub_index"], 0)
                if row["kind"].strip() == "scalar":
                    self.add(index, sub_index, int(row["bytes"], 10), row["access"].strip(),
                             _default_bytes(row["default"], int(row["bytes"], 10),
                                            signed=row["ctype"].strip().startswith("int")), row["name"])
                else:
                    extension_records.setdefault(index, []).append(row)
            for index, rows in extension_records.items():
                for row in rows:
                    width = int(row["bytes"], 10)
                    self.add(index, int(row["sub_index"], 0), width, row["access"].strip(),
                             _default_bytes(row["default"], width,
                                            signed=row["ctype"].strip().startswith("int")), row["name"])

        with D000_SOURCE.open(newline="", encoding="utf-8") as stream:
            for row in csv.DictReader(stream):
                sub_index = int(row["sub_index"], 0)
                ctype = row["ctype"].strip()
                width = _ctype_width(ctype)
                self.add(0xD000, sub_index, width, row["access"].strip(),
                         _default_bytes(row["default"], width, signed=ctype.startswith("int")),
                         row["name"])

        # Include the standard six TPDO communication records as the generated
        # profile does.  Reserved sub-index 4 is intentionally not installed.
        for pdo_number in range(1, 7):
            comm_index = 0x1800 + pdo_number - 1
            self.add(comm_index, 0, 1, "ro", bytes([5]), f"TPDO{pdo_number} communication count")
            default_cob_id = 0xC0000000 | (0x180 + (pdo_number - 1) * 0x100) if pdo_number <= 4 else 0xC0000000
            self.add(comm_index, 1, 4, "rw", struct.pack("<I", default_cob_id), "COB-ID used by TPDO")
            self.add(comm_index, 2, 1, "rw", b"\xFE", "transmission type")
            self.add(comm_index, 3, 2, "rw", b"\x00\x00", "inhibit time")
            self.add(comm_index, 5, 2, "rw", b"\x00\x00", "event timer")
            self.add(comm_index, 6, 1, "rw", b"\x00", "SYNC start value")

        for mapping_index, mapping in PDO_MAPPINGS.items():
            self.add(mapping_index, 0, 1, "rw", bytes([len(mapping)]), "mapped object count")
            for sub_index, entry in enumerate(mapping, start=1):
                self.add(mapping_index, sub_index, 4, "rw", struct.pack("<I", entry), "mapped object")
            for sub_index in range(len(mapping) + 1, 9):
                self.add(mapping_index, sub_index, 4, "rw", b"\x00\x00\x00\x00", "mapped object")

        # Retain the catalog module as an import-time consistency check and
        # ensure the generated profile's extra records are represented here.
        assert len(RECORDS) == 7

    def add(self, index: int, sub_index: int, width: int, access: str, value: bytes, name: str) -> None:
        self.objects[(index, sub_index)] = ObjectValue(index, sub_index, width, access, bytearray(value), name)

    def get(self, index: int, sub_index: int) -> ObjectValue | None:
        return self.objects.get((index, sub_index))

    def read(self, index: int, sub_index: int) -> bytes:
        obj = self.get(index, sub_index)
        if obj is None:
            raise KeyError((index, sub_index))
        return obj.read()

    def write(self, index: int, sub_index: int, value: bytes) -> None:
        obj = self.get(index, sub_index)
        if obj is None:
            raise KeyError((index, sub_index))
        if obj.access == "ro":
            raise PermissionError((index, sub_index))
        obj.write(value)


class MockNode:
    def __init__(self, node_id: int, od: InventusObjectDictionary) -> None:
        if not 1 <= node_id <= 0x7F:
            raise ValueError("node ID must be in the range 1..127")
        self.node_id = node_id
        self.od = od
        self.nmt_state = 0x7F  # pre-operational
        self.pending_upload: bytes | None = None
        self.pending_offset = 0
        self.pending_toggle = 0

    @property
    def heartbeat_cob_id(self) -> int:
        return 0x700 + self.node_id

    def bootup(self) -> Frame:
        return Frame(self.heartbeat_cob_id, b"\x00")

    def heartbeat(self) -> Frame:
        return Frame(self.heartbeat_cob_id, bytes([self.nmt_state]))

    def _abort(self, index: int, sub_index: int, code: int) -> Frame:
        return Frame(0x580 + self.node_id, bytes([0x80]) + struct.pack("<HB", index, sub_index) + struct.pack("<I", code))

    def _sdo_upload(self, index: int, sub_index: int) -> list[Frame]:
        obj = self.od.get(index, sub_index)
        if obj is None:
            return [self._abort(index, sub_index, SDO_ABORT_UNKNOWN_OBJECT if sub_index == 0 else SDO_ABORT_SUB_NOT_EXIST)]
        raw = obj.read()
        if len(raw) <= 4:
            unused = 4 - len(raw)
            command = 0x43 | (unused << 2)
            return [Frame(0x580 + self.node_id, bytes([command]) + struct.pack("<HB", index, sub_index) + raw + b"\x00" * unused)]
        self.pending_upload = raw
        self.pending_offset = 0
        self.pending_toggle = 0
        return [Frame(0x580 + self.node_id, b"\x41" + struct.pack("<HB", index, sub_index) + struct.pack("<I", len(raw)))]

    def _sdo_upload_segment(self, command: int) -> list[Frame]:
        if self.pending_upload is None:
            return [Frame(0x580 + self.node_id, b"\x80\x00\x00\x00" + struct.pack("<I", SDO_ABORT_TOGGLE))]
        toggle = (command >> 4) & 1
        if toggle != self.pending_toggle:
            return [Frame(0x580 + self.node_id, b"\x80\x00\x00\x00" + struct.pack("<I", SDO_ABORT_TOGGLE))]
        chunk = self.pending_upload[self.pending_offset:self.pending_offset + 7]
        self.pending_offset += len(chunk)
        last = self.pending_offset == len(self.pending_upload)
        unused = 7 - len(chunk)
        response_command = (toggle << 4) | (unused << 1) | int(last)
        response = Frame(0x580 + self.node_id, bytes([response_command]) + chunk + b"\x00" * unused)
        self.pending_toggle ^= 1
        if last:
            self.pending_upload = None
        return [response]

    def _sdo_download(self, command: int, index: int, sub_index: int, payload: bytes) -> list[Frame]:
        obj = self.od.get(index, sub_index)
        if obj is None:
            return [self._abort(index, sub_index, SDO_ABORT_UNKNOWN_OBJECT if sub_index == 0 else SDO_ABORT_SUB_NOT_EXIST)]
        if obj.access == "ro":
            return [self._abort(index, sub_index, SDO_ABORT_WRITE_RO)]
        if (command & 0x03) != 0x03:
            return [self._abort(index, sub_index, SDO_ABORT_TYPE_MISMATCH)]
        unused = (command >> 2) & 0x03
        size = 4 - unused
        value = payload[:size]
        if len(value) != obj.width:
            return [self._abort(index, sub_index, SDO_ABORT_TYPE_MISMATCH)]
        obj.write(value)
        return [Frame(0x580 + self.node_id, b"\x60" + struct.pack("<HB", index, sub_index) + b"\x00\x00\x00\x00")]

    def receive(self, frame: Frame) -> list[Frame]:
        if frame.cob_id == 0x000 and len(frame.data) >= 2:
            command, target = frame.data[0], frame.data[1]
            if target not in (0, self.node_id):
                return []
            if command == 0x01:
                self.nmt_state = 0x05
            elif command == 0x02:
                self.nmt_state = 0x04
            elif command == 0x80:
                self.nmt_state = 0x7F
            elif command in (0x81, 0x82):
                self.nmt_state = 0x7F
                return [self.bootup()]
            else:
                return []
            return [self.heartbeat()]

        if frame.cob_id != 0x600 + self.node_id or len(frame.data) != 8:
            return []
        command = frame.data[0]
        index, sub_index = struct.unpack_from("<HB", frame.data, 1)
        if command == 0x40:
            return self._sdo_upload(index, sub_index)
        if command in (0x60, 0x70):
            return self._sdo_upload_segment(command)
        if command & 0xE0 == 0x20:
            return self._sdo_download(command, index, sub_index, frame.data[4:])
        return [self._abort(index, sub_index, SDO_ABORT_TYPE_MISMATCH)]

    def emit_tpdo(self, pdo_number: int) -> Frame | None:
        if not 1 <= pdo_number <= 6:
            raise ValueError("only TPDO1..TPDO6 are modelled")
        comm_index = 0x1800 + pdo_number - 1
        cob_id = struct.unpack("<I", self.od.read(comm_index, 1))[0]
        if cob_id & 0x80000000:
            return None
        mapping = PDO_MAPPINGS[0x1A00 + pdo_number - 1]
        payload = bytearray()
        for entry in mapping:
            index = (entry >> 16) & 0xFFFF
            sub_index = (entry >> 8) & 0xFF
            bit_length = entry & 0xFF
            value = self.od.read(index, sub_index)
            if bit_length % 8 or bit_length // 8 != len(value):
                raise AssertionError(f"mapping width mismatch for {index:#06x}:{sub_index:#04x}")
            payload.extend(value)
        if len(payload) > 8:
            raise AssertionError(f"TPDO{pdo_number} exceeds CAN payload capacity")
        return Frame(cob_id & 0x7FF, bytes(payload))


class MockBus:
    def __init__(self, node: MockNode, verbose: bool = False) -> None:
        self.node = node
        self.verbose = verbose
        self.frames: list[tuple[str, Frame]] = []

    def transmit(self, frame: Frame) -> list[Frame]:
        self.frames.append(("tx", frame))
        responses = self.node.receive(frame)
        self.frames.extend(("rx", response) for response in responses)
        if self.verbose:
            print(f"TX {frame.pretty()}")
            for response in responses:
                print(f"RX {response.pretty()}")
        return responses


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def sdo_request(bus: MockBus, node_id: int, command: int, index: int, sub_index: int, value: bytes = b"\x00\x00\x00\x00") -> list[Frame]:
    return bus.transmit(Frame(0x600 + node_id, bytes([command]) + struct.pack("<HB", index, sub_index) + value))


def sdo_upload(bus: MockBus, node_id: int, index: int, sub_index: int) -> bytes | None:
    responses = sdo_request(bus, node_id, 0x40, index, sub_index)
    require(len(responses) == 1, "SDO upload must return exactly one initial response")
    response = responses[0]
    if response.data[0] == 0x80:
        return None
    if response.data[0] & 0x02:
        unused = (response.data[0] >> 2) & 0x03
        return response.data[4:8 - unused]
    expected = struct.unpack_from("<I", response.data, 4)[0]
    result = bytearray()
    toggle = 0
    while True:
        segment = sdo_request(bus, node_id, 0x60 | (toggle << 4), 0, 0)
        require(len(segment) == 1, "segmented SDO upload must return one segment")
        segment_data = segment[0].data
        last = bool(segment_data[0] & 0x01)
        unused = (segment_data[0] >> 1) & 0x07
        result.extend(segment_data[1:8 - unused if last else 8])
        if last:
            break
        toggle ^= 1
    require(len(result) == expected, "segmented SDO upload size mismatch")
    return bytes(result)


def enable_tpdo(bus: MockBus, node_id: int, pdo_number: int) -> None:
    cob_id = 0x180 + (pdo_number - 1) * 0x100 + node_id
    response = sdo_request(bus, node_id, 0x23, 0x1800 + pdo_number - 1, 1, struct.pack("<I", cob_id))
    require(response[0].data[0] == 0x60, f"TPDO{pdo_number} COB-ID write failed")


def run_scenarios(node_id: int, verbose: bool) -> int:
    od = InventusObjectDictionary()
    node = MockNode(node_id, od)
    bus = MockBus(node, verbose=verbose)

    bootup = node.bootup()
    bus.frames.append(("rx", bootup))
    require(bootup.cob_id == 0x700 + node_id and bootup.data == b"\x00", "boot-up heartbeat mismatch")

    nmt = bus.transmit(Frame(0x000, bytes([0x01, node_id]) + b"\x00" * 6))
    require(nmt[0].data == b"\x05", "NMT start did not put node into operational state")
    require(node.heartbeat().data == b"\x05", "operational heartbeat mismatch")

    soh = sdo_upload(bus, node_id, 0x4800, 0)
    require(soh == b"\x00", "SOH default mismatch")
    sleep_write = sdo_request(bus, node_id, 0x2B, 0x4819, 0, b"\x01\x00\x00\x00")
    require(sleep_write[0].data[0] == 0x60, "rw sleep command write failed")
    require(sdo_upload(bus, node_id, 0x4819, 0) == b"\x01\x00", "sleep command readback mismatch")
    ro_write = sdo_request(bus, node_id, 0x2F, 0x4800, 0, b"\x01\x00\x00\x00")
    require(ro_write[0].data[0] == 0x80 and struct.unpack_from("<I", ro_write[0].data, 4)[0] == SDO_ABORT_WRITE_RO, "RO write was not rejected")

    for index, width in ((0x1008, 32), (0x1009, 16), (0x100A, 16)):
        identity = sdo_upload(bus, node_id, index, 0)
        require(identity == b"\x00" * width, f"identity placeholder {index:#06x} mismatch")

    require(sdo_upload(bus, node_id, 0xD000, 0) == b"\x70", "D000 highest-subindex value mismatch")
    require(sdo_upload(bus, node_id, 0xD000, 1) == struct.pack("<H", 2980), "D000 NTC1 default mismatch")
    require(sdo_upload(bus, node_id, 0xD000, 0x64) == b"\x00\x00", "D000 signed-current default mismatch")
    require(sdo_upload(bus, node_id, 0xD001, 0) == b"\xFE", "D001 count mismatch")
    for gap in (0x19, 0x1A, 0x25, 0x26, 0x27, 0x29, 0x41, 0x4F, 0xFF):
        require(sdo_upload(bus, node_id, 0xD000, gap) is None, f"D000 gap {gap:#04x} was accepted")
    require(sdo_upload(bus, node_id, 0xD001, 0xFF) is None, "D001 sub-index FF was accepted")
    diag_write = sdo_request(bus, node_id, 0x2F, 0xD001, 1, b"\x5A\x00\x00\x00")
    require(diag_write[0].data[0] == 0x60, "D001 diagnostic write failed")
    require(sdo_upload(bus, node_id, 0xD001, 1) == b"\x5A", "D001 diagnostic readback mismatch")
    d000_write = sdo_request(bus, node_id, 0x2B, 0xD000, 0x70, b"\x56\x34\x00\x00")
    require(d000_write[0].data[0] == 0x60, "D000 writable field write failed")
    require(sdo_upload(bus, node_id, 0xD000, 0x70) == b"\x56\x34", "D000 writable field readback mismatch")
    diag_ro_write = sdo_request(bus, node_id, 0x2F, 0xD000, 1, b"\x5A\x00\x00\x00")
    require(diag_ro_write[0].data[0] == 0x80, "D000 read-only field write was accepted")

    for pdo_number in range(1, 7):
        require(node.emit_tpdo(pdo_number) is None, f"TPDO{pdo_number} was not disabled by default")
        enable_tpdo(bus, node_id, pdo_number)
        frame = node.emit_tpdo(pdo_number)
        require(frame is not None, f"TPDO{pdo_number} did not emit after enable")
        expected_bytes = sum((entry & 0xFF) // 8 for entry in PDO_MAPPINGS[0x1A00 + pdo_number - 1])
        require(len(frame.data) == expected_bytes, f"TPDO{pdo_number} payload length mismatch")
        require(frame.cob_id == 0x180 + (pdo_number - 1) * 0x100 + node_id, f"TPDO{pdo_number} COB-ID mismatch")
        bus.frames.append(("tx", frame))
        if verbose:
            print(f"TX {frame.pretty()} (mock TPDO{pdo_number})")

    unknown = sdo_upload(bus, node_id, 0x4FFF, 0)
    require(unknown is None, "unknown SDO object was accepted")

    print(f"mock_canopen_runner: PASS ({len(bus.frames)} recorded frames, node 0x{node_id:02X})")
    return 0


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--node-id", type=lambda value: int(value, 0), default=0x23, help="CANopen node ID (default: 0x23)")
    parser.add_argument("--verbose", action="store_true", help="print every simulated TX/RX frame")
    args = parser.parse_args(list(argv) if argv is not None else None)
    try:
        return run_scenarios(args.node_id, args.verbose)
    except (AssertionError, KeyError, PermissionError, ValueError) as error:
        print(f"mock_canopen_runner: FAIL: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
