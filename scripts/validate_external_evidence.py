#!/usr/bin/env python3
"""Validate archived physical/HIL/qualification evidence for production release.

This validator intentionally accepts only explicit, reviewable PASS records. It
never creates evidence and never treats software-only reports as hardware proof.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

SCHEMA = "stm32-canopen-release-evidence-v1"
REQUIRED_RECORDS = (
    "board_electrical_review.md",
    "physical_can_interoperability.md",
    "bus_off_campaign.md",
    "flash_power_loss_endurance.md",
    "watchdog_timing.md",
    "cia401_acceptance.md",
    "cia402_acceptance.md",
    "cia302_lss_commissioning.md",
    "security_update_approval.md",
    "formal_canopen_conformance.md",
)
KEY_VALUE = re.compile(r"^([A-Za-z][A-Za-z0-9_.-]*)\s*:\s*(.+?)\s*$")


def parse_record(path: Path) -> dict[str, str]:
    fields: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        match = KEY_VALUE.match(line)
        if match:
            fields[match.group(1).lower()] = match.group(2).strip()
    return fields


def fail(message: str) -> int:
    print(f"external evidence: FAIL: {message}", file=sys.stderr)
    return 1


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("evidence_dir", type=Path)
    parser.add_argument("--release-commit", required=True)
    args = parser.parse_args()
    root = args.evidence_dir
    if not root.is_dir():
        return fail(f"directory does not exist: {root}")

    manifest_path = root / "release-evidence-manifest.json"
    if not manifest_path.is_file():
        return fail("release-evidence-manifest.json is missing")
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return fail(f"invalid release evidence manifest: {exc}")
    if manifest.get("schema") != SCHEMA:
        return fail("manifest schema is not stm32-canopen-release-evidence-v1")
    if manifest.get("release_commit") != args.release_commit:
        return fail("manifest release_commit does not match the release under review")
    if manifest.get("status") != "PASS":
        return fail("manifest status must be PASS")
    if not manifest.get("board_revision") or not manifest.get("hardware_serial"):
        return fail("manifest must identify board_revision and hardware_serial")

    for name in REQUIRED_RECORDS:
        path = root / name
        if not path.is_file() or not path.read_text(encoding="utf-8").strip():
            return fail(f"missing or empty evidence record: {name}")
        fields = parse_record(path)
        if fields.get("status") != "PASS":
            return fail(f"{name} must contain a machine-readable 'status: PASS' record")
        if fields.get("release_commit") != args.release_commit:
            return fail(f"{name} release_commit does not match the release under review")
        if not fields.get("evidence_id") or not fields.get("reviewer"):
            return fail(f"{name} must identify evidence_id and reviewer")

    socketcan = root / "release-socketcan-status.txt"
    if socketcan.is_file():
        fields = parse_record(socketcan)
        if fields.get("status") == "available" and fields.get("native_runtime_tests") == "passed":
            pass
        elif fields.get("status") == "unavailable":
            physical = parse_record(root / "physical_can_interoperability.md")
            if physical.get("status") != "PASS" or physical.get("evidence_type") != "hardware":
                return fail("native SocketCAN is unavailable and no physical CAN evidence is declared")
        else:
            return fail("SocketCAN status exists but is not a passing native run or explicit unavailable record")

    print(f"external evidence: PASS ({len(REQUIRED_RECORDS)} records; release={args.release_commit})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
