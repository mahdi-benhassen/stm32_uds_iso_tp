#!/usr/bin/env python3
"""Validate the authoritative CiA 401 OD/PDO product contract.

The checked-in manifest is JSON-compatible YAML so this validation remains
available on minimal CI images without adding a YAML dependency. The checker
fails closed on drift between the manifest, EDS, generated CANopenNode C/H
artifacts, and the selected firmware personality.
"""
from __future__ import annotations

import argparse
import configparser
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "product" / "cia401_od.yaml"


def fail(message: str) -> None:
    print(f"CiA 401 product validation failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def load_manifest(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        fail(f"cannot parse JSON-compatible YAML manifest {path}: {exc}")


def load_eds(path: Path) -> tuple[configparser.ConfigParser, str]:
    parser = configparser.ConfigParser(interpolation=None, strict=True)
    parser.optionxform = str
    try:
        text = path.read_text(encoding="utf-8")
        parser.read_string(text)
    except (OSError, configparser.Error) as exc:
        fail(f"cannot parse EDS {path}: {exc}")
    return parser, text


def raw_section(text: str, section: str) -> dict[str, str]:
    match = re.search(rf"(?ms)^\[{re.escape(section)}\]\n(.*?)(?=^\[|\Z)", text)
    if match is None:
        fail(f"EDS lacks [{section}]")
    values: dict[str, str] = {}
    for line in match.group(1).splitlines():
        line = line.strip()
        if line.startswith(";StorageLocation="):
            values["StorageLocation"] = line.split("=", 1)[1].strip()
        elif line and not line.startswith(";") and "=" in line:
            key, value = line.split("=", 1)
            values[key] = value.strip()
    return values


def require_equal(actual: str | None, expected: object, label: str) -> None:
    if actual is None:
        fail(f"{label} is absent")
    if isinstance(expected, bool):
        expected_text = "1" if expected else "0"
        if actual != expected_text:
            fail(f"{label} is {actual!r}, expected {expected!r}")
        return
    if isinstance(expected, int):
        try:
            if int(actual, 0) != expected:
                fail(f"{label} is {actual!r}, expected {expected!r}")
        except ValueError:
            fail(f"{label} is not numeric: {actual!r}")
        return
    if actual != str(expected):
        fail(f"{label} is {actual!r}, expected {expected!r}")


def validate_application_objects(manifest: dict, eds: configparser.ConfigParser, eds_text: str, od_c: str, od_h: str) -> None:
    objects = manifest.get("application_objects")
    if not isinstance(objects, list) or not objects:
        fail("application_objects must be a non-empty list")
    for item in objects:
        index_text = item["index"]
        index = int(index_text, 0)
        section = f"{index:04X}"
        fields = raw_section(eds_text, section)
        for key in ("DataType", "AccessType", "DefaultValue", "PDOMapping", "StorageLocation"):
            expected = item[{
                "DataType": "data_type",
                "AccessType": "access",
                "DefaultValue": "default",
                "PDOMapping": "pdo_mapping",
                "StorageLocation": "storage",
            }[key]]
            if key == "PDOMapping":
                expected = "1" if expected else "0"
            require_equal(fields.get(key), expected, f"EDS [{section}] {key}")

        ident = item["od_ident"]
        object_pattern = rf"(?ms)\.o_{index:04X}_{re.escape(ident)}\s*=\s*\{{(.*?)\n\s*\}}\s*,?"
        object_match = re.search(object_pattern, od_c)
        if object_match is None:
            fail(f"Generated/OD.c lacks object definition o_{index:04X}_{ident}")
        body = object_match.group(1)
        expected_sdo = "ODA_SDO_R" if item["access"] == "ro" else "ODA_SDO_RW"
        expected_pdo = "ODA_TPDO" if item["pdo_direction"] == "tpdo" else "ODA_RPDO"
        if expected_sdo not in body:
            fail(f"Generated/OD.c {index_text} lacks {expected_sdo}")
        if expected_pdo not in body:
            fail(f"Generated/OD.c {index_text} lacks {expected_pdo}")
        if f"dataLength = {item['data_length']}" not in body:
            fail(f"Generated/OD.c {index_text} dataLength does not match manifest")
        shortcut = f"OD_ENTRY_H{index:04X}_{ident}"
        if shortcut not in od_h:
            fail(f"Generated/OD.h lacks {shortcut}")


def validate_pdo_records(manifest: dict, eds: configparser.ConfigParser, eds_text: str) -> None:
    pdo = manifest.get("pdo", {})
    for direction in ("rpdo", "tpdo"):
        records = pdo.get(direction)
        if not isinstance(records, list) or len(records) != 4:
            fail(f"pdo.{direction} must contain exactly four records")
        for record in records:
            comm = int(record["communication_index"], 0)
            mapping = int(record["mapping_index"], 0)
            comm_fields = {key: raw_section(eds_text, f"{comm:04X}sub{sub}") for sub, key in ((1, "cob"), (2, "transmission"), (5, "event"))}
            require_equal(comm_fields["cob"].get("DefaultValue"), record["cob_id_default"], f"EDS 0x{comm:04X}:01 DefaultValue")
            require_equal(comm_fields["transmission"].get("DefaultValue"), record["transmission_type_default"], f"EDS 0x{comm:04X}:02 DefaultValue")
            require_equal(comm_fields["event"].get("DefaultValue"), record["event_timer_default"], f"EDS 0x{comm:04X}:05 DefaultValue")
            if record.get("inhibit_time_default") is not None:
                inhibit_fields = raw_section(eds_text, f"{comm:04X}sub3")
                require_equal(inhibit_fields.get("DefaultValue"), record["inhibit_time_default"], f"EDS 0x{comm:04X}:03 DefaultValue")
            if direction == "tpdo":
                sync_fields = raw_section(eds_text, f"{comm:04X}sub6")
                require_equal(sync_fields.get("DefaultValue"), record["sync_start_default"], f"EDS 0x{comm:04X}:06 DefaultValue")
            for sub, expected in enumerate(record["mapping_default"]):
                fields = raw_section(eds_text, f"{mapping:04X}sub{sub}")
                require_equal(fields.get("DefaultValue"), expected, f"EDS 0x{mapping:04X}:{sub:02X} DefaultValue")


def main() -> None:
    argument_parser = argparse.ArgumentParser()
    argument_parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    args = argument_parser.parse_args()
    manifest = load_manifest(args.manifest)
    if manifest.get("personality") != "cia401":
        fail("manifest personality is not cia401")
    if manifest.get("schema") != "stm32-canopen-cia401-product-v1":
        fail("unsupported manifest schema")
    paths = manifest["artifact_paths"]
    eds_path = ROOT / paths["eds"]
    od_c_path = ROOT / paths["od_c"]
    od_h_path = ROOT / paths["od_h"]
    config_path = ROOT / paths["firmware_config"]
    for path in (eds_path, od_c_path, od_h_path, config_path):
        if not path.is_file():
            fail(f"required artifact is missing: {path}")
    eds, eds_text = load_eds(eds_path)
    od_c = od_c_path.read_text(encoding="utf-8")
    od_h = od_h_path.read_text(encoding="utf-8")
    config = config_path.read_text(encoding="utf-8")
    if not re.search(r"#define\s+CANOPEN_REFERENCE_ENABLE_CIA401\s+1U", config):
        fail("firmware configuration does not select CiA 401")
    if re.search(r"#define\s+CANOPEN_REFERENCE_ENABLE_CIA418\s+1U", config):
        fail("firmware configuration unexpectedly selects CiA 418")
    validate_application_objects(manifest, eds, eds_text, od_c, od_h)
    validate_pdo_records(manifest, eds, eds_text)
    print("CiA 401 product validation passed: manifest, EDS, generated OD, PDO defaults, and firmware personality are synchronized.")


if __name__ == "__main__":
    main()
