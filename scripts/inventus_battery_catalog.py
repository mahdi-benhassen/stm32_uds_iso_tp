"""Inventus battery test-profile Object Dictionary catalog.

The populated application rows are transcribed from the issue workbook
CANopen.cmd.xlsx (issue #10 attachment). The three identity strings are kept
in the same reviewable CSV source with empty defaults until the product owner
approves the actual device, hardware, and software version values. This is a
test-only catalog: application values remain raw wire-width values until
signed semantics, ranges, persistence, and runtime behavior are approved.
"""

from __future__ import annotations

import csv
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "product" / "inventus_battery_od.csv"
EXTENSION_SOURCE = ROOT / "product" / "inventus_battery_application_od.csv"
D000_SOURCE = ROOT / "product" / "inventus_battery_d000.csv"


def _identifier(index: int, name: str) -> str:
    stem = re.sub(r"[^A-Za-z0-9]+", "_", name).strip("_").lower()
    return f"inventus_{index:04x}_{stem}"


def _parse_default(value: str, *, preserve_empty: bool = False) -> str:
    value = value.strip()
    return value if value or preserve_empty else "0"


SCALARS = []
RECORDS = []
ARRAYS = []
SOURCE_ROWS = []
APPLICATION_SOURCE_ROWS = []
D000_SOURCE_ROWS = []
D000_FIELDS = []
with D000_SOURCE.open(newline="", encoding="utf-8") as stream:
    for row in csv.DictReader(stream):
        sub_index = int(row["sub_index"], 0)
        field = row["field"].strip()
        ctype = row["ctype"].strip()
        eds_type = int(row["eds_type"], 0)
        access = row["access"].strip()
        default = _parse_default(row["default"])
        width = int(row["bytes"], 10)
        if width not in (1, 2, 4):
            raise ValueError(f"invalid D000 width at sub-index {sub_index:#04x}")
        if access not in {"ro", "rw"}:
            raise ValueError(f"invalid D000 access at sub-index {sub_index:#04x}")
        if sub_index == 0 and (field != "highestSub_indexSupported" or default != "0x70"):
            raise ValueError("D000:00 must resolve the highest supported sub-index to 0x70")
        D000_SOURCE_ROWS.append(dict(row))
        D000_FIELDS.append((sub_index, field, ctype, eds_type, access, default))

if len(D000_FIELDS) != 81 or max(sub_index for sub_index, *_ in D000_FIELDS) != 0x70:
    raise ValueError("D000 source must contain 81 fields with maximum sub-index 0x70")
IDENTITY_INDICES = (0x1008, 0x1009, 0x100A)
with SOURCE.open(newline="", encoding="utf-8") as stream:
    for row in csv.DictReader(stream):
        index = int(row["index"], 0)
        name = row["name"].strip()
        access = row["access"].strip()
        kind = row.get("kind", "application").strip() or "application"
        width = int(row["bytes"], 10)
        default = _parse_default(row["default"], preserve_empty=(kind == "identity"))
        unit = row["unit"].strip()
        ident = _identifier(index, name)
        SOURCE_ROWS.append((index, name, access, width, unit, default, ident, kind))
        if kind == "identity":
            ctype = row["ctype"].strip()
            eds_type = int(row["eds_type"], 0)
            if not ctype.startswith("char[") or eds_type != 0x0009:
                raise ValueError(f"invalid visible-string identity metadata for {index:#06x}")
            SCALARS.append((index, ident, ctype, eds_type, access, default, 0))
            continue
        if kind == "diagnostic_array":
            ctype = row["ctype"].strip()
            eds_type = int(row["eds_type"], 0)
            count = int(row.get("count", ""), 0)
            if ctype != "uint8_t" or eds_type != 0x0005 or width != 1:
                raise ValueError(f"invalid raw diagnostic-array metadata for {index:#06x}")
            if access not in {"ro", "rw"} or count != 0xFE:
                raise ValueError(f"invalid bounded diagnostic-array metadata for {index:#06x}")
            ARRAYS.append((index, ident, ctype, eds_type, access, count, []))
            continue
        if kind != "application":
            raise ValueError(f"unsupported catalog row kind {kind!r} for {index:#06x}")
        APPLICATION_SOURCE_ROWS.append((index, name, access, width, unit, default, ident, kind))
        if width not in (1, 2):
            raise ValueError(f"unsupported width for {index:#06x}: {width}")
        ctype = "uint8_t" if width == 1 else "uint16_t"
        eds_type = 0x0005 if width == 1 else 0x0006
        if access == "array":
            # CANopen sub-index is uint8_t; sub-index 0 is the count, so the
            # largest representable data range is 1..0xFE. The workbook's
            # 0x00..0xFF notation is retained in the source CSV and called
            # out as a limitation in the test-profile documentation.
            ARRAYS.append((index, ident, ctype, eds_type, "ro", 0xFE, []))
        else:
            SCALARS.append((index, ident, ctype, eds_type, access, default, 0))

