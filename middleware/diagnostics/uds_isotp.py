"""Bounded ISO-TP over classic CAN and UDS server model.

This is a deterministic host-side contract model. The current STM32F767
reference uses classic bxCAN, so CAN-FD transport is deliberately not claimed.
The model supports ISO-TP single/first/consecutive/flow-control frames and a
small UDS service set suitable for diagnostics integration tests.

Implementation boundary: this file is host-side validation code only. The
STM32F767 firmware contains no embedded ISO-TP transport or UDS server, so this
module must not be presented as an on-target diagnostic implementation.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Callable, Dict, Iterable, List, Optional


class IsoTpError(ValueError):
    pass


@dataclass(frozen=True)
class CanFrame:
    can_id: int
    data: bytes

    def __post_init__(self) -> None:
        if not 0 <= self.can_id <= 0x7FF:
            raise IsoTpError("classic CAN identifier out of range")
        if not 0 <= len(self.data) <= 8:
            raise IsoTpError("classic CAN frame must contain 0..8 bytes")


@dataclass
class IsoTpReceiver:
    max_payload: int = 4095
    _expected: Optional[int] = None
    _buffer: bytearray = field(default_factory=bytearray)
    _next_sequence: int = 1

    def feed(self, frame: CanFrame) -> tuple[list[CanFrame], Optional[bytes]]:
        if not frame.data:
            raise IsoTpError("empty ISO-TP frame")
        pci = frame.data[0] >> 4
        if pci == 0x0:
            length = frame.data[0] & 0x0F
            if length > 7 or length > len(frame.data) - 1:
                raise IsoTpError("invalid single-frame length")
            return [], bytes(frame.data[1 : 1 + length])
        if pci == 0x1:
            if len(frame.data) < 2:
                raise IsoTpError("truncated first frame")
            length = ((frame.data[0] & 0x0F) << 8) | frame.data[1]
            if length <= 7 or length > self.max_payload:
                raise IsoTpError("invalid first-frame length")
            self._expected = length
            self._buffer = bytearray(frame.data[2:])
            self._next_sequence = 1
            return [CanFrame(frame.can_id, bytes((0x30, 0x00, 0x00)))], self._complete_if_ready()
        if pci == 0x2:
            if self._expected is None:
                raise IsoTpError("consecutive frame without first frame")
            sequence = frame.data[0] & 0x0F
            if sequence != self._next_sequence:
                self._reset()
                raise IsoTpError("consecutive-frame sequence mismatch")
            self._buffer.extend(frame.data[1:])
            self._next_sequence = (self._next_sequence + 1) & 0x0F
            return [], self._complete_if_ready()
        if pci == 0x3:
            if (frame.data[0] & 0x0F) not in (0x0, 0x1, 0x2):
                raise IsoTpError("unsupported flow-control status")
            return [], None
        raise IsoTpError("reserved ISO-TP PCI type")

    def _complete_if_ready(self) -> Optional[bytes]:
        if self._expected is not None and len(self._buffer) >= self._expected:
            payload = bytes(self._buffer[: self._expected])
            self._reset()
            return payload
        return None

    def _reset(self) -> None:
        self._expected = None
        self._buffer.clear()
        self._next_sequence = 1


def isotp_encode(can_id: int, payload: bytes) -> list[CanFrame]:
    if not payload:
        raise IsoTpError("empty UDS payload")
    if len(payload) <= 7:
        return [CanFrame(can_id, bytes((len(payload),)) + payload)]
    if len(payload) > 4095:
        raise IsoTpError("classic ISO-TP payload exceeds 4095 bytes")
    frames = [CanFrame(can_id, bytes((0x10 | ((len(payload) >> 8) & 0x0F), len(payload) & 0xFF)) + payload[:6])]
    sequence = 1
    offset = 6
    while offset < len(payload):
        chunk = payload[offset : offset + 7]
        frames.append(CanFrame(can_id, bytes((0x20 | sequence,)) + chunk))
        sequence = (sequence + 1) & 0x0F
        offset += len(chunk)
    return frames


NRC_SERVICE_NOT_SUPPORTED = 0x11
NRC_SUBFUNCTION_NOT_SUPPORTED = 0x12
NRC_INCORRECT_LENGTH = 0x13
NRC_CONDITIONS_NOT_CORRECT = 0x22
NRC_REQUEST_OUT_OF_RANGE = 0x31


@dataclass
class UdsServer:
    read_did: Callable[[int], Optional[bytes]]
    write_did: Callable[[int, bytes], bool]
    session: int = 0x01
    reset_requested: bool = False

    def handle(self, request: bytes) -> bytes:
        if not request:
            return self._negative(0, NRC_INCORRECT_LENGTH)
        service = request[0]
        if service == 0x10:
            if len(request) != 2 or request[1] not in (0x01, 0x03):
                return self._negative(service, NRC_SUBFUNCTION_NOT_SUPPORTED)
            self.session = request[1]
            return bytes((0x50, request[1], 0x00, 0x32, 0x01, 0xF4))
        if service == 0x11:
            if len(request) != 2 or request[1] != 0x01:
                return self._negative(service, NRC_SUBFUNCTION_NOT_SUPPORTED)
            self.reset_requested = True
            return bytes((0x51, request[1]))
        if service == 0x3E:
            if len(request) != 2 or request[1] not in (0x00, 0x80):
                return self._negative(service, NRC_SUBFUNCTION_NOT_SUPPORTED)
            return bytes((0x7E, request[1]))
        if service == 0x22:
            if len(request) != 3:
                return self._negative(service, NRC_INCORRECT_LENGTH)
            did = int.from_bytes(request[1:3], "big")
            value = self.read_did(did)
            if value is None:
                return self._negative(service, NRC_REQUEST_OUT_OF_RANGE)
            return bytes((0x62,)) + request[1:3] + value
        if service == 0x2E:
            if len(request) < 4:
                return self._negative(service, NRC_INCORRECT_LENGTH)
            did = int.from_bytes(request[1:3], "big")
            if not self.write_did(did, request[3:]):
                return self._negative(service, NRC_REQUEST_OUT_OF_RANGE)
            return bytes((0x6E,)) + request[1:3]
        return self._negative(service, NRC_SERVICE_NOT_SUPPORTED)

    @staticmethod
    def _negative(service: int, nrc: int) -> bytes:
        return bytes((0x7F, service, nrc))


@dataclass
class UdsIsoTpNode:
    request_id: int
    response_id: int
    server: UdsServer
    receiver: IsoTpReceiver = field(default_factory=IsoTpReceiver)

    def receive(self, frame: CanFrame) -> list[CanFrame]:
        if frame.can_id != self.request_id:
            return []
        flow_control, payload = self.receiver.feed(frame)
        responses = list(flow_control)
        if payload is not None:
            responses.extend(isotp_encode(self.response_id, self.server.handle(payload)))
        return responses
