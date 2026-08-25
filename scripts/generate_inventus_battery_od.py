"""Generate the isolated Inventus battery test-profile OD and EDS.

This generator reuses the pinned CANopenNode DS301 example template and the
repository's deterministic OD rendering helpers. It emits separate profile
artifacts and never modifies the default or generic CiA 418 artifacts.
"""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))

import generate_cia418_od as common  # noqa: E402
from inventus_battery_catalog import ARRAYS, PDO_MAPPINGS, RECORDS, SCALARS  # noqa: E402

common.SCALARS = SCALARS
common.RECORDS = RECORDS
common.ARRAYS = ARRAYS
common.APPLICATION_NAMES = {index: ident for index, ident, *_ in SCALARS}
common.APPLICATION_NAMES.update({index: ident for index, ident, _fields in RECORDS})
common.APPLICATION_NAMES.update({index: ident for index, ident, *_ in ARRAYS})

GENERATED = ROOT / "Generated"
OD_DIR = ROOT / "ObjectDictionary"


def _replace_profile_names(text: str) -> str:
    return (text.replace("CIA418_OD_H", "INVENTUS_BATTERY_OD_H")
                .replace("CIA418_OD", "INVENTUS_BATTERY_OD")
                .replace("cia418_OD.h", "inventus_battery_OD.h")
                .replace("stm32f767_cia418_reference.eds", "stm32f767_inventus_battery_test.eds")
                .replace("STM32F767 CiA 418 Reference", "STM32F767 Inventus Battery Test Profile"))


def _replace_mapping_block(source: str, index: int, values: list[int]) -> str:
    padded = values + [0] * (8 - len(values))
    ident = "TPDOMappingParameter" if index <= 0x1A03 else f"TPDOMappingParameter{index - 0x1A00 + 1}"
    block = (f"    .x{index:04X}_{ident} = {{\n"
             f"        .numberOfMappedApplicationObjectsInPDO = 0x{len(values):02X},\n" +
             "\n".join(f"        .applicationObject{position} = 0x{value:08X},"
                           for position, value in enumerate(padded, start=1)) +
             "\n    }")
    source, replaced = re.subn(
        rf"    \.x{index:04X}_{ident} = \{{.*?\n    \}}",
        block,
        source,
        count=1,
        flags=re.DOTALL,
    )
    if replaced != 1:
        raise RuntimeError(f"Unable to set Inventus TPDO mapping {index:#06x}")
    return source


def generate_header() -> str:
    return _replace_profile_names(common.generate_header())


def generate_source() -> str:
    source = _replace_profile_names(common.generate_source())
    for index, values in PDO_MAPPINGS.items():
        source = _replace_mapping_block(source, index, values)
    return source


def generate_eds() -> str:
    return _replace_profile_names(common.generate_eds())


def main() -> None:
    GENERATED.mkdir(exist_ok=True)
    OD_DIR.mkdir(exist_ok=True)
    (GENERATED / "inventus_battery_OD.h").write_text(generate_header(), encoding="utf-8")
    (GENERATED / "inventus_battery_OD.c").write_text(generate_source(), encoding="utf-8")
    (OD_DIR / "stm32f767_inventus_battery_test.eds").write_text(generate_eds(), encoding="utf-8")
    print("Generated Inventus battery test-profile OD header, source, and EDS.")


if __name__ == "__main__":
    main()
