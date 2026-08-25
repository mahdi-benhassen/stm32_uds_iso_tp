#!/usr/bin/env python3
"""Hardware acceptance runner for UDS/ISO-TP and CiA 302.

This runner uses Linux SocketCAN CAN_RAW frames so that ISO-TP framing and
CANopen NMT frames remain visible to a bus analyzer. It is intentionally
conservative: reset and write tests are opt-in.

Example:
    sudo ip link set can0 up type can bitrate 500000
    python3 tests/hardware/run_uds_cia302_acceptance.py \
        --iface can0 --remote-node 2 --json-out results.json

The script does not configure or reset a CAN interface. Configure the bus,
termination, power, and safety equipment before running it.
"""

from __future__ import annotations

import argparse
import json
import select
import os
import socket
import struct
import sys
import time
from dataclasses import asdict, dataclass
from typing import Callable, Iterable, Optional

CAN_FRAME = struct.Struct("=IB3x8s")
CAN_EFF_FLAG = 0x80000000
CAN_EFF_MASK = 0x1FFFFFFF
CAN_SFF_MASK = 0x000007FF


@dataclass
class Frame:
    timestamp: float
    can_id: int
    data: bytes


@dataclass
class Result:
    name: str
    status: str
    detail: str
    duration_ms: float


class RawCan:
    def __init__(self, iface: str):
        self.iface = iface
        self.sock = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
        self.sock.bind((iface,))

    def close(self) -> None:
        self.sock.close()

    def send(self, can_id: int, payload: bytes) -> None:
        if len(payload) > 8:
            raise ValueError("classic CAN payload must be at most 8 bytes")
        if can_id < 0 or can_id > CAN_EFF_MASK:
            raise ValueError(f"invalid CAN identifier 0x{can_id:X}")
        data = payload.ljust(8, b"\x00")
        frame_id = can_id | (CAN_EFF_FLAG if can_id > CAN_SFF_MASK else 0)
        self.sock.send(CAN_FRAME.pack(frame_id, len(payload), data))

    def recv(self, timeout: float) -> Optional[Frame]:
        readable, _, _ = select.select([self.sock], [], [], max(0.0, timeout))
        if not readable:
            return None
        raw = self.sock.recv(CAN_FRAME.size)
        frame_id, dlc, data = CAN_FRAME.unpack(raw)
        extended = bool(frame_id & CAN_EFF_FLAG)
        can_id = frame_id & (CAN_EFF_MASK if extended else CAN_SFF_MASK)
        return Frame(time.monotonic(), can_id, data[:dlc])

    def drain(self, duration: float = 0.03) -> None:
        deadline = time.monotonic() + duration
        while time.monotonic() < deadline:
            if self.recv(max(0.0, deadline - time.monotonic())) is None:
                break


def hex_bytes(text: str) -> bytes:
    cleaned = text.replace("0x", "").replace("0X", "").replace(" ", "").replace(":", "")
    if len(cleaned) % 2 or any(c not in "0123456789abcdefABCDEF" for c in cleaned):
        raise argparse.ArgumentTypeError(f"invalid hexadecimal bytes: {text}")
    return bytes.fromhex(cleaned)


