#!/usr/bin/env python3
"""STM32F767 UDS/ISO-TP hardware acceptance runner.

The runner deliberately separates positive diagnostic checks from operations that
can alter ECU state. It never configures CAN, changes termination, erases Flash,
or resets the board unless the operator explicitly enables those actions.

Example (non-destructive):
    python3 tests/hardware/run_stm32f767_uds_acceptance.py \
        --iface can0 --uds-tx-id 0x7E0 --uds-rx-id 0x7E8 \
        --json-out build/hil/stm32f767-uds.json

Use --dry-run in CI to validate the test inventory without opening SocketCAN.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import time
from dataclasses import asdict, dataclass
from typing import Callable, Optional

from run_uds_cia302_acceptance import IsoTp, RawCan, expect_negative, expect_prefix


@dataclass
class Result:
    name: str
    status: str
    detail: str
    duration_ms: float


class Acceptance:
    def __init__(self, args: argparse.Namespace):
        self.args = args
        self.bus: Optional[RawCan] = None
        self.iso: Optional[IsoTp] = None
        self.results: list[Result] = []

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
            result = Result(name, "PASS", "acceptance criteria satisfied", (time.monotonic() - started) * 1000.0)
        except RuntimeError as exc:
            detail = str(exc)
            result = Result(name, "SKIP" if detail.startswith("SKIP:") else "FAIL", detail,
                            (time.monotonic() - started) * 1000.0)
        except Exception as exc:  # runner continues and records all checks
            result = Result(name, "FAIL", f"{type(exc).__name__}: {exc}",
                            (time.monotonic() - started) * 1000.0)
        self.results.append(result)
        print(f"[{result.status}] {result.name}: {result.detail}")

    def request(self, payload: bytes) -> bytes:
        if self.iso is None:
            raise RuntimeError("runner is not connected")
        return self._request(payload)

    def _request(self, payload: bytes) -> bytes:
        assert self.iso is not None
        self.iso.send(payload)
        return self.iso.receive()

    def test_session_control(self) -> None:
        response = self.request(bytes([0x10, 0x01]))
        expect_prefix(response, bytes([0x50, 0x01]))

    def test_ecu_reset(self) -> None:
        if not self.args.enable_reset:
            raise RuntimeError("SKIP: ECU reset requires --enable-reset")
        response = self.request(bytes([0x11, self.args.reset_type]))
        expect_prefix(response, bytes([0x51, self.args.reset_type]))

    def test_read_data_by_identifier(self) -> None:
        response = self.request(bytes([0x22]) + self.args.did)
        expect_prefix(response, bytes([0x62]) + self.args.did)

    def test_security_access_seed(self) -> None:
        response = self.request(bytes([0x27, self.args.security_seed_subfunction]))
        expect_prefix(response, bytes([0x67, self.args.security_seed_subfunction]))
        if len(response) <= 2:
            raise AssertionError("SecurityAccess returned an empty seed")

    def test_communication_control_policy(self) -> None:
        response = self.request(bytes([0x28, 0x00, 0x01]))
        expect_negative(response, 0x28)

    def test_io_control_policy(self) -> None:
        response = self.request(bytes([0x2F]) + self.args.did + bytes([0x03]))
        expect_negative(response, 0x2F)

    def test_routine_control_policy(self) -> None:
        response = self.request(bytes([0x31, 0x01, 0xFF, 0x00]))
        expect_negative(response, 0x31)

    def test_request_download_policy(self) -> None:
        if not self.args.enable_download:
            raise RuntimeError("SKIP: RequestDownload requires --enable-download")
        # Address/length format: 2 address bytes and 2 size bytes. The reference
        # runtime protects application/bootloader regions and may reject this.
        response = self.request(bytes([0x34, 0x22, 0x00, 0x00, 0x00, 0x10]))
        if response[0] not in (0x74, 0x7F):
            raise AssertionError(f"unexpected RequestDownload response: {response.hex(' ')}")

    def test_transfer_data_sequence_policy(self) -> None:
        response = self.request(bytes([0x36, 0x01, 0x00]))
        expect_negative(response, 0x36)

    def test_request_transfer_exit_policy(self) -> None:
        response = self.request(bytes([0x37]))
        expect_negative(response, 0x37)

    def test_tester_present(self) -> None:
        response = self.request(bytes([0x3E, 0x00]))
        expect_prefix(response, bytes([0x7E, 0x00]))

    def selected_tests(self) -> list[tuple[str, Callable[[], None]]]:
        tests: list[tuple[str, Callable[[], None]]] = [
            ("uds-session-control", self.test_session_control),
            ("uds-read-data-by-identifier", self.test_read_data_by_identifier),
            ("uds-security-access-seed", self.test_security_access_seed),
            ("uds-communication-control-policy", self.test_communication_control_policy),
            ("uds-io-control-policy", self.test_io_control_policy),
            ("uds-routine-control-policy", self.test_routine_control_policy),
            ("uds-transfer-data-sequence-policy", self.test_transfer_data_sequence_policy),
            ("uds-request-transfer-exit-policy", self.test_request_transfer_exit_policy),
            ("uds-tester-present", self.test_tester_present),
        ]
        if self.args.enable_download:
            tests.insert(6, ("uds-request-download-policy", self.test_request_download_policy))
        if self.args.enable_reset:
            tests.insert(1, ("uds-ecu-reset", self.test_ecu_reset))
        return tests

    def execute(self) -> int:
        tests = self.selected_tests()
        if self.args.dry_run:
            print("DRY RUN: no CAN interface opened")
            for name, _ in tests:
                print(f"[DRY-RUN] {name}")
            return 0

        self.setup()
        try:
            for name, test in tests:
                self.run(name, test)
        finally:
            self.close()

        failed = [result for result in self.results if result.status == "FAIL"]
        skipped = [result for result in self.results if result.status == "SKIP"]
        passed = [result for result in self.results if result.status == "PASS"]
        if self.args.json_out:
            output = {
                "schema_version": 1,
                "runner": "stm32f767-uds-acceptance-v1",
                "git_sha": os.environ.get("GIT_SHA", "unknown"),
                "interface": self.args.iface,
                "uds_tx_id": self.args.uds_tx_id,
                "uds_rx_id": self.args.uds_rx_id,
                "destructive_reset_enabled": self.args.enable_reset,
                "download_enabled": self.args.enable_download,
                "results": [asdict(result) for result in self.results],
            }
            with open(self.args.json_out, "w", encoding="utf-8") as handle:
                json.dump(output, handle, indent=2)
                handle.write("\n")
        print(f"SUMMARY: {len(passed)} passed, {len(failed)} failed, {len(skipped)} skipped")
        return 1 if failed else 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iface", default="can0")
    parser.add_argument("--uds-tx-id", type=lambda value: int(value, 0), default=0x7E0,
                        help="tester-to-ECU request ID")
    parser.add_argument("--uds-rx-id", type=lambda value: int(value, 0), default=0x7E8,
                        help="ECU-to-tester response ID")
    parser.add_argument("--timeout", type=float, default=1.0)
    parser.add_argument("--did", type=lambda value: bytes.fromhex(value.removeprefix("0x")),
                        default=bytes.fromhex("F180"), help="four-hex-digit DID")
    parser.add_argument("--security-seed-subfunction", type=lambda value: int(value, 0), default=0x01)
    parser.add_argument("--reset-type", type=lambda value: int(value, 0), default=0x01)
    parser.add_argument("--enable-reset", action="store_true",
                        help="enable ECU reset acceptance test")
    parser.add_argument("--enable-download", action="store_true",
                        help="enable bounded RequestDownload policy test")
    parser.add_argument("--json-out")
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