EXTENSION_SCALARS = []
EXTENSION_RECORDS = []
EXTENSION_SOURCE_ROWS = []
_extension_fields = {}
with EXTENSION_SOURCE.open(newline="", encoding="utf-8") as stream:
    for row in csv.DictReader(stream):
        index = int(row["index"], 0)
        sub_index = int(row["sub_index"], 0)
        name = row["name"].strip()
        access = row["access"].strip()
        width = int(row["bytes"], 10)
        ctype = row["ctype"].strip()
        eds_type = int(row["eds_type"], 0)
        default = _parse_default(row["default"])
        kind = row["kind"].strip()
        if access not in {"ro", "rw"} or width not in (1, 2, 4):
            raise ValueError(f"invalid Issue #12 metadata for {index:#06x}:{sub_index:#04x}")
        if kind == "scalar":
            if sub_index != 0:
                raise ValueError(f"scalar Issue #12 object has nonzero sub-index {index:#06x}:{sub_index:#04x}")
            ident = _identifier(index, name)
            EXTENSION_SCALARS.append((index, ident, ctype, eds_type, access, default, 0))
        elif kind == "record":
            _extension_fields.setdefault(index, []).append((sub_index, name, ctype, eds_type, access, default))
        else:
            raise ValueError(f"unsupported Issue #12 catalog kind {kind!r}")
        EXTENSION_SOURCE_ROWS.append(dict(row))

for index, fields in sorted(_extension_fields.items()):
    ident = _identifier(index, next(field[1] for field in fields if field[0] == 0))
    EXTENSION_RECORDS.append((index, ident, sorted(fields, key=lambda field: field[0])))
SCALARS.extend(EXTENSION_SCALARS)
SOURCE_ROWS.extend((int(row["index"], 0), row["name"].strip(), row["access"].strip(),
                    int(row["bytes"], 10), row["unit"].strip(), _parse_default(row["default"]),
                    _identifier(int(row["index"], 0), row["name"].strip()), "extension")
                   for row in EXTENSION_SOURCE_ROWS)

# The requested TPDO5/TPDO6 communication records are emitted with disabled
# COB-IDs and event-driven transmission defaults. These are test-safe defaults,
# not product configuration or node-ID policy.
def _tpdo_communication_record(number: int):
    return (
        0x1800 + number - 1,
        f"TPDOCommunicationParameter{number}",
        [
            (1, "COB_IDUsedByTPDO", "uint32_t", 0x0007, "rw", "0xC0000000"),
            (2, "transmissionType", "uint8_t", 0x0005, "rw", "0xFE"),
            (3, "inhibitTime", "uint16_t", 0x0006, "rw", "0"),
            (5, "eventTimer", "uint16_t", 0x0006, "rw", "0"),
            (6, "SYNCStartValue", "uint8_t", 0x0005, "rw", "0"),
        ],
    )


def _tpdo_mapping_record(index: int, values: list[int]):
    fields = [(0, "numberOfMappedApplicationObjectsInPDO", "uint8_t", 0x0005, "rw", str(len(values)))]
    fields.extend((position, f"applicationObject{position}", "uint32_t", 0x0007, "rw", f"0x{value:08X}")
                  for position, value in enumerate(values, start=1))
    fields.extend((position, f"applicationObject{position}", "uint32_t", 0x0007, "rw", "0")
                  for position in range(len(values) + 1, 9))
    return index, f"TPDOMappingParameter{index - 0x1A00 + 1}", fields


# D000 is a sparse typed diagnostic record from the workbook-derived catalog.
# The source row records the workbook's 0x29 value, while the generated test
# personality resolves sub-index 0 to 0x70 because entries are defined through 0x70.
D000_RECORD = (0xD000, "inventus_d000_internal_test_commands", D000_FIELDS)

