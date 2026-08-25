#!/usr/bin/env python3
"""Validate the software release evidence bundle before artifact upload."""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


ARTIFACT_BASENAME = "stm32f767_canopen"
ARTIFACT_SUFFIXES = (".elf", ".hex", ".bin", ".map")
# The evidence bundle is generated only by the default firmware matrix job.
# Optional personalities are validated by their own build-and-upload jobs.
PERSONALITIES = ("ci-firmware",)


def require_file(path: Path) -> None:
    if not path.is_file() or path.stat().st_size == 0:
        raise AssertionError(f"missing or empty release artifact: {path}")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    manifest_path = root / "build/ci-build-manifest.json"
    manifest_text_path = root / "build/ci-build-manifest.txt"
    require_file(manifest_path)
    require_file(manifest_text_path)
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema") != "stm32-canopen-build-manifest-v2":
        raise AssertionError("unexpected build manifest schema")
    od = root / "Generated/OD.c"
    linker = root / "STM32F767xx_FLASH.ld"
    require_file(od)
    require_file(linker)
    if sha256(od) != manifest["inputs"]["object_dictionary_sha256"]:
        raise AssertionError("Object Dictionary hash does not match build manifest")
    if sha256(linker) != manifest["inputs"]["linker_script_sha256"]:
        raise AssertionError("linker script hash does not match build manifest")

    for personality in PERSONALITIES:
        base = root / f"build/{personality}/{ARTIFACT_BASENAME}"
        for suffix in ARTIFACT_SUFFIXES:
            require_file(base.with_name(base.name + suffix))

    junit_path = root / "build/reports/test-results.xml"
    coverage_path = root / "build/reports/coverage-summary.json"
    sanitizer_path = root / "build/reports/sanitizer-report.txt"
    require_file(junit_path)
    require_file(coverage_path)
    require_file(sanitizer_path)
    junit_root = ET.parse(junit_path).getroot()
    if junit_root.tag != "testsuite" or int(junit_root.attrib.get("tests", "0")) == 0:
        raise AssertionError("JUnit report has no test cases")
    if int(junit_root.attrib.get("failures", "0")) != 0:
        raise AssertionError("JUnit report contains failed validation cases")
    coverage = json.loads(coverage_path.read_text(encoding="utf-8"))
    if coverage.get("schema") != "stm32-canopen-host-coverage-v1" or not coverage.get("passed"):
        raise AssertionError("coverage report did not pass its configured thresholds")
    sanitizer = sanitizer_path.read_text(encoding="utf-8")
    if "status=0" not in sanitizer:
        raise AssertionError("sanitizer report does not show status=0")
    print("release artifact validation: PASS")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, KeyError, json.JSONDecodeError, ET.ParseError) as exc:
        print(f"release artifact validation: FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
