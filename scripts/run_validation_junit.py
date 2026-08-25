#!/usr/bin/env python3
"""Run software validation commands and emit JUnit XML.

The resulting report records software-only evidence. Hardware, EMC, security,
and official conformance claims remain outside this runner.
"""
from __future__ import annotations

import argparse
import os
import subprocess
import sys
import time
from pathlib import Path
from xml.sax.saxutils import escape


COMMANDS = [
    ("repository", ["python3", "scripts/validate_repository.py"]),
    ("object_dictionary", ["python3", "scripts/validate_od.py"]),
    ("cia418", ["python3", "scripts/validate_cia418.py"]),
    ("firmware_contracts", ["python3", "tests/test_firmware_configuration.py"]),
    ("wire_contract", ["python3", "tests/test_canopen_wire_contract.py"]),
    ("core_vectors", ["python3", "tests/conformance/run_core_vectors.py"]),
    ("uds_isotp_contract", ["python3", "tests/run_uds_isotp_contract.py"]),
    ("nmea2000_gateway_contract", ["python3", "tests/run_nmea2000_gateway_contract.py"]),
    ("hardware_acceptance_dry_run", ["python3", "tests/hardware/run_uds_cia302_acceptance.py", "--dry-run"]),
]


def run_case(root: Path, name: str, command: list[str]) -> dict[str, object]:
    environment = os.environ.copy()
    environment["PYTHONDONTWRITEBYTECODE"] = "1"
    environment["PYTHONPATH"] = f"{root}:{root / 'tests'}:{environment.get('PYTHONPATH', '')}"
    started = time.monotonic()
    completed = subprocess.run(command, cwd=root, env=environment, text=True, capture_output=True, check=False)
    duration = time.monotonic() - started
    output = (completed.stdout + completed.stderr).strip()
    return {"name": name, "command": command, "duration": duration, "returncode": completed.returncode, "output": output}


def junit(results: list[dict[str, object]]) -> str:
    failures = sum(1 for result in results if result["returncode"] != 0)
    total_time = sum(float(result["duration"]) for result in results)
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<testsuite name="stm32f767-canopen-software-validation" tests="{len(results)}" failures="{failures}" errors="0" time="{total_time:.3f}">',
    ]
    for result in results:
        name = escape(str(result["name"]))
        duration = float(result["duration"])
        command = escape(" ".join(str(part) for part in result["command"]))
        lines.append(f'  <testcase classname="repository.validation" name="{name}" time="{duration:.3f}">')
        if result["returncode"] != 0:
            message = escape(f"command exited with status {result['returncode']}")
            output = escape(str(result["output"]))
            lines.append(f'    <failure message="{message}">{output}</failure>')
        else:
            lines.append(f'    <system-out>command: {command}\n{escape(str(result["output"]))}</system-out>')
        lines.append("  </testcase>")
    lines.append("</testsuite>")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = args.root.resolve()
    results = [run_case(root, name, command) for name, command in COMMANDS]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(junit(results), encoding="utf-8")
    for result in results:
        status = "PASS" if result["returncode"] == 0 else "FAIL"
        print(f"{status:4} {result['name']} ({float(result['duration']):.2f}s)")
    return 1 if any(result["returncode"] != 0 for result in results) else 0


if __name__ == "__main__":
    raise SystemExit(main())
