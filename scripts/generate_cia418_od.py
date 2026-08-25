"""Generate the independent CANopenNode-native CiA 418 Object Dictionary.

The generator starts from the pinned CANopenNode DS301 example OD and injects
only the checked-in CiA 418 catalog.  The generated OD is a real ``OD_t``
with the same ``OD_find``/SDO surface as the default personality; it is linked
only by the CiA 418 CMake personality.
"""
from pathlib import Path
import re
import shutil
import sys

from cia418_catalog import ARRAYS, RECORDS, SCALARS

ROOT = Path(__file__).resolve().parents[1]
UPSTREAM = ROOT / "third_party" / "CanOpenSTM32" / "CANopenNode" / "example"
GENERATED = ROOT / "Generated"
OD_DIR = ROOT / "ObjectDictionary"

BASE_APPLICATION = []
APPLICATION_NAMES = {
    index: ident
    for index, ident, *_ in SCALARS + [(i, n, f) for i, n, f in RECORDS]
}
APPLICATION_NAMES.update({index: ident for index, ident, *_ in ARRAYS})


def data_length(ctype: str) -> int:
    match = re.fullmatch(r"char\[(\d+)\]", ctype)
    if match:
        return int(match.group(1))
    return {"int8_t": 1, "uint8_t": 1, "int16_t": 2, "uint16_t": 2,
            "int32_t": 4, "uint32_t": 4}[ctype]


def c_declaration(ctype: str, name: str, indent: str = "") -> str:
    match = re.fullmatch(r"char\[(\d+)\]", ctype)
    if match:
        return f"{indent}char {name}[{match.group(1)}];"
    return f"{indent}{ctype} {name};"


def od_attribute(access: str, length: int, pdo: bool = False, string: bool = False) -> str:
    attribute = "ODA_SDO_R" if access == "ro" else "ODA_SDO_RW"
    if length > 1 and not string:
        attribute += " | ODA_MB"
    if string:
        attribute += " | ODA_STR"
    if pdo:
        attribute += " | ODA_TPDO"
    return attribute


def catalog_member_declarations() -> str:
    lines = []
    for index, ident, ctype, *_ in SCALARS:
        lines.append(c_declaration(ctype, f"x{index:04X}_{ident}", "    "))
    for index, ident, fields in RECORDS:
        lines.append("    struct {")
        if not any(sub_index == 0 for sub_index, *_ in fields):
            lines.append("        uint8_t highestSub_indexSupported;")
        for _sub, field, ctype, *_ in fields:
            lines.append(c_declaration(ctype, field, "        "))
        lines.append(f"    }} x{index:04X}_{ident};")
    for index, ident, ctype, _eds, _access, count, _fields in ARRAYS:
        lines.append(f"    {ctype} x{index:04X}_{ident}[{count + 1}];")
    return "\n".join(lines)


def app_type() -> str:
    return f'''typedef struct {{
    /* CiA 418 application data from the checked-in catalog. */
{catalog_member_declarations()}
}} OD_APP_t;

#ifndef OD_ATTR_APP
#define OD_ATTR_APP
#endif
extern OD_ATTR_APP OD_APP_t OD_APP;

'''


def app_initializer() -> str:
    lines = ["OD_ATTR_APP OD_APP_t OD_APP = {"]
    for index, ident, _ctype, _eds, _access, default, _pdo in SCALARS:
        value = "{0}" if _ctype.startswith("char[") else (default or "0")
        lines.append(f"    .x{index:04X}_{ident} = {value},")
    for index, ident, fields in RECORDS:
        max_sub_index = max((sub_index for sub_index, *_ in fields), default=0)
        explicit_sub0 = any(sub_index == 0 for sub_index, *_ in fields)
        if explicit_sub0:
            lines.append(f"    .x{index:04X}_{ident} = {{")
        else:
            lines.append(f"    .x{index:04X}_{ident} = {{ .highestSub_indexSupported = {max_sub_index},")
        for _sub, field, _ctype, _eds, _access, default in fields:
            lines.append(f"        .{field} = {default or '0'},")
        lines[-1] = lines[-1].rstrip(",")
        lines.append("    },")
    for index, ident, _ctype, _eds, _access, count, _fields in ARRAYS:
        lines.append(f"    .x{index:04X}_{ident} = {{ {count}, 0 }},")
    lines.append("};\n\n")
    return "\n".join(lines)


