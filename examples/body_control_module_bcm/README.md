# Automotive Body Control Module (BCM) Application Example

This example demonstrates how an automotive Body Control Module (BCM) firmware application uses this repository as its **Diagnostic Middleware Layer** to expose standardized ISO 14229-1 UDS diagnostic capabilities over ISO 15765-2 CAN / CAN-FD transport.

---

## 1. Architecture

```text
+-------------------------------------------------------------+
|               BCM Application Logic (bcm_app.c)             |
|   - Exterior Lighting Control (Headlights, High Beam, Haz)  |
|   - Wiper Motor Relay & Speed Governor                      |
|   - Central Locking & Door Ajar Monitoring                  |
|   - Low-Voltage Battery & Environmental Monitoring          |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|             BCM Diagnostic Adapter (bcm_uds_adapter.c)      |
|   - DID Serializers / Deserializers                         |
|   - I/O Actuator Overrides (0x2F Return/ShortTermAdjust)    |
|   - DTC Mapping & Diagnostic Clear Engine                   |
+-------------------------------------------------------------+
                              |
                              v [UdsServer / UdsCallbacks]
+=============================================================+
|             STM32 UDS ISO-TP Middleware Layer               |
|   - Session State Machine (0x10)                            |
|   - Security Gate & S3 Timeout Management                   |
|   - Multi-Frame ISO 15765-2 Reassembly & Pacing             |
+=============================================================+
                              |
                              v
+-------------------------------------------------------------+
|               CAN / CAN-FD Bus Controller (HAL)             |
+-------------------------------------------------------------+
```

---

## 2. Implemented Diagnostic Services

| Service | Name | Scope in this Application |
|---|---|---|
| `0x22` | ReadDataByIdentifier | Read BCM state (`0x0100`), battery & temp (`0x0101`), VIN (`0xF190`), Part Number (`0xF187`) |
| `0x2F` | InputOutputControlByIdentifier | Diagnostic override of Headlights (`0x0102`), Horn (`0x0103`), and Hazard indicators (`0x0104`) with `returnControlToECU` |
| `0x19` | ReadDTCInformation | Reports active lighting short-circuits (`0xB10001`), door actuator stall (`0xB10002`), and battery under-voltage (`0x900116`) |
| `0x14` | ClearDiagnosticInformation | Clears active DTCs across memory blocks and restores nominal reporting |
| `0x31` | RoutineControl | Initiates Wiper Sweep Self-Test (`0x0301`) |
| `0x11` | ECUReset | Soft reset returning all actuators and state machine to default states |

---

## 3. How to Build & Run

From the repository root with host CMake:

```sh
cmake -S library -B build-host -G Ninja -DUDS_ISO_TP_BUILD_TESTS=ON -DUDS_ISO_TP_BUILD_EXAMPLES=ON
cmake --build build-host --target bcm_application_example
./build-host/examples/body_control_module_bcm/bcm_application_example
```
