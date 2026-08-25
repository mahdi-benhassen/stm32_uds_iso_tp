#!/usr/bin/env python3
"""Derive the reference CANopenNode V4 Object Dictionary from its upstream base.

This script is deliberately deterministic.  A released product shall retain the
corresponding CANopenEditor project and regenerate the OD after each profile or
PDO-map change; this reference demonstrates the generated C layout only.
"""
from pathlib import Path
import re
import shutil

from cia402_catalog import ARRAYS, RECORDS, REQUESTED_INDICES, SCALARS


def _data_length(ctype):
    match = re.fullmatch(r"char\[(\d+)\]", ctype)
    if match:
        return int(match.group(1))
    return {"int8_t": 1, "uint8_t": 1, "int16_t": 2, "uint16_t": 2,
            "int32_t": 4, "uint32_t": 4}[ctype]


def _c_declaration(ctype, name, indent=""):
    match = re.fullmatch(r"char\[(\d+)\]", ctype)
    if match:
        return f"{indent}char {name}[{match.group(1)}];"
    return f"{indent}{ctype} {name};"


def _attribute(access, length, string=False):
    attribute = "ODA_SDO_R" if access == "ro" else "ODA_SDO_RW"
    if length > 1 and not string:
        attribute += " | ODA_MB"
    if string:
        attribute += " | ODA_STR"
    return attribute


ROOT = Path(__file__).resolve().parents[1]
UPSTREAM = ROOT / "third_party" / "CanOpenSTM32" / "CANopenNode" / "example"
OUTPUT = ROOT / "Generated"
OUTPUT.mkdir(exist_ok=True)

shutil.copy2(UPSTREAM / "OD.h", OUTPUT / "OD.h")
shutil.copy2(UPSTREAM / "OD.c", OUTPUT / "OD.c")

header = (OUTPUT / "OD.h").read_text()
source = (OUTPUT / "OD.c").read_text()

def _catalog_member_declarations():
    lines = []
    for index, ident, ctype, *_ in SCALARS:
        lines.append(_c_declaration(ctype, f"x{index:04X}_{ident}", "    "))
    for index, ident, fields in RECORDS:
        lines.append("    struct {")
        lines.append("        uint8_t highestSub_indexSupported;")
        for _sub, field, ctype, *_ in fields:
            lines.append(_c_declaration(ctype, field, "        "))
        lines.append(f"    }} x{index:04X}_{ident};")
    for index, ident, ctype, _eds_type, _access, count, _fields in ARRAYS:
        lines.append(f"    {ctype} x{index:04X}_{ident}[{count + 1}];")
    return "\n".join(lines)


app_type = f'''typedef struct {{
    /* CiA 401 reference process data. */
    uint8_t x6000_readDigitalInputs;
    uint8_t x6200_writeDigitalOutputs;
    int16_t x6401_readAnalogInput1;
    int16_t x6411_readAnalogInput2;
    int16_t x6422_writeAnalogOutput1;

    /* CiA 402 reference process data. */
    uint16_t x603F_errorCode;
    uint16_t x6040_controlword;
    uint16_t x6041_statusword;
    int8_t x6060_modesOfOperation;
    int8_t x6061_modesOfOperationDisplay;
    int32_t x6064_positionActualValue;
    int32_t x607A_targetPosition;
    int16_t x6071_targetTorque;
    int16_t x6077_torqueActualValue;
    int32_t x606C_velocityActualValue;
    int32_t x60FF_targetVelocity;

    /* CiA 402 catalog extension. */
{_catalog_member_declarations()}
}} OD_APP_t;

#ifndef OD_ATTR_APP
#define OD_ATTR_APP
#endif
extern OD_ATTR_APP OD_APP_t OD_APP;

'''
needle = "} OD_RAM_t;\n\n#ifndef OD_ATTR_PERSIST_COMM"
if needle not in header:
    raise RuntimeError("Unexpected upstream OD.h layout while inserting OD_APP_t")
