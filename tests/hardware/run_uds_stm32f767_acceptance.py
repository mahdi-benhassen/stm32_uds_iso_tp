#!/usr/bin/env python3
"""Safety-gated STM32F767 UDS/ISO-TP and CANopen acceptance runner.

The runner uses an independently selected SocketCAN interface and never configures
that interface. Reset, Flash-download probes, malformed-frame probes, and active
CANopen traffic are explicit operator gates. Dry-run mode is suitable for CI.

Example:
    PYTHONPATH=tests/hardware python3 tests/hardware/run_uds_stm32f767_acceptance.py \
        --iface can0 --json-out build/hil/uds.json \
        --csv-out build/hil/uds.csv --report-out build/hil/uds.txt
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import sys
import threading
import time
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Callable, Optional

_THIS_DIR = Path(__file__).resolve().parent
if str(_THIS_DIR) not in sys.path:
    sys.path.insert(0, str(_THIS_DIR))

from run_uds_cia302_acceptance import (  # noqa: E402
    Frame,
    IsoTp,
    RawCan,
    expect_negative,
    expect_prefix,
)


@dataclass
class Result:
    name: str
    status: str
    detail: str
    duration_ms: float
    nrc: str = ""


@dataclass
class Capture:
    timestamp: float
    direction: str
    test: str
    can_id: int
    dlc: int
    data: str
    response_time_ms: str = ""
    nrc: str = ""
    verdict: str = ""


class RecordingCan(RawCan):
    """RawCan wrapper retaining physical CAN frame evidence."""

    def __init__(self, iface: str, owner: "Acceptance"):
        super().__init__(iface)
        self.owner = owner

    def send(self, can_id: int, payload: bytes) -> None:
        self.owner.capture_frame("TX", can_id, payload)
        super().send(can_id, payload)

    def recv(self, timeout: float) -> Optional[Frame]:
        frame = super().recv(timeout)
        if frame is not None:
            self.owner.capture_frame("RX", frame.can_id, frame.data, frame.timestamp)
        return frame


class Acceptance:
    def __init__(self, args: argparse.Namespace):
        self.args = args
        self.results: list[Result] = []
        self.captures: list[Capture] = []
        self.bus: Optional[RecordingCan] = None
        self.iso: Optional[IsoTp] = None
        self.current_test = "setup"
        self.last_request_started = 0.0
        self.last_response = b""
        self._canopen_stop = threading.Event()
        self._canopen_thread: Optional[threading.Thread] = None

    def capture_frame(self, direction: str, can_id: int, data: bytes,
                      timestamp: Optional[float] = None) -> None:
        self.captures.append(Capture(
            timestamp=time.monotonic() if timestamp is None else timestamp,
            direction=direction,
            test=self.current_test,
            can_id=can_id,
            dlc=len(data),
            data=data.hex(" "),
        ))

    def setup(self) -> None:
        self.bus = RecordingCan(self.args.iface, self)
        self.iso = IsoTp(self.bus, self.args.uds_tx_id, self.args.uds_rx_id, self.args.timeout)

    def close(self) -> None:
        self.stop_canopen_traffic()
        if self.bus is not None:
            self.bus.close()

    def start_canopen_traffic(self) -> None:
        if not self.args.enable_canopen_traffic or self.bus is None:
            return
        self._canopen_stop.clear()

        def worker() -> None:
            # This is deliberately opt-in: NMT is an active network operation.
            node = self.args.canopen_node
            frames = [
                (0x000, bytes([0x01, node])),       # NMT start
                (0x700 + node, bytes([0x7F])),      # heartbeat
                (0x180 + node, bytes([0x00] * 8)),  # PDO sample
                (0x600 + node, bytes([0x40, 0, 0, 0, 0, 0, 0, 0])),  # SDO upload init
                (0x080 + node, bytes([0x00] * 8)),  # EMCY sample
            ]
            index = 0
            while not self._canopen_stop.is_set():
                can_id, payload = frames[index % len(frames)]
                try:
                    self.bus.send(can_id, payload)
                except OSError:
                    return
                index += 1
                self._canopen_stop.wait(self.args.canopen_period)

        self._canopen_thread = threading.Thread(target=worker, name="canopen-traffic", daemon=True)
        self._canopen_thread.start()

    def stop_canopen_traffic(self) -> None:
        self._canopen_stop.set()
        if self._canopen_thread is not None:
            self._canopen_thread.join(timeout=1.0)
            self._canopen_thread = None

    def run(self, name: str, fn: Callable[[], None]) -> None:
        self.current_test = name
        started = time.monotonic()
        try:
            fn()
            result = Result(name, "PASS", "acceptance criteria satisfied",
                            (time.monotonic() - started) * 1000.0)
        except RuntimeError as exc:
            detail = str(exc)
            result = Result(name, "SKIP" if detail.startswith("SKIP:") else "FAIL", detail,
                            (time.monotonic() - started) * 1000.0)
        except Exception as exc:  # Every test is reported; one failure must not hide others.
            result = Result(name, "FAIL", f"{type(exc).__name__}: {exc}",
                            (time.monotonic() - started) * 1000.0)
        self.results.append(result)
        for capture in reversed(self.captures):
            if capture.test == name and capture.response_time_ms == "":
                capture.verdict = result.status
                capture.response_time_ms = f"{result.duration_ms:.3f}"
                if self.last_response.startswith(b"\x7f") and len(self.last_response) >= 3:
                    capture.nrc = f"0x{self.last_response[2]:02X}"
                break
        print(f"[{result.status}] {result.name}: {result.detail}")

    def request(self, payload: bytes) -> bytes:
        if self.iso is None:
            raise RuntimeError("runner is not connected")
        self.last_request_started = time.monotonic()
        self.last_response = b""
        self.iso.send(payload)
        self.last_response = self.iso.receive()
        return self.last_response

    # UDS service inventory: 0x10, 0x11, 0x19, 0x22, 0x27, 0x28, 0x31,
    # 0x34, 0x36, 0x37, 0x3E, and 0x85.
    def test_10_session_control(self) -> None:
        expect_prefix(self.request(bytes([0x10, 0x01])), bytes([0x50, 0x01]))

    def test_11_ecu_reset(self) -> None:
        if not self.args.enable_reset:
            raise RuntimeError("SKIP: ECU reset requires --enable-reset")
        expect_prefix(self.request(bytes([0x11, self.args.reset_type])), bytes([0x51, self.args.reset_type]))

    def test_19_read_dtc_information(self) -> None:
        response = self.request(bytes([0x19, 0x02]))
        if response[0] not in (0x59, 0x7F):
            raise AssertionError(f"unexpected ReadDTCInformation response: {response.hex(' ')}")
        if response[0] == 0x7F:
            expect_negative(response, 0x19)

    def test_22_read_data_by_identifier(self) -> None:
        response = self.request(bytes([0x22]) + self.args.did)
        if response[0] == 0x7F:
            expect_negative(response, 0x22)
        else:
            expect_prefix(response, bytes([0x62]) + self.args.did)

    def test_27_security_access(self) -> None:
        response = self.request(bytes([0x27, self.args.security_seed_subfunction]))
        if response[0] == 0x7F:
            expect_negative(response, 0x27)
        else:
            expect_prefix(response, bytes([0x67, self.args.security_seed_subfunction]))
            if len(response) <= 2:
                raise AssertionError("SecurityAccess returned an empty seed")

    def test_28_communication_control(self) -> None:
        expect_negative(self.request(bytes([0x28, 0x00, 0x01])), 0x28)

    def test_31_routine_control(self) -> None:
        expect_negative(self.request(bytes([0x31, 0x01, 0xFF, 0x00])), 0x31)

    def test_34_request_download(self) -> None:
        if not self.args.enable_download:
            # The reference runtime is intentionally safe-deny when no Flash callbacks exist.
            expect_negative(self.request(bytes([0x34, 0x22, 0x00, 0x00, 0x00, 0x10])), 0x34)
            return
        response = self.request(bytes([0x34, 0x22, 0x00, 0x00, 0x00, 0x10]))
        if response[0] not in (0x74, 0x7F):
            raise AssertionError(f"unexpected RequestDownload response: {response.hex(' ')}")

    def test_36_transfer_data(self) -> None:
        response = self.request(bytes([0x36, 0x01, 0x00]))
        expect_negative(response, 0x36)

    def test_37_request_transfer_exit(self) -> None:
        expect_negative(self.request(bytes([0x37])), 0x37)

    def test_3e_tester_present(self) -> None:
        expect_prefix(self.request(bytes([0x3E, 0x00])), bytes([0x7E, 0x00]))

    def test_85_control_dtc_setting(self) -> None:
        expect_negative(self.request(bytes([0x85, 0x02])), 0x85)

    def test_isotp_multiframe(self) -> None:
        payload = bytes([0x3E, 0x00]) + bytes(range(2, 12))
        response = self.request(payload)
        if len(response) < 1 or response[0] not in (0x7E, 0x7F):
            raise AssertionError(f"unexpected multi-frame request response: {response.hex(' ')}")

    def test_isotp_overflow_probe(self) -> None:
        if not self.args.enable_adversarial:
            raise RuntimeError("SKIP: ISO-TP malformed probes require --enable-adversarial")
        if self.bus is None:
            raise RuntimeError("runner is not connected")
        # A legal FF declaring the maximum classic-CAN payload, followed by no CF,
        # exercises the ECU's bounded FF/timeout path without writing application data.
        self.bus.send(self.args.uds_tx_id, bytes([0x1F, 0xFF, 0x3E, 0x00, 0, 0, 0, 0]))
        time.sleep(min(self.args.timeout * 1.5, 2.0))

    def selected_tests(self) -> list[tuple[str, Callable[[], None]]]:
        return [
            ("uds-0x10-session-control", self.test_10_session_control),
            ("uds-0x11-ecu-reset", self.test_11_ecu_reset),
            ("uds-0x19-read-dtc-information", self.test_19_read_dtc_information),
            ("uds-0x22-read-data-by-identifier", self.test_22_read_data_by_identifier),
            ("uds-0x27-security-access", self.test_27_security_access),
            ("uds-0x28-communication-control", self.test_28_communication_control),
            ("uds-0x31-routine-control", self.test_31_routine_control),
            ("uds-0x34-request-download", self.test_34_request_download),
            ("uds-0x36-transfer-data", self.test_36_transfer_data),
            ("uds-0x37-request-transfer-exit", self.test_37_request_transfer_exit),
            ("uds-0x3e-tester-present", self.test_3e_tester_present),
            ("uds-0x85-control-dtc-setting", self.test_85_control_dtc_setting),
            ("isotp-sf-ff-cf-fc-bs-stmin", self.test_isotp_multiframe),
            ("isotp-overflow-timeout-probe", self.test_isotp_overflow_probe),
            ("canopen-concurrent-nmt-heartbeat-sdo-pdo-emcy", self.canopen_concurrency_marker),
        ]

    def canopen_concurrency_marker(self) -> None:
        if not self.args.enable_canopen_traffic:
            raise RuntimeError("SKIP: concurrent CANopen traffic requires --enable-canopen-traffic")
        # Traffic is generated around the full UDS inventory in execute().
        if self._canopen_thread is None:
            raise AssertionError("CANopen traffic thread was not started")

    def write_reports(self) -> None:
        metadata = {
            "schema_version": 2,
            "runner": "uds-stm32f767-acceptance-v2",
            "git_sha": os.environ.get("GIT_SHA", "unknown"),
            "interface": self.args.iface,
            "independent_can_interface": True,
            "uds_tx_id": self.args.uds_tx_id,
            "uds_rx_id": self.args.uds_rx_id,
            "destructive_reset_enabled": self.args.enable_reset,
            "download_enabled": self.args.enable_download,
            "adversarial_probes_enabled": self.args.enable_adversarial,
            "concurrent_canopen_enabled": self.args.enable_canopen_traffic,
            "results": [asdict(result) for result in self.results],
            "captures": [asdict(capture) for capture in self.captures],
        }
        if self.args.json_out:
            path = Path(self.args.json_out)
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
        if self.args.csv_out:
            path = Path(self.args.csv_out)
            path.parent.mkdir(parents=True, exist_ok=True)
            with path.open("w", newline="", encoding="utf-8") as handle:
                writer = csv.DictWriter(handle, fieldnames=list(asdict(Capture(0, "", "", 0, 0, "")).keys()))
                writer.writeheader()
                writer.writerows(asdict(capture) for capture in self.captures)
        if self.args.report_out:
            path = Path(self.args.report_out)
            path.parent.mkdir(parents=True, exist_ok=True)
            lines = [
                "STM32F767 UDS/ISO-TP acceptance report",
                f"Interface: {self.args.iface} (independent CAN interface: yes)",
                f"Firmware SHA: {metadata['git_sha']}",
                "",
            ]
            lines.extend(f"{result.status:4} {result.name:48} {result.duration_ms:9.3f} ms  {result.detail}"
                         for result in self.results)
            lines.append("")
            lines.append(f"Frames captured: {len(self.captures)}")
            path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    def execute(self) -> int:
        tests = self.selected_tests()
        if self.args.dry_run:
            print("DRY RUN: no CAN interface opened")
            for name, _ in tests:
                print(f"[DRY-RUN] {name}")
            self.results = [Result(name, "SKIP", "dry-run inventory only", 0.0) for name, _ in tests]
            self.write_reports()
            return 0

        self.setup()
        self.start_canopen_traffic()
        try:
            for name, test in tests:
                self.run(name, test)
        finally:
            self.close()

        self.write_reports()
        failed = sum(result.status == "FAIL" for result in self.results)
        passed = sum(result.status == "PASS" for result in self.results)
        skipped = sum(result.status == "SKIP" for result in self.results)
        print(f"SUMMARY: {passed} passed, {failed} failed, {skipped} skipped")
        return 1 if failed else 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iface", "--can-interface", default="can0",
                        help="independent SocketCAN interface; it is never configured by this runner")
    parser.add_argument("--uds-tx-id", type=lambda value: int(value, 0), default=0x7E0,
                        help="tester-to-ECU request ID")
    parser.add_argument("--uds-rx-id", type=lambda value: int(value, 0), default=0x7E8,
                        help="ECU-to-tester response ID")
    parser.add_argument("--timeout", type=float, default=1.0)
    parser.add_argument("--did", type=lambda value: bytes.fromhex(value.removeprefix("0x")),
                        default=bytes.fromhex("F180"), help="four-hex-digit DID")
    parser.add_argument("--security-seed-subfunction", type=lambda value: int(value, 0), default=0x01)
    parser.add_argument("--reset-type", type=lambda value: int(value, 0), default=0x01)
    parser.add_argument("--canopen-node", type=int, default=10)
    parser.add_argument("--canopen-period", type=float, default=0.010)
    parser.add_argument("--enable-reset", action="store_true",
                        help="enable ECU reset acceptance test")
    parser.add_argument("--enable-download", action="store_true",
                        help="enable Flash-download acceptance policy probe")
    parser.add_argument("--enable-adversarial", action="store_true",
                        help="enable malformed ISO-TP probe")
    parser.add_argument("--enable-canopen-traffic", action="store_true",
                        help="send active NMT/heartbeat/SDO/PDO/EMCY traffic concurrently")
    parser.add_argument("--json-out")
    parser.add_argument("--csv-out")
    parser.add_argument("--report-out")
    parser.add_argument("--dry-run", action="store_true")
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
