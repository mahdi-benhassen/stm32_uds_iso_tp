#!/usr/bin/env python3
"""Safety-gated UDS/ISO-TP HIL inventory for Classical CAN and CAN FD."""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import subprocess
import sys
import time
from dataclasses import asdict, dataclass
from pathlib import Path


@dataclass
class Evidence:
    name: str
    profile: str
    can_id: str
    frame_format: str
    dlc: int
    data: str
    response_time_ms: float | None
    response: str
    nrc: str | None
    verdict: str
    executed: bool
    destructive: bool


def inventory(can_fd: bool, profile: str | None = None) -> list[Evidence]:
    profile = profile or ("can-fd" if can_fd else "classic-can")
    dlc = 64 if can_fd else 8
    values = [
        ("tester_present", 2, "3E00", False),
        ("session_control", 2, "1003", False),
        ("read_data_by_identifier", 3, "22F190", False),
        ("security_access_policy", 2, "2701", False),
        ("communication_control_policy", 3, "280001", False),
        ("routine_control_policy", 4, "31010203", False),
        ("request_download_denied_by_default", 4, "34440000", True),
        ("transfer_data_sequence_guard", 3, "3601AA", True),
        ("transfer_exit_guard", 1, "37", True),
        ("ecu_reset_guard", 2, "1101", True),
        ("isotp_sf_boundary", dlc, "0000000000000000", False),
        ("isotp_ff_4095_boundary", dlc, "1FFF000000000000", False),
        ("isotp_flow_control_wait_limit", 3, "310000", False),
        ("isotp_sequence_error", dlc, "2100000000000000", False),
        ("isotp_wrong_can_id", dlc, "", False),
    ]
    if can_fd:
        values += [
            ("isotp_canfd_sf_62_bytes", 64, "003E" + "00" * 62, False),
            ("isotp_extended_ff_over_4095", 64, "100000001388" + "00" * 58, False),
            ("isotp_canfd_brs_metadata", 64, "300000" + "00" * 61, False),
        ]
    return [
        Evidence(name, profile, f"0x{0x7E0:03X}",
                 "CAN-FD" if can_fd else "Classical CAN", item_dlc, data, None,
                 "not-executed", None, "NOT_EXECUTED", False, destructive)
        for name, item_dlc, data, destructive in values
    ]


def run_live(results: list[Evidence], channel: str, bitrate: int, data_bitrate: int | None,
             allow_destructive: bool) -> None:
    try:
        import can  # type: ignore
    except ImportError as exc:
        raise RuntimeError("live mode requires python-can; use --dry-run for report validation") from exc
    bus = can.Bus(interface="socketcan", channel=channel, bitrate=bitrate, fd=data_bitrate is not None)
    try:
        for result in results:
            if result.destructive and not allow_destructive:
                result.response = "safety-gated"
                result.verdict = "SKIPPED_DESTRUCTIVE"
                continue
            payload = bytes.fromhex(result.data) if result.data else b""
            message = can.Message(arbitration_id=int(result.can_id, 16), data=payload,
                                  is_fd=result.frame_format == "CAN-FD",
                                  bitrate_switch=data_bitrate is not None)
            started = time.monotonic()
            bus.send(message, timeout=0.2)
            reply = bus.recv(timeout=1.0)
            result.response_time_ms = (time.monotonic() - started) * 1000.0
            result.executed = True
            if reply is None:
                result.response = "timeout"
                result.verdict = "FAIL"
            else:
                result.response = reply.data.hex().upper()
                result.dlc = len(reply.data)
                result.verdict = "PASS"
    finally:
        bus.shutdown()


def provenance(board_profile: Path | None, analyzer: str, trace: Path | None) -> dict[str, object]:
    try:
        commit = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], text=True, stderr=subprocess.DEVNULL
        ).strip()
    except (OSError, subprocess.CalledProcessError):
        commit = "unknown"
    values: dict[str, object] = {
        "repository_commit": commit,
        "board_profile": str(board_profile) if board_profile else "not-specified",
        "analyzer": analyzer,
        "trace": str(trace) if trace else "not-attached",
        "trace_sha256": "not-attached",
    }
    if trace:
        values["trace_sha256"] = hashlib.sha256(trace.read_bytes()).hexdigest()
    if board_profile:
        values["board_profile_sha256"] = hashlib.sha256(board_profile.read_bytes()).hexdigest()
    else:
        values["board_profile_sha256"] = "not-specified"
    return values


def write_reports(results: list[Evidence], metadata: dict[str, object], json_path: Path | None,
                  csv_path: Path | None, report_path: Path | None) -> None:
    values = [asdict(result) for result in results]
    if json_path:
        json_path.write_text(json.dumps({"metadata": metadata, "results": values}, indent=2) + "\n", encoding="utf-8")
    if csv_path:
        with csv_path.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=list(values[0]))
            writer.writeheader()
            writer.writerows(values)
    if report_path:
        lines = ["# UDS/ISO-TP HIL report", "", "## Provenance", "", "| Field | Value |", "|---|---|"]
        lines += [f"| {key} | {value} |" for key, value in metadata.items()]
        lines += ["", "| Name | Profile | CAN ID | Format | DLC | Verdict |", "|---|---|---|---|---:|---|"]
        lines += [f"| {r.name} | {r.profile} | {r.can_id} | {r.frame_format} | {r.dlc} | {r.verdict} |" for r in results]
        report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--interface", default="vcan0")
    parser.add_argument("--bitrate", type=int, default=500000)
    parser.add_argument("--data-bitrate", type=int)
    parser.add_argument("--can-fd", action="store_true")
    parser.add_argument("--profile", choices=("classic-can", "c092-fdcan-classic", "can-fd"))
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--allow-destructive", action="store_true")
    parser.add_argument("--board-profile", type=Path)
    parser.add_argument("--analyzer", default="not-specified")
    parser.add_argument("--trace", type=Path)
    parser.add_argument("--json", type=Path)
    parser.add_argument("--csv", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    if args.profile == "can-fd" and (args.data_bitrate is None) and not args.dry_run:
        args.can_fd = True
    if args.profile and args.profile != "can-fd" and args.can_fd:
        parser.error("a Classic CAN profile cannot be combined with --can-fd")
    if args.can_fd and (args.data_bitrate is None) and not args.dry_run:
        parser.error("live CAN-FD mode requires --data-bitrate")
    if args.allow_destructive and not args.dry_run:
        print("WARNING: destructive cases enabled", file=sys.stderr)
    if args.board_profile and not args.board_profile.is_file():
        parser.error("--board-profile must point to an existing file")
    if args.trace and not args.trace.is_file():
        parser.error("--trace must point to an existing file")
    metadata = provenance(args.board_profile, args.analyzer, args.trace)
    selected_profile = args.profile or ("can-fd" if args.can_fd else "classic-can")
    if selected_profile == "can-fd":
        args.can_fd = True
    results = inventory(args.can_fd, selected_profile)
    if args.dry_run:
        for result in results:
            result.verdict = "DRY_RUN"
    else:
        run_live(results, args.interface, args.bitrate, args.data_bitrate, args.allow_destructive)
    write_reports(results, metadata, args.json, args.csv, args.report)
    print(json.dumps({"profile": selected_profile, "cases": len(results),
                      "verdicts": sorted({result.verdict for result in results})}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