header = header.replace(needle, "} OD_RAM_t;\n\n" + app_type + "#ifndef OD_ATTR_PERSIST_COMM", 1)

base_application = [
    (0x6000, "readDigitalInputs"), (0x603F, "errorCode"),
    (0x6040, "controlword"), (0x6041, "statusword"),
    (0x6060, "modesOfOperation"), (0x6061, "modesOfOperationDisplay"),
    (0x6064, "positionActualValue"), (0x606C, "velocityActualValue"),
    (0x6071, "targetTorque"), (0x6077, "torqueActualValue"),
    (0x607A, "targetPosition"), (0x60FF, "targetVelocity"),
    (0x6200, "writeDigitalOutputs"), (0x6401, "readAnalogInput1"),
    (0x6411, "readAnalogInput2"), (0x6422, "writeAnalogOutput1"),
]
application_names = dict(base_application)
application_names.update((index, ident) for index, ident, *_ in SCALARS)
application_names.update((index, ident) for index, ident, *_ in RECORDS)
application_names.update((index, ident) for index, ident, *_ in ARRAYS)
base_odlist = source.split("static OD_ATTR_OD OD_entry_t ODList[] = {", 1)[1].split("};", 1)[0]
base_count = len(re.findall(r"^\s+\{0x", base_odlist, re.MULTILINE))
shortcut_macros = "\n".join(
    f"#define OD_ENTRY_H{index:04X}_{ident} &OD->list[{base_count + position}]"
    for position, (index, ident) in enumerate(sorted(application_names.items()))
) + "\n"
needle = "#define OD_ENTRY_H1A03_TPDOMappingParameter &OD->list[32]\n"
if needle not in header:
    raise RuntimeError("Unexpected upstream OD.h list shortcut layout")
header = header.replace(needle, needle + "\n" + shortcut_macros, 1)

source_app_init = r'''
OD_ATTR_APP OD_APP_t OD_APP = {
    .x6000_readDigitalInputs = 0x00,
    .x6200_writeDigitalOutputs = 0x00,
    .x6401_readAnalogInput1 = 0,
    .x6411_readAnalogInput2 = 0,
    .x6422_writeAnalogOutput1 = 0,
    .x603F_errorCode = 0x0000,
    .x6040_controlword = 0x0000,
    .x6041_statusword = 0x0040, /* Switch-on disabled. */
    .x6060_modesOfOperation = 0,
    .x6061_modesOfOperationDisplay = 0,
    .x6064_positionActualValue = 0,
    .x607A_targetPosition = 0,
    .x6071_targetTorque = 0,
    .x6077_torqueActualValue = 0,
    .x606C_velocityActualValue = 0,
    .x60FF_targetVelocity = 0
};

'''
catalog_initializers = []
for index, ident, ctype, _eds_type, _access, default, _pdo in SCALARS:
    value = "{0}" if ctype.startswith("char[") else default
    catalog_initializers.append(f"    .x{index:04X}_{ident} = {value},")
for index, ident, fields in RECORDS:
    catalog_initializers.append(f"    .x{index:04X}_{ident} = {{ .highestSub_indexSupported = {len(fields)},")
    for _sub, field, _ctype, _eds_type, _access, default in fields:
        catalog_initializers.append(f"        .{field} = {default},")
    catalog_initializers[-1] = catalog_initializers[-1].rstrip(",")
    catalog_initializers.append("    },")
for index, ident, _ctype, _eds_type, _access, count, _fields in ARRAYS:
    catalog_initializers.append(f"    .x{index:04X}_{ident} = {{ {count}, 0, 0 }},")
source_app_init = source_app_init.replace("    .x60FF_targetVelocity = 0\n};", "    .x60FF_targetVelocity = 0,\n" + "\n".join(catalog_initializers) + "\n};")

needle = "};\n\n\n\n/*******************************************************************************\n    All OD objects (constant definitions)"
if needle not in source:
    raise RuntimeError("Unexpected upstream OD.c initialization layout")