def object_member_declarations() -> str:
    lines = []
    for index, ident, *_ in SCALARS:
        lines.append(f"    OD_obj_var_t o_{index:04X}_{ident};")
    for index, ident, fields in RECORDS:
        max_sub_index = max((sub_index for sub_index, *_ in fields), default=0)
        lines.append(f"    OD_obj_record_t o_{index:04X}_{ident}[{max_sub_index + 1}];")
    for index, ident, *_ in ARRAYS:
        lines.append(f"    OD_obj_array_t o_{index:04X}_{ident};")
    return "\n".join(lines) + "\n"


def scalar_definition(index, ident, ctype, _eds, access, _default, pdo) -> str:
    length = data_length(ctype)
    string = ctype.startswith("char[")
    return f'''    .o_{index:04X}_{ident} = {{
        .dataOrig = &OD_APP.x{index:04X}_{ident},
        .attribute = {od_attribute(access, length, bool(pdo), string)},
        .dataLength = {length}
    }}'''


def record_definition(index, ident, fields) -> str:
    explicit_sub0 = next((field_data for field_data in fields if field_data[0] == 0), None)
    if explicit_sub0 is None:
        sub0 = (f"            .dataOrig = &OD_APP.x{index:04X}_{ident}.highestSub_indexSupported,\n"
                "            .subIndex = 0,\n"
                "            .attribute = ODA_SDO_R,\n"
                "            .dataLength = 1\n")
    else:
        _position, field, ctype, _eds, access, _default = explicit_sub0
        sub0 = (f"            .dataOrig = &OD_APP.x{index:04X}_{ident}.{field},\n"
                "            .subIndex = 0,\n"
                f"            .attribute = {od_attribute(access, data_length(ctype))},\n"
                f"            .dataLength = {data_length(ctype)}\n")
    lines = [
        f"    .o_{index:04X}_{ident} = {{",
        "        {",
        sub0.rstrip("\n"),
        "        },",
    ]
    for position, field, ctype, _eds, access, _default in fields:
        if position == 0:
            continue
        lines.extend([
            "        {",
            f"            .dataOrig = &OD_APP.x{index:04X}_{ident}.{field},",
            f"            .subIndex = {position},",
            f"            .attribute = {od_attribute(access, data_length(ctype))},",
            f"            .dataLength = {data_length(ctype)}",
            "        },",
        ])
    lines[-1] = lines[-1].rstrip(",")
    lines.append("    }")
    return "\n".join(lines)


def array_definition(index, ident, ctype, _eds, access, _count, _fields) -> str:
    length = data_length(ctype)
    return f'''    .o_{index:04X}_{ident} = {{
        .dataOrig0 = (uint8_t*) &OD_APP.x{index:04X}_{ident}[0],
        .dataOrig = &OD_APP.x{index:04X}_{ident}[1],
        .attribute0 = ODA_SDO_R,
        .attribute = {od_attribute(access, length)},
        .dataElementLength = {length},
        .dataElementSizeof = sizeof({ctype})
    }}'''


