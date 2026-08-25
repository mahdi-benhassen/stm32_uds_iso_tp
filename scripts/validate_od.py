#!/usr/bin/env python3
"""Validate CANopenNode Object Dictionary source artifacts.

Default mode verifies the reference EDS plus its required CiA 401/CiA 402
indices. --generic validates an imported objdictgen C/H pair without imposing
this reference product profile.
"""
from __future__ import annotations

import argparse
import configparser
import re
import sys
from pathlib import Path

from cia402_catalog import REQUESTED_INDICES

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_EDS = ROOT / "ObjectDictionary" / "stm32f767_canopen_reference.eds"
DEFAULT_OD_C = ROOT / "Generated" / "OD.c"
DEFAULT_OD_H = ROOT / "Generated" / "OD.h"
PROFILE_INDICES = sorted({
    0x6000, 0x603F, 0x6040, 0x6041, 0x6060, 0x6061, 0x6064, 0x606C,
    0x6071, 0x6077, 0x607A, 0x60FF, 0x6200, 0x6401, 0x6411, 0x6422,
    *REQUESTED_INDICES,
})


def fail(message: str) -> None:
    print(f"OD validation failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def parse_od_entries(od_c: Path) -> list[int]:
    source = od_c.read_text(encoding="utf-8")
    od_list_match = re.search(r"static .*?OD_entry_t ODList\[\] = \{(?P<entries>.*?)\n\};", source, re.DOTALL)
    if od_list_match is None:
        fail(f"could not locate CANopenNode ODList in {od_c}")
    entries = [int(match, 16) for match in re.findall(r"\{0x([0-9A-Fa-f]{4}),", od_list_match.group("entries"))]
    entries = [index for index in entries if index != 0]
    if not entries:
        fail(f"{od_c} contains no ODList entries")
    if entries != sorted(entries):
        fail(f"{od_c} ODList indices are not ascending")
    if len(entries) != len(set(entries)):
        fail(f"{od_c} ODList has duplicate indices")
    return entries


def validate_header(od_h: Path) -> None:
    header = od_h.read_text(encoding="utf-8")
    if "OD_entry_t" not in header and "OD_APP_t" not in header:
        fail(f"{od_h} does not expose a CANopenNode Object Dictionary type")
    if "OD.h" not in od_h.name and "OD" not in header:
        fail(f"{od_h} does not appear to be a generated CANopenNode OD header")


def validate_eds(eds: Path, profile: list[int] | None) -> tuple[int, set[int]]:
    parser = configparser.ConfigParser(interpolation=None, strict=True)
    parser.optionxform = str
    with eds.open(encoding="utf-8") as handle:
        parser.read_file(handle)
    if not parser.has_section("OptionalObjects"):
        fail(f"{eds} lacks [OptionalObjects]")
    optional = parser["OptionalObjects"]
    declared = int(optional["SupportedObjects"], 0)
    listed = [value for key, value in optional.items() if key != "SupportedObjects"]
    if declared != len(listed):
        fail(f"{eds} declares {declared} optional objects but lists {len(listed)}")
    listed_indices = {int(value, 0) for value in listed}
    if profile is not None:
        missing = [f"0x{index:04X}" for index in profile if index not in listed_indices]
        if missing:
            fail(f"{eds} optional-object list lacks " + ", ".join(missing))
        missing_sections = [f"0x{index:04X}" for index in profile if not parser.has_section(f"{index:04X}")]
        if missing_sections:
            fail(f"{eds} lacks sections " + ", ".join(missing_sections))
    return declared, listed_indices


def main() -> None:
    argument_parser = argparse.ArgumentParser()
    argument_parser.add_argument("--generic", action="store_true", help="validate an imported C/H pair without reference-profile rules")
    argument_parser.add_argument("--od-c", type=Path, default=DEFAULT_OD_C)
    argument_parser.add_argument("--od-h", type=Path, default=DEFAULT_OD_H)
    argument_parser.add_argument("--eds", type=Path, help="optional EDS to validate")
    args = argument_parser.parse_args()

    if not args.od_c.is_file() or not args.od_h.is_file():
        fail("both --od-c and --od-h must identify regular files")
    entries = parse_od_entries(args.od_c)
    validate_header(args.od_h)

    if args.generic:
        declared = None
        if args.eds is not None:
            declared, _ = validate_eds(args.eds, None)
        suffix = f"; {declared} EDS optional objects" if declared is not None else ""
        print(f"Generic OD validation passed: {len(entries)} sorted OD entries{suffix}.")
        return

    eds = args.eds if args.eds is not None else DEFAULT_EDS
    if not eds.is_file():
        fail(f"reference EDS not found: {eds}")
    declared, _ = validate_eds(eds, PROFILE_INDICES)
    missing_c = [f"0x{index:04X}" for index in PROFILE_INDICES if index not in entries]
    if missing_c:
        fail("Generated/OD.c lacks " + ", ".join(missing_c))
    header = args.od_h.read_text(encoding="utf-8")
    for index in PROFILE_INDICES:
        if f"H{index:04X}" not in header:
            fail(f"Generated/OD.h lacks an entry shortcut for 0x{index:04X}")
    print(
        "OD validation passed: "
        f"{len(entries)} sorted OD entries; {declared} EDS optional objects; "
        f"{len(PROFILE_INDICES)} profile indices synchronized."
    )


if __name__ == "__main__":
    main()
