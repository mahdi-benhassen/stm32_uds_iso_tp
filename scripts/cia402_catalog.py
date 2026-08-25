"""Declarative CiA 402 extension catalog for the reference Object Dictionary.

The catalog intentionally exposes the standard object names and data widths while
keeping manufacturer-specific records bounded and explicit.  Runtime control-mode
semantics remain the responsibility of the profile adapter; adding an OD entry does
not claim that a complete motion-control loop is implemented.
"""

# index, C identifier suffix, C type, EDS data type, access, default, PDO mapping
# EDS data type codes follow CiA 306: 0x0002 INTEGER8, 0x0003 INTEGER16,
# 0x0004 INTEGER32, 0x0005 UNSIGNED8, 0x0006 UNSIGNED16, 0x0007 UNSIGNED32,
# 0x0009 VISIBLE_STRING, and 0x000C TIME_OF_DAY.
SCALARS = [
    (0x6007, "abortConnectionOptionCode", "int16_t", 0x0003, "rw", "0", 0),
    (0x6402, "motorType", "uint16_t", 0x0006, "rw", "0", 0),
    (0x6403, "motorCatalogNumber", "char[64]", 0x0009, "rw", "", 0),
    (0x6404, "motorManufacturer", "char[64]", 0x0009, "rw", "", 0),
    (0x6405, "motorCatalogAddress", "char[128]", 0x0009, "rw", "", 0),
    (0x6406, "motorCalibrationDate", "uint32_t", 0x000C, "rw", "0", 0),
    (0x6407, "motorServicePeriod", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6502, "supportedDriveModes", "uint32_t", 0x0007, "ro", "0", 0),
    (0x6503, "driveCatalogNumber", "char[64]", 0x0009, "rw", "", 0),
    (0x6504, "driveManufacturer", "char[64]", 0x0009, "rw", "", 0),
    (0x6505, "driveCatalogAddress", "char[128]", 0x0009, "rw", "", 0),
    (0x60FD, "digitalInputs", "uint32_t", 0x0007, "ro", "0", 0),
    (0x605B, "shutdownOptionCode", "int16_t", 0x0003, "rw", "0", 0),
    (0x605C, "disableOperationOptionCode", "int16_t", 0x0003, "rw", "0", 0),
    (0x605A, "quickStopOptionCode", "int16_t", 0x0003, "rw", "0", 0),
    (0x605D, "haltOptionCode", "int16_t", 0x0003, "rw", "0", 0),
    (0x605E, "faultReactionOptionCode", "int16_t", 0x0003, "rw", "0", 0),
    (0x6089, "positionNotationIndex", "int8_t", 0x0002, "rw", "0", 0),
    (0x608A, "positionDimensionIndex", "uint8_t", 0x0005, "rw", "0", 0),
    (0x608B, "velocityNotationIndex", "int8_t", 0x0002, "rw", "0", 0),
    (0x608C, "velocityDimensionIndex", "uint8_t", 0x0005, "rw", "0", 0),
    (0x608D, "accelerationNotationIndex", "int8_t", 0x0002, "rw", "0", 0),
    (0x608E, "accelerationDimensionIndex", "uint8_t", 0x0005, "rw", "0", 0),
    (0x607E, "polarity", "uint8_t", 0x0005, "rw", "0", 0),
    (0x607F, "maxProfileVelocity", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6080, "maxMotorSpeed", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6081, "profileVelocity", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6082, "endVelocity", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6083, "profileAcceleration", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6084, "profileDeceleration", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6085, "quickStopDeceleration", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6086, "motionProfileType", "int16_t", 0x0003, "rw", "0", 0),
    (0x60C5, "maxAcceleration", "uint32_t", 0x0007, "rw", "0", 0),
    (0x60C6, "maxDeceleration", "uint32_t", 0x0007, "rw", "0", 0),
    (0x607C, "homeOffset", "int32_t", 0x0004, "rw", "0", 0),
    (0x6098, "homingMethod", "int8_t", 0x0002, "rw", "0", 0),
    (0x609A, "homingAcceleration", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6062, "positionDemandValue", "int32_t", 0x0004, "ro", "0", 0),
    (0x6063, "positionActualValueEncoder", "int32_t", 0x0004, "ro", "0", 0),
    (0x6065, "followingErrorWindow", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6066, "followingErrorTimeout", "uint16_t", 0x0006, "rw", "0", 0),
    (0x6067, "positionWindow", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6068, "positionWindowTime", "uint16_t", 0x0006, "rw", "0", 0),
    (0x60F4, "followingErrorActualValue", "int32_t", 0x0004, "ro", "0", 0),
    (0x60FA, "positionControlEffort", "int16_t", 0x0003, "ro", "0", 0),
    (0x60FC, "positionDemandValueInternal", "int32_t", 0x0004, "ro", "0", 0),
    (0x60C0, "interpolationSubModeSelect", "int8_t", 0x0002, "rw", "0", 0),
    (0x6069, "velocitySensorActualValue", "int32_t", 0x0004, "ro", "0", 0),
    (0x606A, "sensorSelectionCode", "int16_t", 0x0003, "rw", "0", 0),
    (0x606B, "velocityDemandValue", "int32_t", 0x0004, "ro", "0", 0),
    (0x606D, "velocityWindow", "uint32_t", 0x0007, "rw", "0", 0),
    (0x606E, "velocityWindowTime", "uint16_t", 0x0006, "rw", "0", 0),
    (0x606F, "velocityThreshold", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6070, "velocityThresholdTime", "uint16_t", 0x0006, "rw", "0", 0),
    (0x60F8, "maxSlippage", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6072, "maxTorque", "uint16_t", 0x0006, "rw", "0", 0),
    (0x6073, "maxCurrent", "uint16_t", 0x0006, "rw", "0", 0),
    (0x6074, "torqueDemandValue", "int16_t", 0x0003, "ro", "0", 0),
    (0x6075, "motorRatedCurrent", "uint16_t", 0x0006, "rw", "0", 0),
    (0x6076, "motorRatedTorque", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6078, "currentActualValue", "int16_t", 0x0003, "ro", "0", 0),
    (0x6079, "dcLinkCircuitVoltage", "uint16_t", 0x0006, "ro", "0", 0),
    (0x6087, "torqueSlope", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6088, "torqueProfileType", "int16_t", 0x0003, "rw", "0", 0),
    (0x6042, "vlTargetVelocity", "int32_t", 0x0004, "rw", "0", 0),
    (0x6053, "vlPercentageDemand", "int16_t", 0x0003, "ro", "0", 0),
    (0x6054, "vlActualPercentage", "int16_t", 0x0003, "ro", "0", 0),
    (0x6055, "vlManipulatedPercentage", "int16_t", 0x0003, "ro", "0", 0),
    (0x604E, "vlVelocityReference", "int32_t", 0x0004, "rw", "0", 0),
    (0x604D, "vlPoleNumber", "uint16_t", 0x0006, "rw", "0", 0),
    (0x604F, "vlRampFunctionTime", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6050, "vlSlowDownTime", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6051, "vlQuickStopTime", "uint32_t", 0x0007, "rw", "0", 0),
    (0x6044, "vlControlEffort", "int16_t", 0x0003, "ro", "0", 0),
    (0x6045, "vlManipulatedVelocity", "int32_t", 0x0004, "ro", "0", 0),
    (0x6052, "vlNominalPercentage", "int16_t", 0x0003, "rw", "0", 0),
]