def generate_header() -> str:
    header = (UPSTREAM / "OD.h").read_text()
    header = header.replace("#ifndef OD_H\n#define OD_H", "#ifndef CIA418_OD_H\n#define CIA418_OD_H\n\n#include \"CO_ODinterface.h\"", 1)
    needle = "} OD_RAM_t;\n\n#ifndef OD_ATTR_PERSIST_COMM"
    if needle not in header:
        raise RuntimeError("Unexpected upstream OD.h layout")
    header = header.replace(needle, "} OD_RAM_t;\n\n" + app_type() + "#ifndef OD_ATTR_PERSIST_COMM", 1)

    base_source = (UPSTREAM / "OD.c").read_text()
    base_odlist = base_source.split("static OD_ATTR_OD OD_entry_t ODList[] = {", 1)[1].split("};", 1)[0]
    low_index_profile = any(index < 0x2000 for index in APPLICATION_NAMES)
    shortcuts = []
    if low_index_profile:
        base_indices = [int(value, 16) for value in re.findall(r"^[ \t]+\{0x([0-9A-Fa-f]+),", base_odlist, re.MULTILINE) if int(value, 16) != 0]
        all_indices = sorted(set(base_indices) | set(APPLICATION_NAMES))
        list_positions = {index: position for position, index in enumerate(all_indices)}
        # Low-index profiles can be inserted before the upstream OD entries,
        # so rewrite inherited shortcuts for the rebuilt sorted ODList.
        def rewrite_shortcut(match):
            prefix, index_text, suffix = match.groups()
            return f"{prefix}{list_positions[int(index_text, 16)]}{suffix}"
        header = re.sub(
            r"(#define OD_ENTRY_H([0-9A-Fa-f]{4})(?:_[A-Za-z0-9_]+)? &OD->list\[)\d+(\])",
            rewrite_shortcut,
            header,
        )
        for index, ident in sorted(APPLICATION_NAMES.items()):
            position = list_positions[index]
            shortcuts.append(f"#define OD_ENTRY_H{index:04X} &OD->list[{position}]")
            shortcuts.append(f"#define OD_ENTRY_H{index:04X}_{ident} &OD->list[{position}]")
    else:
        base_count = len(re.findall(r"^[ \t]+\{0x", base_odlist, re.MULTILINE))
        for position, (index, ident) in enumerate(sorted(APPLICATION_NAMES.items())):
            shortcuts.append(f"#define OD_ENTRY_H{index:04X} &OD->list[{base_count + position}]")
            shortcuts.append(f"#define OD_ENTRY_H{index:04X}_{ident} &OD->list[{base_count + position}]")
    marker_match = re.search(r"#define OD_ENTRY_H1A03_TPDOMappingParameter &OD->list\[\d+\]\n", header)
    if marker_match is None:
        raise RuntimeError("Unexpected upstream OD.h shortcut layout")
    marker = marker_match.group(0)
    header = header.replace(marker, marker + "\n" + "\n".join(shortcuts) + "\n", 1)
    prefix, suffix = header.rsplit("#endif", 1)
    rendered = prefix + "#endif /* CIA418_OD_H */" + suffix
    return "\n".join(line.rstrip() for line in rendered.splitlines()) + "\n"