def parse_did(text: str) -> bytes:
    value = text.lower().removeprefix("0x")
    if len(value) != 4:
        raise argparse.ArgumentTypeError("DID must be exactly four hexadecimal digits")
    try:
        return bytes.fromhex(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("invalid DID") from exc


def stmin_seconds(value: int) -> float:
    if 0 <= value <= 0x7F:
        return value / 1000.0
    if 0xF1 <= value <= 0xF9:
        return (value - 0xF0) / 10000.0
    raise ValueError(f"reserved STmin value 0x{value:02X}")


class IsoTp:
    def __init__(self, bus: RawCan, tx_id: int, rx_id: int, timeout: float):
        self.bus = bus
        self.tx_id = tx_id
        self.rx_id = rx_id
        self.timeout = timeout

    def _wait_frame(self, predicate: Callable[[Frame], bool], timeout: Optional[float] = None) -> Frame:
        deadline = time.monotonic() + (self.timeout if timeout is None else timeout)
        while time.monotonic() < deadline:
            frame = self.bus.recv(deadline - time.monotonic())
            if frame is not None and predicate(frame):
                return frame
        raise TimeoutError("timed out waiting for expected CAN frame")

    def send(self, payload: bytes) -> None:
        if not payload:
            raise ValueError("empty ISO-TP payload is not accepted by this runner")
        self.bus.drain()
        if len(payload) <= 7:
            self.bus.send(self.tx_id, bytes([len(payload)]) + payload)
            return
        if len(payload) > 4095:
            raise ValueError("classic ISO-TP payload must be <= 4095 bytes")

        first_len = len(payload)
        self.bus.send(self.tx_id, bytes([0x10 | ((first_len >> 8) & 0x0F), first_len & 0xFF]) + payload[:6])
        fc = self._wait_frame(lambda f: f.can_id == self.rx_id and len(f.data) >= 3 and (f.data[0] >> 4) == 3)
        flow_status, block_size, stmin = fc.data[0] & 0x0F, fc.data[1], fc.data[2]
        if flow_status == 2:
            raise RuntimeError("receiver rejected ISO-TP transfer with overflow")
        if flow_status == 1:
            raise RuntimeError("receiver requested wait; bounded acceptance runner does not retry wait frames")
        delay = stmin_seconds(stmin)
        offset, sequence, sent_in_block = 6, 1, 0
        while offset < len(payload):
            chunk = payload[offset:offset + 7]
            self.bus.send(self.tx_id, bytes([0x20 | (sequence & 0x0F)]) + chunk)
            offset += len(chunk)
            sequence = (sequence + 1) & 0x0F
            sent_in_block += 1
            if offset < len(payload) and block_size and sent_in_block >= block_size:
                fc = self._wait_frame(lambda f: f.can_id == self.rx_id and len(f.data) >= 3 and (f.data[0] >> 4) == 3)
                flow_status, block_size, stmin = fc.data[0] & 0x0F, fc.data[1], fc.data[2]
                if flow_status != 0:
                    raise RuntimeError(f"receiver did not continue ISO-TP transfer: FS={flow_status}")
                delay = stmin_seconds(stmin)
                sent_in_block = 0
            if offset < len(payload) and delay:
                time.sleep(delay)

    def receive(self) -> bytes:
        first = self._wait_frame(lambda f: f.can_id == self.rx_id and len(f.data) >= 1)
        pci_type = first.data[0] >> 4
        if pci_type == 0:
            length = first.data[0] & 0x0F
            if length == 0 or length > len(first.data) - 1:
                raise ValueError("invalid ISO-TP single-frame length")
            return first.data[1:1 + length]
        if pci_type != 1 or len(first.data) < 2:
            raise ValueError("expected ISO-TP single or first frame")

        length = ((first.data[0] & 0x0F) << 8) | first.data[1]
        if length <= 7 or length > 4095:
            raise ValueError("invalid ISO-TP first-frame length")
        result = bytearray(first.data[2:])
        self.bus.send(self.tx_id, bytes([0x30, 0x00, 0x00]))
        sequence = 1
        while len(result) < length:
            cf = self._wait_frame(lambda f: f.can_id == self.rx_id and len(f.data) >= 1)
            if (cf.data[0] >> 4) != 2 or (cf.data[0] & 0x0F) != sequence:
                raise ValueError("ISO-TP consecutive-frame sequence mismatch")
            result.extend(cf.data[1:])
            sequence = (sequence + 1) & 0x0F
        return bytes(result[:length])


def expect_prefix(response: bytes, prefix: bytes) -> None:
    if not response.startswith(prefix):
        raise AssertionError(f"expected {prefix.hex(' ')}, got {response.hex(' ')}")


def expect_negative(response: bytes, requested_sid: int, nrc: Optional[int] = None) -> None:
    if len(response) < 3 or response[0:2] != bytes([0x7F, requested_sid]):
        raise AssertionError(f"expected negative response for 0x{requested_sid:02X}, got {response.hex(' ')}")
    if nrc is not None and response[2] != nrc:
        raise AssertionError(f"expected NRC 0x{nrc:02X}, got 0x{response[2]:02X}")


class Acceptance:
    def __init__(self, args: argparse.Namespace):
        self.args = args
        self.results: list[Result] = []
        self.bus: Optional[RawCan] = None
        self.iso: Optional[IsoTp] = None

    def setup(self) -> None:
        self.bus = RawCan(self.args.iface)
        self.iso = IsoTp(self.bus, self.args.uds_tx_id, self.args.uds_rx_id, self.args.timeout)

    def close(self) -> None:
        if self.bus is not None:
            self.bus.close()

    def run(self, name: str, fn: Callable[[], None]) -> None:
        started = time.monotonic()
        try:
            fn()
            result = Result(name, "PASS", "acceptance criteria satisfied", (time.monotonic() - started) * 1000)
        except RuntimeError as exc:
            detail = str(exc)
            status = "SKIP" if detail.startswith("SKIP:") else "FAIL"
            result = Result(name, status, detail, (time.monotonic() - started) * 1000)
        except Exception as exc:  # test runner must continue and report all failures
            result = Result(name, "FAIL", f"{type(exc).__name__}: {exc}", (time.monotonic() - started) * 1000)
        self.results.append(result)
        print(f"[{result.status}] {result.name}: {result.detail}")

    def uds_request(self, payload: bytes) -> bytes:
        assert self.iso is not None
        self.iso.send(payload)
        return self.iso.receive()

    def test_uds_default_session(self) -> None:
        expect_prefix(self.uds_request(bytes([0x10, 0x01])), bytes([0x50, 0x01]))

    def test_uds_extended_session(self) -> None:
        expect_prefix(self.uds_request(bytes([0x10, 0x03])), bytes([0x50, 0x03]))

    def test_uds_tester_present(self) -> None:
        expect_prefix(self.uds_request(bytes([0x3E, 0x00])), bytes([0x7E, 0x00]))

    def test_uds_read_did(self) -> None:
        response = self.uds_request(bytes([0x22]) + self.args.did)
        expect_prefix(response, bytes([0x62]) + self.args.did)

    def test_uds_unknown_service(self) -> None:
        response = self.uds_request(bytes([0x99]))
        expect_negative(response, 0x99, 0x11)

    def test_uds_multiframe(self) -> None:
        if len(self.args.multiframe_request) <= 7:
            raise AssertionError("configured multi-frame request must be longer than 7 bytes")
        response = self.uds_request(self.args.multiframe_request)
        if not response:
            raise AssertionError("empty multi-frame response")
        if response[0] == 0x7F:
            if len(response) < 3 or response[1] != self.args.multiframe_request[0]:
                raise AssertionError(f"malformed negative response: {response.hex(' ')}")
        elif response[0] != (self.args.multiframe_request[0] | 0x40):
            raise AssertionError(f"unexpected multi-frame UDS response: {response.hex(' ')}")

    def test_uds_write_did(self) -> None:
        if not self.args.enable_destructive:
            raise RuntimeError("SKIP: write test requires --enable-destructive")
        response = self.uds_request(bytes([0x2E]) + self.args.write_did + self.args.write_value)
        expect_prefix(response, bytes([0x6E]) + self.args.write_did)

    def test_uds_reset(self) -> None:
        if not self.args.enable_reset:
            raise RuntimeError("SKIP: reset test requires --enable-reset")
        response = self.uds_request(bytes([0x11, self.args.reset_type]))
        expect_prefix(response, bytes([0x51, self.args.reset_type]))
        if self.args.bootup_id is not None:
            self._wait_can(self.args.bootup_id, bytes([0x00]), self.args.reset_wait)

    def _wait_can(self, can_id: int, prefix: bytes, timeout: float) -> Frame:
        assert self.bus is not None
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            frame = self.bus.recv(deadline - time.monotonic())
            if frame is not None and frame.can_id == can_id and frame.data.startswith(prefix):
                return frame
        raise TimeoutError(f"no frame ID 0x{can_id:03X} with data {prefix.hex(' ')}")

    def _send_nmt_and_expect(self, command: int, state: int, node_id: int) -> None:
        assert self.bus is not None
        self.bus.drain()
        self.bus.send(0x000, bytes([command, node_id]))
        self._wait_can(0x700 + node_id, bytes([state]), self.args.nmt_timeout)

    def test_cia302_bootup(self) -> None:
        self._wait_can(0x700 + self.args.remote_node, bytes([0x00]), self.args.nmt_timeout)

    def test_cia302_start(self) -> None:
        self._send_nmt_and_expect(0x01, 0x05, self.args.remote_node)

    def test_cia302_preop(self) -> None:
        self._send_nmt_and_expect(0x80, 0x7F, self.args.remote_node)

    def test_cia302_stop(self) -> None:
        self._send_nmt_and_expect(0x02, 0x04, self.args.remote_node)

    def test_cia302_reset_node(self) -> None:
        self._send_nmt_and_expect(0x81, 0x00, self.args.remote_node)

    @property
    def monitored_nodes(self) -> list[int]:
        return [self.args.remote_node, *self.args.additional_remote_nodes]

    def _send_broadcast_and_expect(self, command: int, state: int) -> None:
        assert self.bus is not None
        self.bus.drain()
        self.bus.send(0x000, bytes([command, 0x00]))
        for node_id in self.monitored_nodes:
            self._wait_can(0x700 + node_id, bytes([state]), self.args.nmt_timeout)

    def test_cia302_broadcast_start(self) -> None:
        self._send_broadcast_and_expect(0x01, 0x05)

    def test_cia302_broadcast_preop(self) -> None:
        self._send_broadcast_and_expect(0x80, 0x7F)

    def test_cia302_broadcast_stop(self) -> None:
        self._send_broadcast_and_expect(0x02, 0x04)

    def test_cia302_broadcast_reset_communication(self) -> None:
        self._send_broadcast_and_expect(0x82, 0x00)

    def test_cia302_target_reset_communication(self) -> None:
        self._send_nmt_and_expect(0x82, 0x00, self.args.remote_node)

    def test_cia302_targeted_isolation(self) -> None:
        if not self.args.additional_remote_nodes:
            raise RuntimeError("SKIP: targeted isolation requires --additional-remote-node")
        assert self.bus is not None
        isolated = self.args.additional_remote_nodes[0]
        self._send_nmt_and_expect(0x80, 0x7F, self.args.remote_node)
        self._send_nmt_and_expect(0x80, 0x7F, isolated)
        self.bus.drain()
        self.bus.send(0x000, bytes([0x01, self.args.remote_node]))
        self._wait_can(0x700 + self.args.remote_node, bytes([0x05]), self.args.nmt_timeout)
        deadline = time.monotonic() + self.args.nmt_timeout
        while time.monotonic() < deadline:
            frame = self.bus.recv(deadline - time.monotonic())
            if frame is not None and frame.can_id == 0x700 + isolated and frame.data != bytes([0x7F]):
                raise AssertionError("targeted NMT command changed an unaddressed node")

    def _heartbeat_samples(self, node_id: int, window: float) -> list[Frame]:
        assert self.bus is not None
        deadline = time.monotonic() + window
        samples: list[Frame] = []
        while time.monotonic() < deadline:
            frame = self.bus.recv(deadline - time.monotonic())
            if frame is not None and frame.can_id == 0x700 + node_id and len(frame.data) == 1:
                samples.append(frame)
        return samples

    def test_cia302_heartbeat(self) -> None:
        for node_id in self.monitored_nodes:
            samples = self._heartbeat_samples(node_id, self.args.heartbeat_window)
            if len(samples) < self.args.min_heartbeats:
                raise AssertionError(
                    f"node {node_id}: received {len(samples)} heartbeats, "
                    f"expected at least {self.args.min_heartbeats}"
                )
            invalid = [frame for frame in samples if frame.data[0] not in (0x00, 0x04, 0x05, 0x7F)]
            if invalid:
                raise AssertionError(f"node {node_id}: invalid heartbeat state {invalid[0].data.hex(' ')}")

    def test_cia302_heartbeat_timing(self) -> None:
        if self.args.heartbeat_window <= 0 or self.args.heartbeat_max_gap <= 0:
            raise AssertionError("heartbeat window and maximum gap must be positive")
        for node_id in self.monitored_nodes:
            samples = self._heartbeat_samples(node_id, self.args.heartbeat_window)
            if len(samples) < self.args.min_heartbeats:
                raise AssertionError(f"node {node_id}: insufficient heartbeat samples for timing")
            gaps = [b.timestamp - a.timestamp for a, b in zip(samples, samples[1:])]
            if not gaps:
                raise AssertionError(f"node {node_id}: at least two samples are required for timing")
            maximum_gap = max(gaps)
            if maximum_gap > self.args.heartbeat_max_gap:
                raise AssertionError(
                    f"node {node_id}: heartbeat gap {maximum_gap:.3f}s exceeds "
                    f"{self.args.heartbeat_max_gap:.3f}s"
                )
            if self.args.heartbeat_period is not None:
                tolerance = max(self.args.heartbeat_period * self.args.heartbeat_jitter, 0.005)
                for gap in gaps:
                    if abs(gap - self.args.heartbeat_period) > tolerance:
                        raise AssertionError(
                            f"node {node_id}: heartbeat period {gap:.3f}s outside "
                            f"{self.args.heartbeat_period:.3f}s +/- {tolerance:.3f}s"
                        )

    def _assert_preop_after_invalid_frames(self, invalid_frames: list[bytes]) -> None:
        assert self.bus is not None
        self._send_nmt_and_expect(0x80, 0x7F, self.args.remote_node)
        self.bus.drain()
        for payload in invalid_frames:
            self.bus.send(0x000, payload)
        deadline = time.monotonic() + self.args.nmt_timeout
        observed = 0
        while time.monotonic() < deadline:
            frame = self.bus.recv(deadline - time.monotonic())
            if frame is not None and frame.can_id == 0x700 + self.args.remote_node:
                observed += 1
                if frame.data != bytes([0x7F]):
                    raise AssertionError(
                        f"invalid NMT sequence changed state to {frame.data.hex(' ')}"
                    )
        if observed == 0:
            raise AssertionError("no heartbeat observed during malformed NMT matrix")

    def test_cia302_malformed_nmt(self) -> None:
        self._assert_preop_after_invalid_frames([bytes([0x01])])

    def test_cia302_malformed_nmt_matrix(self) -> None:
        self._assert_preop_after_invalid_frames(
            [
                b"",
                bytes([0x01]),
                bytes([0x01, self.args.remote_node, 0x00]),
                bytes([0x03, self.args.remote_node]),
                bytes([0x01, 0xFF]),
                bytes([0x80, 0x00]),
            ]
        )

    def selected_tests(self) -> Iterable[tuple[str, Callable[[], None]]]:
        tests = {
            "uds-default-session": self.test_uds_default_session,
            "uds-extended-session": self.test_uds_extended_session,
            "uds-tester-present": self.test_uds_tester_present,
            "uds-read-did": self.test_uds_read_did,
            "uds-unknown-service": self.test_uds_unknown_service,
            "uds-multiframe": self.test_uds_multiframe,
            "uds-write-did": self.test_uds_write_did,
            "uds-reset": self.test_uds_reset,
            "cia302-bootup": self.test_cia302_bootup,
            "cia302-start": self.test_cia302_start,
            "cia302-preop": self.test_cia302_preop,
            "cia302-stop": self.test_cia302_stop,
            "cia302-reset-node": self.test_cia302_reset_node,
            "cia302-broadcast-start": self.test_cia302_broadcast_start,
            "cia302-broadcast-preop": self.test_cia302_broadcast_preop,
            "cia302-broadcast-stop": self.test_cia302_broadcast_stop,
            "cia302-broadcast-reset-communication": self.test_cia302_broadcast_reset_communication,
            "cia302-target-reset-communication": self.test_cia302_target_reset_communication,
            "cia302-targeted-isolation": self.test_cia302_targeted_isolation,
            "cia302-heartbeat": self.test_cia302_heartbeat,
            "cia302-heartbeat-timing": self.test_cia302_heartbeat_timing,
            "cia302-malformed-nmt": self.test_cia302_malformed_nmt,
            "cia302-malformed-nmt-matrix": self.test_cia302_malformed_nmt_matrix,
        }
        if self.args.tests:
            unknown = sorted(set(self.args.tests) - set(tests))
            if unknown:
                raise ValueError(f"unknown test names: {', '.join(unknown)}")
            return ((name, tests[name]) for name in self.args.tests)
        return tests.items()

    def execute(self) -> int:
        if self.args.dry_run:
            print("DRY RUN: no CAN interface opened")
            for name, _ in self.selected_tests():
                print(f"[DRY-RUN] {name}")
            return 0
        self.setup()
        try:
            for name, test in self.selected_tests():
                self.run(name, test)
        finally:
            self.close()
        failed = [r for r in self.results if r.status == "FAIL"]
        skipped = [r for r in self.results if r.status == "SKIP"]
        passed = [r for r in self.results if r.status == "PASS"]
        if self.args.json_out:
            output = {
                "schema_version": 1,
                "git_sha": os.environ.get("GIT_SHA", "unknown"),
                "interface": self.args.iface,
                "uds_tx_id": self.args.uds_tx_id,
                "uds_rx_id": self.args.uds_rx_id,
                "remote_node": self.args.remote_node,
                "additional_remote_nodes": self.args.additional_remote_nodes,
                "heartbeat_period": self.args.heartbeat_period,
                "heartbeat_max_gap": self.args.heartbeat_max_gap,
                "results": [asdict(r) for r in self.results],
            }
            with open(self.args.json_out, "w", encoding="utf-8") as handle:
                json.dump(output, handle, indent=2)
                handle.write("\n")
        print(f"SUMMARY: {len(passed)} passed, {len(failed)} failed, {len(skipped)} skipped")
        return 1 if failed else 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iface", default="can0", help="SocketCAN interface (default: can0)")
    parser.add_argument("--uds-tx-id", type=lambda s: int(s, 0), default=0x7E0)
    parser.add_argument("--uds-rx-id", type=lambda s: int(s, 0), default=0x7E8)
    parser.add_argument("--remote-node", type=int, default=2)
    parser.add_argument("--additional-remote-node", dest="additional_remote_nodes", action="append", type=int, default=[],
                        help="additional node-ID for broadcast and targeted-isolation tests; repeatable")
    parser.add_argument("--timeout", type=float, default=1.0)
    parser.add_argument("--nmt-timeout", type=float, default=2.0)
    parser.add_argument("--reset-wait", type=float, default=5.0)
    parser.add_argument("--heartbeat-window", type=float, default=3.0)
    parser.add_argument("--min-heartbeats", type=int, default=2)
    parser.add_argument("--heartbeat-period", type=float, default=None,
                        help="expected heartbeat period in seconds; omit to check maximum gap only")
    parser.add_argument("--heartbeat-jitter", type=float, default=0.20,
                        help="relative period tolerance for timing acceptance (default: 20%%)")
    parser.add_argument("--heartbeat-max-gap", type=float, default=2.0,
                        help="maximum permitted heartbeat gap in seconds (default: 2.0)")
    parser.add_argument("--did", type=parse_did, default=bytes.fromhex("F190"))
    parser.add_argument("--multiframe-request", type=hex_bytes, default=bytes.fromhex("22 F1 90 F1 91 F1 92 F1 93"))
    parser.add_argument("--write-did", type=parse_did, default=bytes.fromhex("F1 90"))
    parser.add_argument("--write-value", type=hex_bytes, default=b"TEST")
    parser.add_argument("--reset-type", type=lambda s: int(s, 0), default=0x01)
    parser.add_argument("--bootup-id", type=lambda s: int(s, 0), default=None)
    parser.add_argument("--enable-destructive", action="store_true", help="enable DID write test")
    parser.add_argument("--enable-reset", action="store_true", help="enable ECU reset test")
    parser.add_argument("--json-out")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--tests", nargs="*", help="run only selected test names")
    return parser


if __name__ == "__main__":
    try:
        raise SystemExit(Acceptance(build_parser().parse_args()).execute())
    except KeyboardInterrupt:
        print("interrupted", file=sys.stderr)
        raise SystemExit(130)
    except Exception as exc:
        print(f"ERROR: {type(exc).__name__}: {exc}", file=sys.stderr)
        raise SystemExit(2)

# Test names:
# uds-default-session uds-extended-session uds-tester-present uds-read-did
# uds-unknown-service uds-multiframe uds-write-did uds-reset
# cia302-bootup cia302-start cia302-preop cia302-stop cia302-reset-node
# cia302-broadcast-start cia302-broadcast-preop cia302-broadcast-stop
# cia302-broadcast-reset-communication cia302-target-reset-communication
# cia302-targeted-isolation cia302-heartbeat cia302-heartbeat-timing
# cia302-malformed-nmt cia302-malformed-nmt-matrix
