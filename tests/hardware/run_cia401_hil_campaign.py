#!/usr/bin/env python3
"""Initialize and validate the CiA 401 physical/HIL campaign record.

This command deliberately supports only a pending dry-run. It creates the
case-level evidence structure, but it never marks a physical result PASS and
never pretends that a sandbox host can exercise STM32 silicon, transceivers,
power interruption, EMC, or calibrated equipment.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_PLAN = ROOT / "tests" / "hardware" / "cia401_hil_plan.json"


def fail(message: str) -> None:
    print(f"CiA 401 HIL initializer failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def parse_equipment(values: list[str]) -> dict[str, str]:
    result: dict[str, str] = {}
    for value in values:
        if "=" not in value:
            fail(f"equipment must use key=value syntax: {value!r}")
        key, identifier = value.split("=", 1)
        if not key or not identifier:
            fail(f"equipment key and identifier must be non-empty: {value!r}")
        result[key] = identifier
    return result


def sha256(path: Path | None) -> str | None:
    """Return an image digest when supplied; absent image means no digest."""
    if path is None or not path.is_file():
        return None
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--plan", type=Path, default=DEFAULT_PLAN)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--release-commit", required=True)
    parser.add_argument("--firmware-sha", required=True)
    parser.add_argument("--board-serial", default="TBD-HARDWARE")
    parser.add_argument("--board-revision", default="TBD-HARDWARE")
    parser.add_argument("--operator", default="TBD-OPERATOR")
    parser.add_argument("--equipment", action="append", default=[])
    parser.add_argument("--firmware-image", type=Path)
    parser.add_argument("--dry-run", action="store_true", help="emit a pending record; required for this safe initializer")
    args = parser.parse_args()
    if not args.dry_run:
        fail("only --dry-run is supported; physical execution requires the approved HIL harness and equipment")
    try:
        plan = json.loads(args.plan.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        fail(f"cannot load campaign plan: {exc}")
    if plan.get("schema") != "stm32-canopen-cia401-hil-v1":
        fail("unsupported campaign plan schema")
    if plan.get("personality") != "cia401":
        fail("campaign plan personality is not cia401")

    cases: list[dict[str, object]] = []
    for campaign in plan["campaigns"]:
        for case in campaign["cases"]:
            cases.append({
                "campaign": campaign["id"],
                "case": case,
                "status": "PENDING",
                "result": None,
                "measurements": {},
                "trace": None,
                "note": "Requires physical STM32F767 DUT and controlled HIL equipment.",
            })

    output = {
        "schema": "stm32-canopen-cia401-hil-evidence-v1",
        "status": "PENDING",
        "created_at_utc": datetime.now(timezone.utc).isoformat(),
        "release_commit": args.release_commit,
        "firmware_sha": args.firmware_sha,
        "firmware_image_sha256": sha256(args.firmware_image),
        "board": {"serial": args.board_serial, "revision": args.board_revision},
        "operator": args.operator,
        "equipment": parse_equipment(args.equipment),
        "plan": plan,
        "measurement_schema": plan["measurement_schema"],
        "measurement_capture": {
            "core_clock_hz": None,
            "can_bitrate": None,
            "can_sample_point": None,
            "bus_load_percent": None,
            "tim7_isr_cycles_max": None,
            "tim7_period_cycles_max": None,
            "tim7_warning_count": None,
            "tim7_overrun_count": None,
            "mainline_cycles_max": None,
            "app_interrupt_cycles_max": None,
            "sync_cycles_max": None,
            "rpdo_cycles_max": None,
            "cia401_cycles_max": None,
            "cia402_cycles_max": None,
            "cia418_cycles_max": None,
            "tpdo_cycles_max": None,
            "can_irq_cycles_max": {},
            "can_fifo_overflow_count": None,
            "can_rx_to_rpdo_latency_cycles_max": None,
            "can_rx_to_application_latency_cycles_max": None,
            "rpdo_to_gpio_output_latency_cycles_max": None,
            "sync_to_tpdo_latency_cycles_max": None,
            "temperature_c": None,
            "supply_voltage_v": None,
            "bus_load_campaign": [
                {
                    "bus_load_percent": load_percent,
                    "tim7_isr_cycles_max": None,
                    "tim7_warning_count": None,
                    "tim7_overrun_count": None,
                    "can_irq_cycles_max": {},
                    "can_fifo_overflow_count": None,
                }
                for load_percent in plan["measurement_schema"]["bus_load_campaign"]["load_percentages"]
            ],
        },
        "cases": cases,
        "external_gate": True,
        "pass_claim_allowed": False,
        "note": "Initializer only. No physical test was executed and no PASS result is asserted.",
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"Wrote pending CiA 401 HIL evidence scaffold: {args.output}")
    print(f"Cases initialized: {len(cases)}; physical execution required; PASS claims disabled.")


if __name__ == "__main__":
    main()