def generate_source() -> str:
    source = (UPSTREAM / "OD.c").read_text()
    base_odlist = source.split("static OD_ATTR_OD OD_entry_t ODList[] = {", 1)[1].split("};", 1)[0]
    source = source.replace('#include "OD.h"', '#include "cia418_OD.h"', 1)
    app_marker = "\n\n/*******************************************************************************\n    All OD objects (constant definitions)"
    if app_marker not in source:
        raise RuntimeError("Unexpected upstream OD.c application insertion point")
    source = source.replace(app_marker, "\n\n" + app_initializer() + "/*******************************************************************************\n    All OD objects (constant definitions)", 1)

    member_marker = "    OD_obj_record_t o_1A03_TPDOMappingParameter[9];\n} ODObjs_t;"
    if member_marker not in source:
        raise RuntimeError("Unexpected upstream OD.c object-member layout")
    source = source.replace(member_marker, "    OD_obj_record_t o_1A03_TPDOMappingParameter[9];\n" + object_member_declarations() + "} ODObjs_t;", 1)

    definitions = []
    definitions.extend(scalar_definition(*item) for item in SCALARS)
    definitions.extend(record_definition(index, ident, fields) for index, ident, fields in RECORDS)
    definitions.extend(array_definition(*item) for item in ARRAYS)
    definition_text = ",\n" + ",\n".join(definitions)
    object_marker = "    .o_1A03_TPDOMappingParameter = {"
    object_start = source.find(object_marker)
    if object_start < 0:
        raise RuntimeError("Unexpected upstream OD.c object initializer")
    object_end = source.find("    }\n};", object_start)
    if object_end < 0:
        raise RuntimeError("Unexpected upstream OD.c object initializer end")
    source = source[:object_end] + "    }" + definition_text + "\n" + source[object_end + len("    }\n"):]

    scalar_by_index = {item[0]: item for item in SCALARS}
    record_by_index = {item[0]: item for item in RECORDS}
    array_by_index = {item[0]: item for item in ARRAYS}
    application_entries = {}
    for index in sorted(APPLICATION_NAMES):
        if index in scalar_by_index:
            _index, ident, *_ = scalar_by_index[index]
            application_entries[index] = f"    {{0x{index:04X}, 0x01, ODT_VAR, &ODObjs.o_{index:04X}_{ident}, NULL}},"
        elif index in record_by_index:
            _index, ident, fields = record_by_index[index]
            max_sub_index = max((sub_index for sub_index, *_ in fields), default=0)
            application_entries[index] = f"    {{0x{index:04X}, 0x{max_sub_index + 1:02X}, ODT_REC, &ODObjs.o_{index:04X}_{ident}, NULL}},"
        else:
            _index, ident, _ctype, _eds, _access, count, _fields = array_by_index[index]
            application_entries[index] = f"    {{0x{index:04X}, 0x{count + 1:02X}, ODT_ARR, &ODObjs.o_{index:04X}_{ident}, NULL}},"
    low_index_profile = any(index < 0x2000 for index in APPLICATION_NAMES)
    if low_index_profile:
        base_entries = {
            int(value, 16): line.rstrip()
            for line, value in re.findall(r"^([ \t]+\{0x([0-9A-Fa-f]+),.*)$", base_odlist, re.MULTILINE)
        }
        base_entries.pop(0, None)
        if set(base_entries) & set(application_entries):
            raise RuntimeError("Application OD index overlaps upstream OD index")
        entries = [application_entries[index] if index in application_entries else base_entries[index]
                   for index in sorted(set(base_entries) | set(application_entries))]
        list_start = source.find("static OD_ATTR_OD OD_entry_t ODList[] = {")
        list_end = source.find("};", list_start)
        if list_start < 0 or list_end < 0:
            raise RuntimeError("Unexpected upstream OD.c ODList layout")
        list_prefix = "static OD_ATTR_OD OD_entry_t ODList[] = {\n\n"
        source = source[:list_start] + list_prefix + "\n".join(entries) + "\n    {0x0000, 0x00, 0, NULL, NULL}\n};" + source[list_end + 2:]
    else:
        entry_marker = "    {0x0000, 0x00, 0, NULL, NULL}\n"
        if entry_marker not in source:
            raise RuntimeError("Unexpected upstream OD.c ODList terminator")
        source = source.replace(entry_marker, "\n".join(application_entries[index] for index in sorted(application_entries)) + "\n" + entry_marker, 1)

    pdo_mappings = {
        0x1A00: [0x60600120, 0x60100110, 0x60810108],
        0x1A01: [0x60700110],
    }
    for index, mapping in pdo_mappings.items():
        values = mapping + [0] * (8 - len(mapping))
        block = (f"    .x{index:04X}_TPDOMappingParameter = {{\n"
                 f"        .numberOfMappedApplicationObjectsInPDO = 0x{len(mapping):02X},\n" +
                 "\n".join(f"        .applicationObject{position} = 0x{value:08X},"
                           for position, value in enumerate(values, start=1)) +
                 "\n    }")
        source, replaced = re.subn(
            rf"    \.x{index:04X}_TPDOMappingParameter = \{{.*?\n    \}}",
            block,
            source,
            count=1,
            flags=re.DOTALL,
        )
        if replaced != 1:
            raise RuntimeError(f"Unable to set TPDO mapping 0x{index:04X}")
    return source


def eds_scalar(index, ident, eds_type, access, default, pdo):
    name = re.sub(r"(?<!^)(?=[A-Z])", " ", ident).capitalize()
    return (f"[{index:04X}]\nParameterName={name}\nObjectType=0x7\n"
            f"DataType={eds_type}\nAccessType={access}\nDefaultValue={default or ''}\n"
            f"PDOMapping={int(pdo)}\n")