# index, identifier suffix, [(sub-index, member, C type, EDS type, access, default)]
# The first subindex is emitted as the CANopen record count.  Manufacturer-specific
# records intentionally expose one bounded placeholder field; products may extend
# them from their CANopenEditor/XDD source model.
RECORDS = [
    (0x6410, "motorData", [(1, "manufacturerData1", "uint32_t", 0x0007, "rw", "0")]),
    (0x6510, "driveData", [(1, "manufacturerData1", "uint32_t", 0x0007, "rw", "0")]),
    (0x608F, "positionEncoderResolution", [(1, "motorRevolutions", "uint32_t", 0x0007, "rw", "0"), (2, "encoderIncrements", "uint32_t", 0x0007, "rw", "0")]),
    (0x6090, "velocityEncoderResolution", [(1, "motorRevolutions", "uint32_t", 0x0007, "rw", "0"), (2, "encoderIncrements", "uint32_t", 0x0007, "rw", "0")]),
    (0x6091, "gearRatio", [(1, "motorRevolutions", "uint32_t", 0x0007, "rw", "0"), (2, "drivenRevolutions", "uint32_t", 0x0007, "rw", "0")]),
    (0x6092, "feedConstant", [(1, "feed", "uint32_t", 0x0007, "rw", "0"), (2, "shaftRevolutions", "uint32_t", 0x0007, "rw", "0")]),
    (0x6093, "positionFactor", [(1, "numerator", "uint32_t", 0x0007, "rw", "0"), (2, "divisor", "uint32_t", 0x0007, "rw", "0")]),
    (0x6094, "velocityEncoderFactor", [(1, "numerator", "uint32_t", 0x0007, "rw", "0"), (2, "divisor", "uint32_t", 0x0007, "rw", "0")]),
    (0x6095, "velocityFactor1", [(1, "numerator", "uint32_t", 0x0007, "rw", "0"), (2, "divisor", "uint32_t", 0x0007, "rw", "0")]),
    (0x6096, "velocityFactor2", [(1, "numerator", "uint32_t", 0x0007, "rw", "0"), (2, "divisor", "uint32_t", 0x0007, "rw", "0")]),
    (0x6097, "accelerationFactor", [(1, "numerator", "uint32_t", 0x0007, "rw", "0"), (2, "divisor", "uint32_t", 0x0007, "rw", "0")]),
    (0x607B, "positionRangeLimit", [(1, "minPosition", "int32_t", 0x0004, "rw", "0"), (2, "maxPosition", "int32_t", 0x0004, "rw", "0")]),
    (0x607D, "softwarePositionLimit", [(1, "minPosition", "int32_t", 0x0004, "rw", "0"), (2, "maxPosition", "int32_t", 0x0004, "rw", "0")]),
    (0x6099, "homingSpeeds", [(1, "speedSwitchSearch", "uint32_t", 0x0007, "rw", "0"), (2, "speedZeroSearch", "uint32_t", 0x0007, "rw", "0")]),
    (0x60FB, "positionControlParameterSet", [(1, "parameterSet1", "uint32_t", 0x0007, "rw", "0")]),
    (0x60C1, "interpolationDataRecord", [(1, "interpolationData1", "int32_t", 0x0004, "rw", "0")]),
    (0x60C2, "interpolationTimePeriod", [(1, "timeUnits", "uint8_t", 0x0005, "rw", "0"), (2, "timeIndex", "int8_t", 0x0002, "rw", "0")]),
    (0x60C3, "interpolationSyncDefinition", [(1, "syncType", "uint8_t", 0x0005, "rw", "0")]),
    (0x60C4, "interpolationDataConfiguration", [(1, "maxBufferSize", "uint8_t", 0x0005, "rw", "0"), (2, "actualBufferSize", "uint8_t", 0x0005, "rw", "0"), (3, "bufferOrganization", "uint8_t", 0x0005, "rw", "0"), (4, "bufferPosition", "uint8_t", 0x0005, "rw", "0")]),
    (0x60F9, "velocityControlParameterSet", [(1, "parameterSet1", "uint32_t", 0x0007, "rw", "0")]),
    (0x60F7, "powerStageParameters", [(1, "parameter1", "uint32_t", 0x0007, "rw", "0")]),
    (0x60F6, "torqueControlParameters", [(1, "parameter1", "uint32_t", 0x0007, "rw", "0")]),
    (0x604C, "vlDimensionFactor", [(1, "numerator", "uint32_t", 0x0007, "rw", "0"), (2, "divisor", "uint32_t", 0x0007, "rw", "0")]),
    (0x604B, "vlSetPointFactor", [(1, "numerator", "uint32_t", 0x0007, "rw", "0"), (2, "divisor", "uint32_t", 0x0007, "rw", "0")]),
    (0x6046, "vlVelocityMinMaxAmount", [(1, "minVelocity", "uint32_t", 0x0007, "rw", "0"), (2, "maxVelocity", "uint32_t", 0x0007, "rw", "0")]),
    (0x6047, "vlVelocityMinMax", [(1, "minVelocity", "uint32_t", 0x0007, "rw", "0"), (2, "maxVelocity", "uint32_t", 0x0007, "rw", "0"), (3, "minAmount", "uint32_t", 0x0007, "rw", "0"), (4, "maxAmount", "uint32_t", 0x0007, "rw", "0")]),
    (0x6058, "vlFrequencyMotorMinMaxAmount", [(1, "minFrequency", "uint32_t", 0x0007, "rw", "0"), (2, "maxFrequency", "uint32_t", 0x0007, "rw", "0")]),
    (0x6059, "vlFrequencyMotorMinMax", [(1, "minFrequency", "uint32_t", 0x0007, "rw", "0"), (2, "maxFrequency", "uint32_t", 0x0007, "rw", "0"), (3, "minAmount", "uint32_t", 0x0007, "rw", "0"), (4, "maxAmount", "uint32_t", 0x0007, "rw", "0")]),
    (0x6056, "vlVelocityMotorMinMaxAmount", [(1, "minVelocity", "uint32_t", 0x0007, "rw", "0"), (2, "maxVelocity", "uint32_t", 0x0007, "rw", "0")]),
    (0x6057, "vlVelocityMotorMinMax", [(1, "minVelocity", "uint32_t", 0x0007, "rw", "0"), (2, "maxVelocity", "uint32_t", 0x0007, "rw", "0"), (3, "minAmount", "uint32_t", 0x0007, "rw", "0"), (4, "maxAmount", "uint32_t", 0x0007, "rw", "0")]),
    (0x6048, "vlVelocityAcceleration", [(1, "deltaSpeed", "uint32_t", 0x0007, "rw", "0"), (2, "deltaTime", "uint16_t", 0x0006, "rw", "0")]),
    (0x6049, "vlVelocityDeceleration", [(1, "deltaSpeed", "uint32_t", 0x0007, "rw", "0"), (2, "deltaTime", "uint16_t", 0x0006, "rw", "0")]),
    (0x604A, "vlVelocityQuickStop", [(1, "deltaSpeed", "uint32_t", 0x0007, "rw", "0"), (2, "deltaTime", "uint16_t", 0x0006, "rw", "0")]),
]

ARRAYS = [
    (0x60FE, "digitalOutputs", "uint32_t", 0x0007, "rw", 2, [(1, "physicalOutputs", "0"), (2, "bitMask", "0")]),
]

# Entries already present in the reference generator are deliberately excluded
# from SCALARS and represented here for validation completeness.
REQUESTED_INDICES = sorted({
    index for index, *_ in SCALARS
} | {
    index for index, *_ in RECORDS
} | {
    index for index, *_ in ARRAYS
} | {0x603F, 0x6040, 0x6041, 0x6060, 0x6061, 0x6064, 0x606C, 0x6071, 0x6077, 0x607A, 0x60FF})