# The workbook maps these application objects into six TPDOs. The mappings are
# retained as checked-in source metadata and are applied to 0x1A00..0x1A05.
PDO_MAPPINGS = {
    0x1A00: [0x48500008, 0x48510008, 0x48520010, 0x48530010, 0x48540010],
    0x1A01: [0x48550010, 0x48560010, 0x48570010, 0x48580008, 0x48590008],
    0x1A02: [0x485A0010, 0x485B0010, 0x485C0010, 0x485D0010],
    0x1A03: [0x485E0008, 0x485F0008, 0x48600008, 0x48610008, 0x48620010, 0x48630010],
    0x1A04: [0x48640010, 0x48650010, 0x48660010, 0x48670010],
    0x1A05: [0x48680010, 0x48690008, 0x486A0010, 0x486B0010, 0x486C0008],
}

# The pinned DS301 template already supplies 0x1800..0x1803 and 0x1A00..0x1A03.
# The Inventus test profile adds TPDO5/TPDO6 communication and mapping records.
RECORDS = EXTENSION_RECORDS + [D000_RECORD, _tpdo_communication_record(5), _tpdo_communication_record(6)]
RECORDS.extend(_tpdo_mapping_record(index, values)
               for index, values in PDO_MAPPINGS.items() if index >= 0x1A04)

for mapping in PDO_MAPPINGS.values():
    for value in mapping:
        index = (value >> 16) & 0xFFFF
        for item_position, item in enumerate(SCALARS):
            if item[0] == index:
                SCALARS[item_position] = (*item[:6], 1)
                break

APPLICATION_INDICES = sorted({row[0] for row in APPLICATION_SOURCE_ROWS})
EXTENSION_INDICES = sorted({index for index, *_ in EXTENSION_SCALARS}
                           | {index for index, *_ in EXTENSION_RECORDS})
REQUESTED_INDICES = sorted(set(APPLICATION_INDICES) | set(EXTENSION_INDICES))
DIAGNOSTIC_INDICES = (0xD000, 0xD001)
DIAGNOSTIC_RECORD_INDICES = (0xD000,)
DIAGNOSTIC_ARRAY_INDICES = (0xD001,)
EXPECTED_APPLICATION_OBJECT_COUNT = 60
EXPECTED_EXTENSION_OBJECT_COUNT = 11
EXPECTED_TOTAL_APPLICATION_OBJECT_COUNT = EXPECTED_APPLICATION_OBJECT_COUNT + EXPECTED_EXTENSION_OBJECT_COUNT
EXPECTED_DIAGNOSTIC_RECORD_COUNT = 1
EXPECTED_DIAGNOSTIC_ARRAY_COUNT = 1
D000_RESOLVED_HIGHEST_SUBINDEX = 0x70
D000_WORKBOOK_HIGHEST_SUBINDEX = 0x29
assert IDENTITY_INDICES == (0x1008, 0x1009, 0x100A)
assert len(APPLICATION_INDICES) == EXPECTED_APPLICATION_OBJECT_COUNT
assert len(EXTENSION_INDICES) == EXPECTED_EXTENSION_OBJECT_COUNT
assert len(REQUESTED_INDICES) == EXPECTED_TOTAL_APPLICATION_OBJECT_COUNT
assert len({index for index, *_ in APPLICATION_SOURCE_ROWS}) == EXPECTED_APPLICATION_OBJECT_COUNT
assert all(0x4800 <= index <= 0x4921 for index in APPLICATION_INDICES)
assert all(0x6000 <= index <= 0x6081 for index in EXTENSION_INDICES)
assert DIAGNOSTIC_INDICES == (0xD000, 0xD001)
assert DIAGNOSTIC_RECORD_INDICES == (0xD000,)
assert DIAGNOSTIC_ARRAY_INDICES == (0xD001,)
assert len(DIAGNOSTIC_RECORD_INDICES) == EXPECTED_DIAGNOSTIC_RECORD_COUNT
assert len(DIAGNOSTIC_ARRAY_INDICES) == EXPECTED_DIAGNOSTIC_ARRAY_COUNT
assert D000_FIELDS[0][0] == 0 and D000_FIELDS[0][5] == "0x70"
assert max(sub_index for sub_index, *_ in D000_FIELDS) == D000_RESOLVED_HIGHEST_SUBINDEX