def eds_record(index, ident, fields):
    name = re.sub(r"(?<!^)(?=[A-Z])", " ", ident).capitalize()
    max_sub_index = max((sub_index for sub_index, *_ in fields), default=0)
    lines = [f"[{index:04X}]", f"ParameterName={name}", "ObjectType=0x9",
             f"SubNumber=0x{max_sub_index + 1:02X}", ""]
    explicit_sub0 = next((field_data for field_data in fields if field_data[0] == 0), None)
    if explicit_sub0 is None:
        sub0_name = "Highest sub-index supported" if index in (0x1804, 0x1805) else "Number of entries"
        lines += [f"[{index:04X}sub0]", f"ParameterName={sub0_name}", "ObjectType=0x7",
                  "DataType=0x0005", "AccessType=ro", f"DefaultValue={max_sub_index}", "PDOMapping=0", ""]
    else:
        _sub, field, _ctype, eds_type, access, default = explicit_sub0
        field_name = re.sub(r"(?<!^)(?=[A-Z])", " ", field).capitalize()
        lines += [f"[{index:04X}sub0]", f"ParameterName={field_name}", "ObjectType=0x7",
                  f"DataType={eds_type}", f"AccessType={access}", f"DefaultValue={default or ''}", "PDOMapping=0", ""]
    for sub, field, _ctype, eds_type, access, default in fields:
        if sub == 0:
            continue
        field_name = re.sub(r"(?<!^)(?=[A-Z])", " ", field).capitalize()
        lines += [f"[{index:04X}sub{sub}]", f"ParameterName={field_name}", "ObjectType=0x7",
                  f"DataType={eds_type}", f"AccessType={access}", f"DefaultValue={default or ''}", "PDOMapping=0", ""]
    return "\n".join(lines)


def eds_array(index, ident, eds_type, access, count):
    name = re.sub(r"(?<!^)(?=[A-Z])", " ", ident).capitalize()
    lines = [f"[{index:04X}]", f"ParameterName={name}", "ObjectType=0x8",
             f"SubNumber=0x{count + 1:02X}", f"DataType={eds_type}", f"AccessType={access}", "",
             f"[{index:04X}sub0]", "ParameterName=Number of entries", "ObjectType=0x7",
             "DataType=0x0005", "AccessType=ro", f"DefaultValue={count}", "PDOMapping=0", ""]
    for sub in range(1, count + 1):
        lines += [f"[{index:04X}sub{sub}]", f"ParameterName=Element {sub}", "ObjectType=0x7",
                  f"DataType={eds_type}", f"AccessType={access}", "DefaultValue=0", "PDOMapping=0", ""]
    return "\n".join(lines)


def generate_eds() -> str:
    eds = (UPSTREAM / "DS301_profile.eds").read_text()
    eds = eds.replace("FileName=DS301_profile.eds", "FileName=stm32f767_cia418_reference.eds")
    eds = eds.replace("ProductName=New Product", "ProductName=STM32F767 CiA 418 Reference")
    optional_match = re.search(r"(?ms)^\[OptionalObjects\]\n.*?(?=^\[)", eds)
    if optional_match is None:
        raise RuntimeError("Unexpected EDS OptionalObjects layout")
    base_optional = [int(value, 0) for value in re.findall(r"^\d+=(0x[0-9A-Fa-f]+)$", optional_match.group(0), re.MULTILINE)]
    app_indices = sorted(APPLICATION_NAMES)
    all_optional = base_optional + [index for index in app_indices if index not in base_optional]
    optional_lines = ["[OptionalObjects]", f"SupportedObjects={len(all_optional)}"]
    optional_lines.extend(f"{position}=0x{index:04X}" for position, index in enumerate(all_optional, start=1))
    eds = eds[:optional_match.start()] + "\n".join(optional_lines) + "\n" + eds[optional_match.end():]
    sections = []
    sections.extend(eds_scalar(index, ident, eds_type, access, default, pdo)
                    for index, ident, _ctype, eds_type, access, default, pdo in SCALARS)
    sections.extend(eds_record(index, ident, fields) for index, ident, fields in RECORDS)
    sections.extend(eds_array(index, ident, eds_type, access, count) for index, ident, _ctype, eds_type, access, count, _fields in ARRAYS)
    return "\n".join(line.rstrip() for line in (eds + "\n" + "\n".join(sections)).splitlines()) + "\n"


def main():
    GENERATED.mkdir(exist_ok=True)
    OD_DIR.mkdir(exist_ok=True)
    (GENERATED / "cia418_OD.h").write_text(generate_header())
    (GENERATED / "cia418_OD.c").write_text(generate_source())
    (OD_DIR / "stm32f767_cia418_reference.eds").write_text(generate_eds())
    print("Generated live CiA 418 OD header, source, and EDS.")


if __name__ == "__main__":
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    main()