source = source.replace(needle, "};\n\n" + source_app_init + "/*******************************************************************************\n    All OD objects (constant definitions)", 1)

object_members = r'''    OD_obj_var_t o_6000_readDigitalInputs;
    OD_obj_var_t o_6200_writeDigitalOutputs;
    OD_obj_var_t o_6401_readAnalogInput1;
    OD_obj_var_t o_6411_readAnalogInput2;
    OD_obj_var_t o_6422_writeAnalogOutput1;
    OD_obj_var_t o_603F_errorCode;
    OD_obj_var_t o_6040_controlword;
    OD_obj_var_t o_6041_statusword;
    OD_obj_var_t o_6060_modesOfOperation;
    OD_obj_var_t o_6061_modesOfOperationDisplay;
    OD_obj_var_t o_6064_positionActualValue;
    OD_obj_var_t o_6071_targetTorque;
    OD_obj_var_t o_6077_torqueActualValue;
    OD_obj_var_t o_607A_targetPosition;
    OD_obj_var_t o_606C_velocityActualValue;
    OD_obj_var_t o_60FF_targetVelocity;
'''
catalog_members = []
catalog_members.extend(f"    OD_obj_var_t o_{index:04X}_{ident};" for index, ident, *_ in SCALARS)
catalog_members.extend(f"    OD_obj_record_t o_{index:04X}_{ident}[{len(fields) + 1}];" for index, ident, fields in RECORDS)
catalog_members.extend(f"    OD_obj_array_t o_{index:04X}_{ident};" for index, ident, *_ in ARRAYS)
object_members = object_members.rstrip() + "\n" + "\n".join(catalog_members) + "\n"

needle = "    OD_obj_record_t o_1A03_TPDOMappingParameter[9];\n"
if needle not in source:
    raise RuntimeError("Unexpected upstream OD.c object member layout")
source = source.replace(needle, needle + object_members, 1)

object_definitions = r'''    .o_6000_readDigitalInputs = {
        .dataOrig = &OD_APP.x6000_readDigitalInputs,
        .attribute = ODA_SDO_R | ODA_TPDO,
        .dataLength = 1
    },
    .o_6200_writeDigitalOutputs = {
        .dataOrig = &OD_APP.x6200_writeDigitalOutputs,
        .attribute = ODA_SDO_RW | ODA_RPDO,
        .dataLength = 1
    },
    .o_6401_readAnalogInput1 = {
        .dataOrig = &OD_APP.x6401_readAnalogInput1,
        .attribute = ODA_SDO_R | ODA_TPDO,
        .dataLength = 2
    },
    .o_6411_readAnalogInput2 = {
        .dataOrig = &OD_APP.x6411_readAnalogInput2,
        .attribute = ODA_SDO_R | ODA_TPDO,
        .dataLength = 2
    },
    .o_6422_writeAnalogOutput1 = {
        .dataOrig = &OD_APP.x6422_writeAnalogOutput1,
        .attribute = ODA_SDO_RW | ODA_RPDO,
        .dataLength = 2
    },
    .o_603F_errorCode = {
        .dataOrig = &OD_APP.x603F_errorCode,
        .attribute = ODA_SDO_R | ODA_TPDO,
        .dataLength = 2
    },
    .o_6040_controlword = {
        .dataOrig = &OD_APP.x6040_controlword,
        .attribute = ODA_SDO_RW | ODA_RPDO,
        .dataLength = 2
    },
    .o_6041_statusword = {
        .dataOrig = &OD_APP.x6041_statusword,
        .attribute = ODA_SDO_R | ODA_TPDO,
        .dataLength = 2
    },
    .o_6060_modesOfOperation = {
        .dataOrig = &OD_APP.x6060_modesOfOperation,
        .attribute = ODA_SDO_RW | ODA_RPDO,
        .dataLength = 1
    },
    .o_6061_modesOfOperationDisplay = {
        .dataOrig = &OD_APP.x6061_modesOfOperationDisplay,
        .attribute = ODA_SDO_R | ODA_TPDO,
        .dataLength = 1
    },
    .o_6064_positionActualValue = {
        .dataOrig = &OD_APP.x6064_positionActualValue,
        .attribute = ODA_SDO_R | ODA_TPDO,
        .dataLength = 4
    },
    .o_6071_targetTorque = {
        .dataOrig = &OD_APP.x6071_targetTorque,
        .attribute = ODA_SDO_RW | ODA_RPDO,
        .dataLength = 2
    },
    .o_6077_torqueActualValue = {
        .dataOrig = &OD_APP.x6077_torqueActualValue,
        .attribute = ODA_SDO_R | ODA_TPDO,
        .dataLength = 2
    },
    .o_607A_targetPosition = {
        .dataOrig = &OD_APP.x607A_targetPosition,
        .attribute = ODA_SDO_RW | ODA_RPDO,
        .dataLength = 4
    },
    .o_606C_velocityActualValue = {
        .dataOrig = &OD_APP.x606C_velocityActualValue,
        .attribute = ODA_SDO_R | ODA_TPDO,
        .dataLength = 4
    },
    .o_60FF_targetVelocity = {
        .dataOrig = &OD_APP.x60FF_targetVelocity,
        .attribute = ODA_SDO_RW | ODA_RPDO,
        .dataLength = 4
    }
'''

