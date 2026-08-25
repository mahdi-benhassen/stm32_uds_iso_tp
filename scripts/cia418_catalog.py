"""Checked-in CiA 418 reference Object Dictionary catalog.

This is the generic reference personality, not a customer-specific Inventus
Power product dictionary.  Vendor-specific entries must be added only after
the issue attachment is transcribed into a reviewable text/CSV source and
approved by the product owner.
"""

# index, identifier, C type, CiA 306 data type, access, default, PDO mapping
SCALARS = [
    (0x6000, "batteryStatus", "uint8_t", 0x0005, "ro", "0x01", 1),
    (0x6001, "chargerStatus", "uint8_t", 0x0005, "rw", "0", 0),
    (0x6010, "temperature", "int16_t", 0x0003, "ro", "0", 1),
    (0x6050, "cumulativeTotalAhCharge", "uint32_t", 0x0007, "ro", "0", 0),
    (0x6051, "ahExpendedSinceLastCharge", "uint16_t", 0x0006, "ro", "0", 0),
    (0x6052, "ahReturnedDuringLastCharge", "uint16_t", 0x0006, "rw", "0", 0),
    (0x6053, "ahSinceLastEqualization", "uint16_t", 0x0006, "rw", "0", 0),
    (0x6060, "batteryVoltage", "uint32_t", 0x0007, "ro", "0", 1),
    (0x6070, "chargeCurrentRequested", "uint16_t", 0x0006, "ro", "0", 1),
    (0x6080, "chargerStateOfCharge", "uint8_t", 0x0005, "rw", "0", 0),
    (0x6081, "batteryStateOfCharge", "uint8_t", 0x0005, "ro", "0", 1),
    (0x6090, "waterLevelStatus", "uint8_t", 0x0005, "ro", "0", 0),
]

# index, identifier, [(sub-index, member, C type, CiA 306 type, access, default)]
RECORDS = [
    (0x6020, "batteryParameters", [
        (1, "batteryType", "uint8_t", 0x0005, "ro", "0"),
        (2, "ahCapacity", "uint16_t", 0x0006, "ro", "0"),
        (3, "maximumChargeCurrent", "uint16_t", 0x0006, "ro", "0"),
        (4, "numberOfCells", "uint16_t", 0x0006, "ro", "0"),
    ]),
]

# index, identifier, C element type, CiA 306 type, access, max count, fields
ARRAYS = [
    (0x6030, "batterySerialNumber", "uint32_t", 0x0007, "ro", 3, []),
    (0x6031, "batteryId", "uint32_t", 0x0007, "ro", 5, []),
    (0x6040, "vehicleSerialNumber", "uint32_t", 0x0007, "ro", 5, []),
    (0x6041, "vehicleId", "uint32_t", 0x0007, "ro", 5, []),
    (0x6054, "dateOfLastEqualization", "uint16_t", 0x0006, "rw", 2, []),
]

REQUESTED_INDICES = sorted(
    {index for index, *_ in SCALARS}
    | {index for index, *_ in RECORDS}
    | {index for index, *_ in ARRAYS}
)

MANDATORY_APPLICATION_INDICES = {0x6000, 0x6001, 0x6010, 0x6020}
