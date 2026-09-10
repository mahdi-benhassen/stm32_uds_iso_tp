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
    expected_payload_prefixes: tuple[str, ...]
    expected_pci: int | None
    expect_silence: bool


def inventory(can_fd: bool, profile: str | None = None) -> list[Evidence]:
    profile = profile or ("can-fd" if can_fd else "classic-can")
    dlc = 64 if can_fd else 8
    did_response = ("62F190",) if profile == "classic-can" else ("7F2211",)
    values = [
        # `data` is a complete on-wire ISO-TP N_PDU, including PCI—not a raw UDS payload.
        ("tester_present", 3, "023E00", False, ("7E00",), None, False, 0x7E0),
        ("session_control", 3, "021003", False, ("5003",), None, False, 0x7E0),
        ("read_data_by_identifier", 4, "0322F190", False, did_response, None, False, 0x7E0),
        ("security_access_policy", 3, "022701", False, ("7F2711",), None, False, 0x7E0),
        ("communication_control_policy", 4, "03280001", False, ("7F2811",), None, False, 0x7E0),
        ("routine_control_policy", 5, "0431010203", False, ("7F3111",), None, False, 0x7E0),
        ("request_download_denied_by_default", 6, "053400440000", True, (), None, False, 0x7E0),
        ("transfer_data_sequence_guard", 4, "033601AA", True, (), None, False, 0x7E0),
        ("transfer_exit_guard", 2, "0137", True, (), None, False, 0x7E0),
        ("ecu_reset_guard", 3, "021101", True, (), None, False, 0x7E0),
        ("isotp_sf_boundary", dlc, "00" + "00" * (dlc - 1), False, (), None, True, 0x7E0),
        ("isotp_ff_4095_boundary", dlc, "1FFF" + "00" * (dlc - 2), False, (), 3, False, 0x7E0),
        ("isotp_flow_control_wait_limit", 3, "310000", False, (), None, True, 0x7E0),
        ("isotp_sequence_error", dlc, "21" + "00" * (dlc - 1), False, (), None, True, 0x7E0),
        ("isotp_wrong_can_id", 3, "023E00", False, (), None, True, 0x7E1),
    ]
    if can_fd:
        values += [
            ("isotp_canfd_sf_62_bytes", 64, "003E" + "00" * 62, False, ("7F3E13",), None, False, 0x7E0),
            ("isotp_extended_ff_over_4095", 64, "100000001388" + "00" * 58, False, (), 3, False, 0x7E0),
            ("isotp_canfd_brs_metadata", 64, "300000" + "00" * 61, False, (), None, True, 0x7E0),
        ]
    return [
        Evidence(name, profile, f"0x{can_id:03X}",
                 "CAN-FD" if can_fd else "Classical CAN", item_dlc, data, None,
                 "not-executed", None, "NOT_EXECUTED", False, destructive,
                 expected_prefixes, expected_pci, expect_silence)
        for name, item_dlc, data, destructive, expected_prefixes, expected_pci, expect_silence,
        can_id in values
    ]


def receive_isotp(bus: object, can_module: object, response_id: int, request_id: int,
                  can_fd: bool, timeout_s: float) -> tuple[str, bytes | None, bytes | None]:
    """Receive one correlated ISO-TP response and reassemble it when needed.

    Returns a kind (``payload``, ``pci``, or ``timeout``), decoded payload when
    available, and the raw first frame for PCI-only expectations.
    """
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        message = bus.recv(timeout=max(0.0, deadline - time.monotonic()))
        if message is None or message.arbitration_id != response_id or not message.data:
            continue
        data = bytes(message.data)
        frame_type = data[0] >> 4
        if frame_type == 0:
            length = data[0] & 0x0F
            offset = 1
            if length == 0:
                if not can_fd or len(data) < 2:
                    return "pci", None, data
                length = data[1]
                offset = 2
            if offset + length > len(data):
                return "pci", None, data
            return "payload", data[offset:offset + length], data
        if frame_type == 1:
            if len(data) < 2:
                return "pci", None, data
            length = ((data[0] & 0x0F) << 8) | data[1]
            offset = 2
            if length == 0:
                if len(data) < 6:
                    return "pci", None, data
                length = int.from_bytes(data[2:6], "big")
                offset = 6
            payload = bytearray(data[offset:min(len(data), offset + length)])
            flow_control = can_module.Message(arbitration_id=request_id,
                                              data=bytes((0x30, 0x00, 0x00)),
                                              is_fd=can_fd, bitrate_switch=can_fd)
            bus.send(flow_control, timeout=0.2)
            sequence = 1
            while len(payload) < length and time.monotonic() < deadline:
                consecutive = bus.recv(timeout=max(0.0, deadline - time.monotonic()))
                if consecutive is None or consecutive.arbitration_id != response_id or not consecutive.data:
                    continue
                chunk = bytes(consecutive.data)
                if (chunk[0] >> 4) != 2 or (chunk[0] & 0x0F) != sequence:
                    return "pci", None, chunk
                payload.extend(chunk[1:min(len(chunk), 1 + length - len(payload))])
                sequence = (sequence + 1) & 0x0F
            return ("payload", bytes(payload), data) if len(payload) == length else ("timeout", None, None)
        return "pci", None, data
    return "timeout", None, None


def validate_response(result: Evidence, kind: str, payload: bytes | None, raw: bytes | None) -> bool:
    if kind == "timeout":
        result.response = "timeout"
        result.verdict = "PASS" if result.expect_silence else "FAIL"
        return result.expect_silence
    if kind == "payload" and payload is not None:
        result.response = payload.hex().upper()
        if len(payload) >= 3 and payload[0] == 0x7F:
            result.nrc = f"0x{payload[2]:02X}"
        matched = any(result.response.startswith(prefix) for prefix in result.expected_payload_prefixes)
        result.verdict = "PASS" if matched else "FAIL"
        return matched
    result.response = raw.hex().upper() if raw is not None else "malformed"
    matched = raw is not None and result.expected_pci == (raw[0] >> 4)
    result.verdict = "PASS" if matched else "FAIL"
    return matched


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
            kind, response_payload, raw = receive_isotp(
                bus, can, 0x7E8, int(result.can_id, 16), result.frame_format == "CAN-FD", 1.0)
            result.response_time_ms = (time.monotonic() - started) * 1000.0
            result.executed = True
            validate_response(result, kind, response_payload, raw)
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
        json_path.parent.mkdir(parents=True, exist_ok=True)
        json_path.write_text(json.dumps({"metadata": metadata, "results": values}, indent=2) + "\n", encoding="utf-8")
    if csv_path:
        csv_path.parent.mkdir(parents=True, exist_ok=True)
        with csv_path.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=list(values[0]))
            writer.writeheader()
            writer.writerows(values)
    if report_path:
        report_path.parent.mkdir(parents=True, exist_ok=True)
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
