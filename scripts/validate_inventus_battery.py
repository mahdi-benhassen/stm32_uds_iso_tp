"""Validate the isolated Inventus battery test-profile artifacts."""

from __future__ import annotations

import re
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from inventus_battery_catalog import (  # noqa: E402
    D000_FIELDS,
    D000_RESOLVED_HIGHEST_SUBINDEX,
    D000_SOURCE,
    D000_WORKBOOK_HIGHEST_SUBINDEX,
    DIAGNOSTIC_ARRAY_INDICES,
    DIAGNOSTIC_INDICES,
    DIAGNOSTIC_RECORD_INDICES,
    EXPECTED_APPLICATION_OBJECT_COUNT,
    EXPECTED_EXTENSION_OBJECT_COUNT,
    EXPECTED_TOTAL_APPLICATION_OBJECT_COUNT,
    EXTENSION_INDICES,
    EXTENSION_SOURCE,
    EXPECTED_DIAGNOSTIC_ARRAY_COUNT,
    EXPECTED_DIAGNOSTIC_RECORD_COUNT,
    IDENTITY_INDICES,
    PDO_MAPPINGS,
    RECORDS,
    REQUESTED_INDICES,
    SOURCE,
)


def fail(message: str) -> None:
    raise SystemExit(f"inventus validation failed: {message}")