def _dynamic_scalar_definition(index, ident, ctype, _eds_type, access, _default, _pdo):
    length = _data_length(ctype)
    string = ctype.startswith("char[")
    return f"""    .o_{index:04X}_{ident} = {{
        .dataOrig = &OD_APP.x{index:04X}_{ident},
        .attribute = {_attribute(access, length, string)},
        .dataLength = {length}
    }}"""


def _dynamic_record_definition(index, ident, fields):
    lines = [f"    .o_{index:04X}_{ident} = {{", "        {", "            .dataOrig = &OD_APP.x%04X_%s.highestSub_indexSupported," % (index, ident), "            .subIndex = 0,", "            .attribute = ODA_SDO_R,", "            .dataLength = 1", "        },"]
    for position, field, ctype, _eds_type, access, _default in fields:
        length = _data_length(ctype)
        lines.extend(["        {", f"            .dataOrig = &OD_APP.x{index:04X}_{ident}.{field},", f"            .subIndex = {position},", f"            .attribute = {_attribute(access, length)},", f"            .dataLength = {length}", "        },"])
    lines[-1] = lines[-1].rstrip(",")
    lines.append("    }")
    return "\n".join(lines)


def _dynamic_array_definition(index, ident, ctype, _eds_type, access, count, _fields):
    length = _data_length(ctype)
    return f"""    .o_{index:04X}_{ident} = {{
        .dataOrig0 = (uint8_t*) &OD_APP.x{index:04X}_{ident}[0],
        .dataOrig = &OD_APP.x{index:04X}_{ident}[1],
        .attribute0 = ODA_SDO_R,
        .attribute = {_attribute(access, length)},
        .dataElementLength = {length},
        .dataElementSizeof = sizeof({ctype})
    }}"""

catalog_definitions = []
catalog_definitions.extend(_dynamic_scalar_definition(*item) for item in SCALARS)
catalog_definitions.extend(_dynamic_record_definition(index, ident, fields) for index, ident, fields in RECORDS)
catalog_definitions.extend(_dynamic_array_definition(*item) for item in ARRAYS)
object_definitions = object_definitions.rstrip() + ",\n" + ",\n".join(catalog_definitions)

