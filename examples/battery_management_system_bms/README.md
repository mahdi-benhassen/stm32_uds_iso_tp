# Electric Vehicle Battery Management System (BMS) Application Example

This example demonstrates how an EV/HEV Battery Management System (BMS) application uses the **STM32 UDS ISO-TP** stack as its safety and diagnostic middleware layer for high-voltage telemetry, Seed/Key security gating, and fault code management.

---

## 1. Architecture

```text
+-------------------------------------------------------------+
|               BMS Application Logic (bms_app.c)             |
|   - High-Voltage Pack Voltage & Current Acquisition         |
|   - State of Charge (SoC) & State of Health (SoH) Algorithm |
|   - Multi-Cell Voltage Extremes & Thermistor Sensing        |
|   - High-Voltage Isolation Monitor & Contactor Safety State |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|             BMS Diagnostic Adapter (bms_uds_adapter.c)      |
|   - DID Serializers (0x0200 Pack Telemetry, 0x0201 Cells)   |
|   - SecurityAccess (0x27 Level 1 Seed/Key Derivation)       |
|   - Protected RoutineControl (0x31 Contactor Test & Calib)  |
|   - DTC Mapping (Over-voltage, Isolation Loss, Welded Rly)  |
+-------------------------------------------------------------+
                              |
                              v [UdsServer / UdsCallbacks]
+=============================================================+
|             STM32 UDS ISO-TP Middleware Layer               |
|   - Diagnostic Session Control (0x10 Default/Extended)      |
|   - Security Gate & S3 Session Supervision                  |
|   - ISO 15765-2 Flow-Controlled Multi-Frame Transport       |
+=============================================================+
                              |
                              v
+-------------------------------------------------------------+
|               CAN / CAN-FD Bus Controller (HAL)             |
+-------------------------------------------------------------+
```

---

## 2. Implemented Diagnostic Services

| Service | Subfunction | Scope in this Application |
|---|---|---|
| `0x22` | ReadDataByIdentifier | Pack Telemetry (`0x0200`), Cell Extremes (`0x0201`), Contactors & HVIL (`0x0202`), Isolation ($k\Omega$) (`0x0203`) |
| `0x10` | DiagnosticSessionControl | Transitions between Default Session (`0x01`) and Extended Diagnostic Session (`0x03`) |
| `0x27` | SecurityAccess | Level 1 Seed/Key challenge-response algorithm unlocking safety-critical actuators |
| `0x31` | RoutineControl | Manual Contactor Cycle (`0x0201` - Security Protected) & SoC Coulomb Counter Calibration (`0x0202`) |
| `0x19` | ReadDTCInformation | Cell Over-Voltage Critical (`0xA00117`), Under-Voltage (`0xA00116`), Isolation Loss (`0xA00219`) |
| `0x14` | ClearDiagnosticInformation | Clears active DTCs and restores nominal safety status |

---

## 3. How to Build & Run

From repository root with host CMake:

```sh
cmake -S library -B build-host -G Ninja -DUDS_ISO_TP_BUILD_TESTS=ON -DUDS_ISO_TP_BUILD_EXAMPLES=ON
cmake --build build-host --target bms_application_example
./build-host/examples/battery_management_system_bms/bms_application_example
```
