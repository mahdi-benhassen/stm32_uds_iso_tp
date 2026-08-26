#!/usr/bin/env python3
"""Deterministic software-only mock-hardware validation for C092 ECUReset recovery.

This harness models the observable FDCAN/ISO-TP/UDS boundaries. It is intentionally
not a physical CAN or STM32 emulator: it validates ordering, bounded handoff,
completion-before-reset, and zero-delay post-reset request behavior.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

RESET_REQUEST = bytes((0x02, 0x11, 0x01))
SESSION_REQUEST = bytes((0x02, 0x10, 0x01))
TESTER_PRESENT_REQUEST = bytes((0x02, 0x3E, 0x00))
INVALID_REQUEST = bytes((0x02, 0x99, 0x00))


@dataclass
class MockTrace:
    events: list[tuple[str, int]] = field(default_factory=list)
    counters: dict[str, int] = field(
        default_factory=lambda: {
            "fdcan_start": 0,
            "uds_init_done": 0,
            "rx_first_after_reset": 0,
            "rx_accepted": 0,
            "rx_rejected_not_initialized": 0,
            "isotp_rx": 0,
            "uds_request": 0,
            "tx_submitted": 0,
            "tx_complete": 0,
            "reset_executed": 0,
            "responses": 0,
            "mailbox_full": 0,
        }
    )

    def mark(self, name: str, now_us: int) -> None:
        self.events.append((name, now_us))
        if name.lower() in self.counters:
            self.counters[name.lower()] += 1


@dataclass
class MockHardware:
    trace: MockTrace = field(default_factory=MockTrace)
    now_us: int = 0
    fdcan_started: bool = False
    rx_notification_enabled: bool = False
    transport_initialized: bool = False
    endpoint_initialized: bool = False
    mailbox: bytes | None = None
    tx_pending: bytes | None = None
    last_response: bytes | None = None
    first_rx_seen_since_reset: bool = False

    def boot(self, *, correct_order: bool = True) -> None:
        self.fdcan_started = False
        self.rx_notification_enabled = False
        self.transport_initialized = False
        self.endpoint_initialized = False
        self.mailbox = None
        self.tx_pending = None
        self.last_response = None
        self.first_rx_seen_since_reset = False
        self.trace.mark("BOOT_ENTRY", self.now_us)
        self.trace.mark("HAL_INIT_DONE", self.now_us)
        self.trace.mark("FDCAN_INIT_DONE", self.now_us)
        self.trace.mark("FILTER_DONE", self.now_us)

        if correct_order:
            self.initialize_application()
            self.enable_rx_and_start()
        else:
            self.enable_rx_and_start()

    def initialize_application(self) -> None:
        self.transport_initialized = True
        self.endpoint_initialized = True
        self.trace.mark("UDS_INIT_DONE", self.now_us)

    def enable_rx_and_start(self) -> None:
        if not self.transport_initialized or not self.endpoint_initialized:
            self.trace.mark("FDCAN_START_BEFORE_UDS", self.now_us)
        self.rx_notification_enabled = True
        self.fdcan_started = True
        self.trace.mark("FDCAN_START", self.now_us)

    def receive_from_bus(self, frame: bytes) -> None:
        if not self.fdcan_started or not self.rx_notification_enabled:
            return
        if not self.endpoint_initialized:
            self.trace.mark("RX_REJECTED_NOT_INITIALIZED", self.now_us)
            return
        if self.mailbox is not None:
            self.trace.mark("MAILBOX_FULL", self.now_us)
            return
        if not self.first_rx_seen_since_reset:
            self.trace.mark("RX_FIRST_AFTER_RESET", self.now_us)
            self.first_rx_seen_since_reset = True
        self.mailbox = frame
        self.trace.mark("RX_ACCEPTED", self.now_us)

    def process_mainline(self) -> None:
        if self.mailbox is None or not self.endpoint_initialized:
            return
        request = self.mailbox
        self.mailbox = None
        self.trace.mark("ISOTP_RX", self.now_us)
        self.trace.mark("UDS_REQUEST", self.now_us)
        if request == RESET_REQUEST:
            response = bytes((0x02, 0x51, 0x01))
        elif request == SESSION_REQUEST:
            response = bytes((0x06, 0x50, 0x01, 0x00, 0x32, 0x13, 0x88))
        elif request == TESTER_PRESENT_REQUEST:
            response = bytes((0x02, 0x7E, 0x00))
        else:
            response = bytes((0x03, 0x7F, request[1] if len(request) > 1 else 0x00, 0x11))
        self.last_response = response
        self.tx_pending = response
        self.trace.mark("TX_SUBMITTED", self.now_us)
        if request != RESET_REQUEST:
            self.trace.counters["responses"] += 1

    def complete_tx(self) -> None:
        if self.tx_pending is None:
            raise AssertionError("TX completion reported with no queued response")
        self.tx_pending = None
        self.trace.mark("TX_COMPLETE", self.now_us)

    def execute_reset_after_completed_response(self) -> None:
        if self.tx_pending is not None:
            raise AssertionError("reset executed before ECUReset response TX completion")
        self.trace.mark("RESET_EXECUTED", self.now_us)
        self.boot(correct_order=True)

    def send_and_expect(self, request: bytes, expected_prefix: bytes) -> None:
        self.receive_from_bus(request)
        self.process_mainline()
        if self.last_response is None or not self.last_response.startswith(expected_prefix):
            raise AssertionError(
                f"unexpected response for {request.hex(' ')}: {self.last_response!r}"
            )
        self.complete_tx()


def run_unsafe_window_probe() -> dict[str, int]:
    """Confirm the guard rejects the exact bad-order window rather than weakening it."""
    model = MockHardware()
    model.boot(correct_order=False)
    model.receive_from_bus(SESSION_REQUEST)
    if model.trace.counters["rx_rejected_not_initialized"] != 1:
        raise AssertionError("pre-initialization RX was not rejected and recorded")
    model.initialize_application()
    if model.trace.counters["rx_rejected_not_initialized"] != 1:
        raise AssertionError("pre-initialization rejection count changed unexpectedly")
    return {
        "rx_rejected_not_initialized": model.trace.counters["rx_rejected_not_initialized"],
        "responses": model.trace.counters["responses"],
    }


def run_reset_campaign(cycles: int, requests_per_cycle: int, first_delay_us: int) -> MockHardware:
    if cycles <= 0 or requests_per_cycle < 4 or first_delay_us < 0:
        raise ValueError(
            "cycles must be positive, requests_per_cycle must be at least 4, "
            "and delay must be non-negative"
        )

    model = MockHardware()
    model.boot(correct_order=True)
    for cycle in range(cycles):
        model.send_and_expect(RESET_REQUEST, bytes((0x02, 0x51, 0x01)))
        model.execute_reset_after_completed_response()
        model.now_us += first_delay_us

        for request_index in range(requests_per_cycle):
            if request_index == 0:
                request, expected = SESSION_REQUEST, bytes((0x06, 0x50, 0x01))
            elif request_index == 1:
                request, expected = TESTER_PRESENT_REQUEST, bytes((0x02, 0x7E, 0x00))
            elif request_index == 2:
                request, expected = INVALID_REQUEST, bytes((0x03, 0x7F, 0x99))
            else:
                request, expected = SESSION_REQUEST, bytes((0x06, 0x50, 0x01))
            model.send_and_expect(request, expected)
            model.now_us += 1
    expected_transactions = cycles * requests_per_cycle
    counters = model.trace.counters
    if counters["reset_executed"] != cycles:
        raise AssertionError(f"expected {cycles} resets, got {counters['reset_executed']}")
    if counters["responses"] != expected_transactions:
        raise AssertionError(
            f"expected {expected_transactions} post-reset responses, got {counters['responses']}"
        )
    if counters["tx_complete"] != cycles + expected_transactions:
        raise AssertionError("every reset and post-reset response must complete TX")
    return model


def run_ctest(ctest_dir: Path) -> None:
    pattern = "uds_iso_tp_c092_immediate_reset_rx_contract|uds_iso_tp_reset_recovery_contract"
    subprocess.run(
        ["ctest", "--test-dir", str(ctest_dir), "-R", pattern, "--output-on-failure"],
        check=True,
    )


def write_report(
    path: Path,
    model: MockHardware,
    cycles: int,
    requests_per_cycle: int,
    first_delay_us: int,
    unsafe_window_probe: dict[str, int],
) -> None:
    report = {
        "model": "software-only deterministic C092 mock hardware",
        "physical_hil": False,
        "cycles": cycles,
        "requests_per_cycle": requests_per_cycle,
        "post_reset_transactions": cycles * requests_per_cycle,
        "first_request_delay_us": first_delay_us,
        "unsafe_window_probe": unsafe_window_probe,
        "counters": model.trace.counters,
        "first_events": {},
        "limitations": [
            "does not execute on STM32C092 silicon",
            "does not exercise vendor HAL registers, NVIC latency, transceiver, or CAN wiring",
            "does not replace Keil/Arm Compiler 6 build or CAN-analyzer evidence",
        ],
    }
    for name, timestamp in model.trace.events:
        report["first_events"].setdefault(name, timestamp)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cycles", type=int, default=100)
    parser.add_argument("--requests-per-cycle", type=int, default=10)
    parser.add_argument("--first-request-delay-us", type=int, default=0)
    parser.add_argument("--ctest-dir", type=Path)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--skip-window-probe", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        if args.ctest_dir is not None:
            run_ctest(args.ctest_dir)
        unsafe_window_probe = (
            {"skipped": 1}
            if args.skip_window_probe
            else run_unsafe_window_probe()
        )
        model = run_reset_campaign(args.cycles, args.requests_per_cycle, args.first_request_delay_us)
        if args.report is not None:
            write_report(
                args.report,
                model,
                args.cycles,
                args.requests_per_cycle,
                args.first_request_delay_us,
                unsafe_window_probe,
            )
        print(
            "C092 mock-hardware validation passed: "
            f"{args.cycles} resets, "
            f"{args.cycles * args.requests_per_cycle} post-reset transactions, "
            f"first-request-delay={args.first_request_delay_us} us"
        )
        print("Physical HIL: not performed; this is a deterministic software model.")
        return 0
    except (AssertionError, OSError, subprocess.CalledProcessError, ValueError) as exc:
        print(f"C092 mock-hardware validation FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