def main() -> None:
    if len(REQUESTED_INDICES) != EXPECTED_TOTAL_APPLICATION_OBJECT_COUNT:
        fail("catalog does not contain the 60 core plus 11 Issue #12 application indices")
    if len(EXTENSION_INDICES) != EXPECTED_EXTENSION_OBJECT_COUNT:
        fail("catalog does not contain exactly 11 Issue #12 extension indices")
    if len(set(REQUESTED_INDICES)) != len(REQUESTED_INDICES):
        fail("catalog contains duplicate application indices")
    if not all((0x4800 <= index <= 0x4921) or (0x6000 <= index <= 0x6081) for index in REQUESTED_INDICES):
        fail("application index is outside the core or Issue #12 Inventus ranges")
    if not SOURCE.exists():
        fail(f"source catalog is missing: {SOURCE}")
    if not D000_SOURCE.exists():
        fail(f"D000 source catalog is missing: {D000_SOURCE}")

    header = (ROOT / "Generated" / "inventus_battery_OD.h").read_text(encoding="utf-8")
    source = (ROOT / "Generated" / "inventus_battery_OD.c").read_text(encoding="utf-8")
    eds = (ROOT / "ObjectDictionary" / "stm32f767_inventus_battery_test.eds").read_text(encoding="utf-8")

    for index in REQUESTED_INDICES:
        token = f"x{index:04X}_"
        if token not in header or f"{{0x{index:04X}," not in source:
            fail(f"generated OD is missing application object 0x{index:04X}")
        if f"[{index:04X}]" not in eds:
            fail(f"generated EDS is missing application object 0x{index:04X}")

    for index, length in ((0x1008, 32), (0x1009, 16), (0x100A, 16)):
        if index not in IDENTITY_INDICES:
            fail(f"identity catalog is missing 0x{index:04X}")
        if f"{{0x{index:04X}, 0x01, ODT_VAR" not in source:
            fail(f"generated OD is missing identity object 0x{index:04X}")
        if f"[{index:04X}]" not in eds or f"DataType=9" not in eds.split(f"[{index:04X}]", 1)[1].split("[", 1)[0]:
            fail(f"generated EDS is missing VISIBLE_STRING identity object 0x{index:04X}")
        if f"dataLength = {length}" not in source:
            fail(f"generated OD identity length is wrong for 0x{index:04X}")

    expected_records = {0x6020, 0x6030, 0xD000, 0x1804, 0x1805, 0x1A04, 0x1A05}
    if {index for index, *_ in RECORDS} != expected_records:
        fail("Inventus catalog record set is not D000 plus TPDO5/TPDO6 communication and mapping records")
    for index in sorted(expected_records):
        if f"{{0x{index:04X}," not in source or f"[{index:04X}]" not in eds:
            fail(f"generated OD/EDS is missing record 0x{index:04X}")
    if ".COB_IDUsedByTPDO = 0xC0000000" not in source:
        fail("TPDO5/TPDO6 communication defaults are not disabled")
    for reserved_index in (0x1804, 0x1805):
        record_section = eds.split(f"[{reserved_index:04X}]", 1)[1].split("[", 1)[0]
        if f"[{reserved_index:04X}sub4]" in record_section:
            fail(f"reserved TPDO sub-index 4 was incorrectly emitted for 0x{reserved_index:04X}")

    if DIAGNOSTIC_INDICES != (0xD000, 0xD001):
        fail("catalog must contain exactly diagnostic objects 0xD000 and 0xD001")
    if DIAGNOSTIC_RECORD_INDICES != (0xD000,) or len(DIAGNOSTIC_RECORD_INDICES) != EXPECTED_DIAGNOSTIC_RECORD_COUNT:
        fail("catalog must contain exactly one structured D000 diagnostic record")
    if DIAGNOSTIC_ARRAY_INDICES != (0xD001,) or len(DIAGNOSTIC_ARRAY_INDICES) != EXPECTED_DIAGNOSTIC_ARRAY_COUNT:
        fail("catalog must contain exactly one bounded D001 diagnostic array")

    d000_max = max(sub_index for sub_index, *_ in D000_FIELDS)
    if len(D000_FIELDS) != 81 or d000_max != D000_RESOLVED_HIGHEST_SUBINDEX:
        fail("D000 catalog must contain 81 fields through resolved sub-index 0x70")
    if D000_WORKBOOK_HIGHEST_SUBINDEX == D000_RESOLVED_HIGHEST_SUBINDEX:
        fail("D000 workbook discrepancy marker unexpectedly disappeared")
    if f"{{0xD000, 0x{d000_max + 1:02X}, ODT_REC" not in source:
        fail("generated OD is missing structured D000 record")
    d000_section = eds.split("[D000]", 1)[1].split("[D001]", 1)[0]
    if "ObjectType=0x9" not in d000_section or f"SubNumber=0x{d000_max + 1:02X}" not in d000_section:
        fail("generated EDS has incorrect structured D000 header")
    if "[D000sub0]" not in d000_section or "DataType=5" not in d000_section or "DefaultValue=0x70" not in d000_section:
        fail("D000:00 does not expose resolved highest sub-index 0x70")
    for sub_index, _field, _ctype, eds_type, access, default in D000_FIELDS:
        section_name = f"[D000sub{sub_index}]"
        if section_name not in d000_section:
            fail(f"generated EDS is missing D000 sub-index {sub_index:#04x}")
        section = d000_section.split(section_name, 1)[1].split("[", 1)[0]
        if f"DataType={eds_type}" not in section or f"AccessType={access}" not in section:
            fail(f"D000 sub-index {sub_index:#04x} has incorrect EDS type/access")
        if f"DefaultValue={default}" not in section:
            fail(f"D000 sub-index {sub_index:#04x} has incorrect EDS default")
    for gap in (0x19, 0x1A, 0x25, 0x26, 0x27, 0x29, 0x41, 0x4F):
        if f"[D000sub{gap}]" in d000_section:
            fail(f"undefined D000 gap {gap:#04x} was emitted")

    if f"{{0xD001, 0xFF, ODT_ARR" not in source:
        fail("generated OD is missing bounded raw D001 diagnostic array")
    d001_section = eds.split("[D001]", 1)[1]
    d001_header = d001_section.split("[", 1)[0]
    if "SubNumber=0xFF" not in d001_header or "AccessType=rw" not in d001_header:
        fail("generated EDS has incorrect bounded D001 metadata")
    if "DataType=0x0005" not in d001_header and "DataType=5" not in d001_header:
        fail("generated EDS D001 metadata is not raw-byte typed")
    if "[D001subFF]" in d001_section:
        fail("diagnostic array D001 incorrectly emits unsupported sub-index 255")
    if "diagnostic_array" not in SOURCE.read_text(encoding="utf-8") or "workbook" not in D000_SOURCE.read_text(encoding="utf-8").lower():
        fail("source catalogs do not disclose diagnostic provenance")
    if not EXTENSION_SOURCE.exists() or "Battery status" not in EXTENSION_SOURCE.read_text(encoding="utf-8"):
        fail("Issue #12 extension source catalog is missing or incomplete")
    for index in (0x6000, 0x6001, 0x6010, 0x6050, 0x6051, 0x6052, 0x6060, 0x6070, 0x6081):
        if f"[{index:04X}]" not in eds:
            fail(f"generated EDS is missing Issue #12 scalar 0x{index:04X}")
    for index, highest in ((0x6020, 4), (0x6030, 2)):
        section = eds.split(f"[{index:04X}]", 1)[1].split("[", 1)[0]
        if "ObjectType=0x9" not in section or f"SubNumber=0x{highest + 1:02X}" not in section:
            fail(f"Issue #12 record 0x{index:04X} has incorrect sub-index count")
    if "DefaultValue=320" not in eds.split("[6070]", 1)[1].split("[", 1)[0]:
        fail("Issue #12 charge-current default 320 is missing")

    for mapping_index, mappings in PDO_MAPPINGS.items():
        if len(mappings) == 0 or sum(value & 0xFF for value in mappings) > 64:
            fail(f"invalid PDO mapping definition 0x{mapping_index:04X}")
        for value in mappings:
            app_index = (value >> 16) & 0xFFFF
            if app_index not in REQUESTED_INDICES:
                fail(f"PDO mapping 0x{mapping_index:04X} references absent object 0x{app_index:04X}")

    optional_section = eds.split("[OptionalObjects]", 1)[-1].split("[", 1)[0]
    supported_match = re.search(r"^SupportedObjects=(\d+)$", optional_section, re.MULTILINE)
    if supported_match is None or int(supported_match.group(1)) < EXPECTED_TOTAL_APPLICATION_OBJECT_COUNT:
        fail("EDS OptionalObjects SupportedObjects is smaller than the core plus Issue #12 application objects")

    print(f"inventus battery validation: PASS ({EXPECTED_TOTAL_APPLICATION_OBJECT_COUNT} application objects, structured D000 with {len(D000_FIELDS)} fields, bounded D001 array, {len(PDO_MAPPINGS)} PDO maps, {len(IDENTITY_INDICES)} identity objects)")


if __name__ == "__main__":
    main()