catalog_entries = []
catalog_entries.extend((index, 1, "ODT_VAR", f"&ODObjs.o_{index:04X}_{ident}") for index, ident, *_ in base_application)
catalog_entries.extend((index, 1, "ODT_VAR", f"&ODObjs.o_{index:04X}_{ident}") for index, ident, *_ in SCALARS)
catalog_entries.extend((index, len(fields) + 1, "ODT_REC", f"&ODObjs.o_{index:04X}_{ident}") for index, ident, fields in RECORDS)
catalog_entries.extend((index, count + 1, "ODT_ARR", f"&ODObjs.o_{index:04X}_{ident}") for index, ident, _ctype, _eds_type, _access, count, _fields in ARRAYS)
entries = "\n".join(f"    {{0x{index:04X}, 0x{sub_count:02X}, {object_type}, {object_ref}, NULL}}," for index, sub_count, object_type, object_ref in sorted(catalog_entries)) + "\n"

needle = "    }\n};\n\n\n/*******************************************************************************\n    Object dictionary\n"
if needle not in source:
    raise RuntimeError("Unexpected upstream OD.c object initializer layout")
source = source.replace(needle, "    },\n" + object_definitions + "};\n\n\n/*******************************************************************************\n    Object dictionary\n", 1)

needle = "    {0x0000, 0x00, 0, NULL, NULL}\n"
if needle not in source:
    raise RuntimeError("Unexpected upstream OD.c ODList terminator")
source = source.replace(needle, entries + needle, 1)

(OUTPUT / "OD.h").write_text(header)
(OUTPUT / "OD.c").write_text(source)

eds = (UPSTREAM / "DS301_profile.eds").read_text()
eds = eds.replace("FileName=DS301_profile.eds", "FileName=stm32f767_canopen_reference.eds")
eds = eds.replace("ProductName=New Product", "ProductName=STM32F767 CANopen Reference")
eds = eds.replace("DynamicChannelsSupported=0", "DynamicChannelsSupported=1")
optional_match = re.search(r"(?ms)^\[OptionalObjects\]\n.*?(?=^\[)", eds)
if optional_match is None:
    raise RuntimeError("Unexpected DS301 EDS OptionalObjects layout")
base_optional = [int(value, 0) for value in re.findall(r"^\d+=(0x[0-9A-Fa-f]+)$", optional_match.group(0), re.MULTILINE)]
application_indices = sorted(application_names)
all_optional = base_optional + [index for index in application_indices if index not in base_optional]
optional_lines = ["[OptionalObjects]", f"SupportedObjects={len(all_optional)}"]
optional_lines.extend(f"{position}=0x{index:04X}" for position, index in enumerate(all_optional, start=1))
eds = eds[:optional_match.start()] + "\n".join(optional_lines) + "\n" + eds[optional_match.end():]
profile_entries = r'''
[6000]
ParameterName=Read digital inputs
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0005
AccessType=ro
DefaultValue=0x00
PDOMapping=1

[603F]
ParameterName=Error code
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0006
AccessType=ro
DefaultValue=0x0000
PDOMapping=1

[6040]
ParameterName=Controlword
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0006
AccessType=rw
DefaultValue=0x0000
PDOMapping=1

[6041]
ParameterName=Statusword
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0006
AccessType=ro
DefaultValue=0x0040
PDOMapping=1

[6060]
ParameterName=Modes of operation
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0002
AccessType=rw
DefaultValue=0
PDOMapping=1

[6061]
ParameterName=Modes of operation display
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0002
AccessType=ro
DefaultValue=0
PDOMapping=1

[6064]
ParameterName=Position actual value
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0004
AccessType=ro
DefaultValue=0
PDOMapping=1

[606C]
ParameterName=Velocity actual value
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0004
AccessType=ro
DefaultValue=0
PDOMapping=1

[6071]
ParameterName=Target torque
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0003
AccessType=rw
DefaultValue=0
PDOMapping=1

[6077]
ParameterName=Torque actual value
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0003
AccessType=ro
DefaultValue=0
PDOMapping=1

[607A]
ParameterName=Target position
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0004
AccessType=rw
DefaultValue=0
PDOMapping=1

[60FF]
ParameterName=Target velocity
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0004
AccessType=rw
DefaultValue=0
PDOMapping=1

[6200]
ParameterName=Write digital outputs
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0005
AccessType=rw
DefaultValue=0x00
PDOMapping=1

[6401]
ParameterName=Read analog input 1
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0003
AccessType=ro
DefaultValue=0
PDOMapping=1

[6411]
ParameterName=Read analog input 2
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0003
AccessType=ro
DefaultValue=0
PDOMapping=1

[6422]
ParameterName=Write analog output 1
ObjectType=0x7
;StorageLocation=RAM
DataType=0x0003
AccessType=rw
DefaultValue=0
PDOMapping=1
    '''

