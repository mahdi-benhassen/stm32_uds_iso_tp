"""Bounded NMEA 2000/J1939 gateway contract model.

The model handles standard 29-bit identifier construction/parsing and a
small battery mapping. It is deliberately transport-neutral; the production
CAN driver and NMEA address-claim policy remain integration responsibilities.

Implementation boundary: this file is host-side gateway contract code only.
The STM32F767 firmware contains no embedded NMEA 2000 stack, address-claim
state machine, or field interoperability implementation.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Optional


PGN_BATTERY_STATUS_1 = 127508
PGN_ADDRESS_CLAIM = 60928


class Nmea2000Error(ValueError):
    pass


@dataclass(frozen=True)
class ExtendedCanFrame:
    can_id: int
    data: bytes

    def __post_init__(self) -> None:
        if not 0x800 <= self.can_id <= 0x1FFFFFFF:
            raise Nmea2000Error("NMEA 2000 frame requires an extended CAN identifier")
        if not 0 <= len(self.data) <= 8:
            raise Nmea2000Error("single-frame gateway payload must be 0..8 bytes")


@dataclass(frozen=True)
class J1939Id:
    priority: int
    pgn: int
    source: int
    destination: Optional[int]


def make_j1939_id(priority: int, pgn: int, source: int, destination: Optional[int] = None) -> int:
    if not 0 <= priority <= 7 or not 0 <= source <= 0xFF or not 0 <= pgn <= 0x3FFFF:
        raise Nmea2000Error("invalid J1939 identifier fields")
    if destination is not None and not 0 <= destination <= 0xFF:
        raise Nmea2000Error("invalid destination address")
    pf = (pgn >> 8) & 0xFF
    if pf >= 240:
        # PDU2: the low PGN byte is the group extension and is part of the ID.
        return (priority << 26) | (pgn << 8) | source
    if destination is None:
        raise Nmea2000Error("PDU1 PGNs require a destination")
    # PDU1: PS is a destination address and is not part of the PGN.
    return (priority << 26) | ((pgn & 0x3FF00) << 8) | (destination << 8) | source


def parse_j1939_id(can_id: int) -> J1939Id:
    if not 0 <= can_id <= 0x1FFFFFFF:
        raise Nmea2000Error("extended CAN identifier out of range")
    priority = (can_id >> 26) & 0x7
    pf = (can_id >> 16) & 0xFF
    ps = (can_id >> 8) & 0xFF
    source = can_id & 0xFF
    data_page = (can_id >> 24) & 0x1
    if pf < 240:
        pgn = (data_page << 16) | (pf << 8)
        destination: Optional[int] = ps
    else:
        pgn = (data_page << 16) | (pf << 8) | ps
        destination = None
    return J1939Id(priority, pgn, source, destination)


def encode_battery_status_1(source: int, instance: int, voltage_v: float, current_a: float, temperature_k: float) -> ExtendedCanFrame:
    if not 0 <= instance <= 0xFF:
        raise Nmea2000Error("battery instance out of range")
    voltage = round(voltage_v * 100.0)
    current = round(current_a * 10.0)
    temperature = round(temperature_k * 100.0)
    if not 0 <= voltage <= 0xFFFF or not -32768 <= current <= 32767 or not -32768 <= temperature <= 32767:
        raise Nmea2000Error("battery measurement cannot be represented")
    can_id = make_j1939_id(6, PGN_BATTERY_STATUS_1, source)
    payload = bytes((instance,)) + voltage.to_bytes(2, "little") + current.to_bytes(2, "little", signed=True) + temperature.to_bytes(2, "little", signed=True) + b"\xFF"
    return ExtendedCanFrame(can_id, payload)


@dataclass
class BatteryGateway:
    source_address: int
    battery_index: int = 0x6081
    canopen_voltage_index: int = 0x6060
    canopen_current_index: int = 0x6070

    def battery_status(self, voltage_v: float, current_a: float, temperature_k: float) -> ExtendedCanFrame:
        return encode_battery_status_1(self.source_address, self.battery_index, voltage_v, current_a, temperature_k)

    def map_canopen_value(self, index: int, value: int) -> Optional[tuple[int, int, int]]:
        if index == self.canopen_voltage_index:
            return (PGN_BATTERY_STATUS_1, 1, value)
        if index == self.canopen_current_index:
            return (PGN_BATTERY_STATUS_1, 2, value)
        return None