def _eds_scalar_section(index, ident, eds_type, access, default, pdo):
    name = re.sub(r"(?<!^)(?=[A-Z])", " ", ident).capitalize()
    default_value = default if default else ""
    return f"[{index:04X}]\nParameterName={name}\nObjectType=0x7\n;StorageLocation=RAM\nDataType={eds_type}\nAccessType={access}\nDefaultValue={default_value}\nPDOMapping={int(pdo)}\n"


def _eds_record_sections(index, ident, fields):
    name = re.sub(r"(?<!^)(?=[A-Z])", " ", ident).capitalize()
    sections = [f"[{index:04X}]", f"ParameterName={name}", "ObjectType=0x9", f"SubNumber=0x{len(fields) + 1:02X}", ""]
    sections.extend([f"[{index:04X}sub0]", f"ParameterName=Number of entries", "ObjectType=0x7", "DataType=0x0005", "AccessType=ro", f"DefaultValue={len(fields)}", "PDOMapping=0", ""])
    for sub, field, _ctype, eds_type, access, default in fields:
        field_name = re.sub(r"(?<!^)(?=[A-Z])", " ", field).capitalize()
        sections.extend([f"[{index:04X}sub{sub}]", f"ParameterName={field_name}", "ObjectType=0x7", f"DataType={eds_type}", f"AccessType={access}", f"DefaultValue={default}", "PDOMapping=0", ""])
    return "\n".join(sections)


def _eds_array_sections(index, ident, eds_type, access, count, fields):
    name = re.sub(r"(?<!^)(?=[A-Z])", " ", ident).capitalize()
    sections = [f"[{index:04X}]", f"ParameterName={name}", "ObjectType=0x8", f"SubNumber=0x{count + 1:02X}", f"DataType={eds_type}", f"AccessType={access}", ""]
    sections.extend([f"[{index:04X}sub0]", "ParameterName=Number of entries", "ObjectType=0x7", "DataType=0x0005", "AccessType=ro", f"DefaultValue={count}", "PDOMapping=0", ""])
    for sub, field, default in fields:
        field_name = re.sub(r"(?<!^)(?=[A-Z])", " ", field).capitalize()
        sections.extend([f"[{index:04X}sub{sub}]", f"ParameterName={field_name}", "ObjectType=0x7", f"DataType={eds_type}", f"AccessType={access}", f"DefaultValue={default}", "PDOMapping=0", ""])
    return "\n".join(sections)

catalog_eds_sections = []
catalog_eds_sections.extend(_eds_scalar_section(index, ident, eds_type, access, default, pdo) for index, ident, _ctype, eds_type, access, default, pdo in SCALARS)
catalog_eds_sections.extend(_eds_record_sections(index, ident, fields) for index, ident, fields in RECORDS)
catalog_eds_sections.extend(_eds_array_sections(index, ident, eds_type, access, count, fields) for index, ident, _ctype, eds_type, access, count, fields in ARRAYS)
profile_entries += "\n" + "\n".join(catalog_eds_sections)

eds_output = "\n".join(line.rstrip() for line in (eds + "\n" + profile_entries).splitlines()) + "\n"
(ROOT / "ObjectDictionary" / "stm32f767_canopen_reference.eds").write_text(eds_output)
print("Generated/OD.h, Generated/OD.c, and ObjectDictionary/stm32f767_canopen_reference.eds updated.")
