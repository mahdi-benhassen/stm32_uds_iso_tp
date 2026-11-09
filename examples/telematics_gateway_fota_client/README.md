# Telematics Gateway & FOTA Diagnostic Client Example

This application example demonstrates how a Connected Telematics Control Unit (TCU) or Vehicle Gateway uses the **STM32 UDS ISO-TP** stack as its **Diagnostic Client & Transport Middleware** to perform Over-The-Air (FOTA) reprogramming on target electronic control units.

---

## 1. Architecture

```text
+-------------------------------------------------------------+
|        Connected TCU / Cloud Interface / OTA Manager        |
|   - Receives encrypted/signed firmware package from cloud   |
|   - Coordinates flashing window with vehicle safe state     |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|             FOTA Client Engine (fota_client.c)              |
|   - Executes OEM 10-Step UDS Flashing State Machine         |
|   - Calculates Seed/Key Challenge Responses (0x27)          |
|   - Slices firmware into block-sequenced transfer chunks    |
|   - Initiates Flash Erase (0x31) & Verification (0x31)      |
+-------------------------------------------------------------+
                              |
                              v [IsoTpLink / UdsServer]
+=============================================================+
|             STM32 UDS ISO-TP Middleware Layer               |
|   - ISO 15765-2 Flow-Controlled Multi-Frame Packaging       |
|   - $ST_{min}$ Timing & BlockSize (BS) Flow Control Handling|
|   - UDS Diagnostic Protocol Framing & Timeouts              |
+=============================================================+
                              |
                              v (CAN / CAN-FD Bus)
+-------------------------------------------------------------+
|             Target In-Vehicle Electronic Control Unit       |
|   - Flash Erase / Program State Machine (uds_flash_fsm.c)   |
|   - Cryptographic Signature & Hash Verification             |
|   - Dual-Bank Handoff & Bootloader Execution                |
+-------------------------------------------------------------+
```

---

## 2. Standardized 10-Step Automotive Flashing Sequence

The client state machine executes the canonical ISO 14229 / OEM reprogramming sequence:

1. **DiagnosticSessionControl (`0x10 0x02`)**: Switches target ECU into Programming Session.
2. **ControlDTCSetting (`0x85 0x02`)**: Disables DTC recording during flash transfer.
3. **CommunicationControl (`0x28 0x03 0x01`)**: Silences non-diagnostic normal bus traffic.
4. **SecurityAccess (`0x27 0x01` / `0x27 0x02`)**: Unlocks ECU via cryptographic seed-key exchange.
5. **RoutineControl EraseMemory (`0x31 0x01 0xFF00`)**: Erases the target Flash sector.
6. **RequestDownload (`0x34`)**: Informs ECU of target memory address and uncompressed payload size.
7. **TransferData (`0x36`)**: Streams firmware chunks with sequential block counters.
8. **RequestTransferExit (`0x37`)**: Finalizes block transfer and exits download mode.
9. **RoutineControl CheckProgrammingDependencies (`0x31 0x01 0x0202`)**: Triggers target checksum/hash check.
10. **ECUReset (`0x11 0x01`)**: Issues a HardReset to reboot into the newly flashed application.

---

## 3. How to Build & Run

From the repository root with host CMake:

```sh
cmake -S library -B build-host -G Ninja -DUDS_ISO_TP_BUILD_TESTS=ON -DUDS_ISO_TP_BUILD_EXAMPLES=ON
cmake --build build-host --target fota_client_example
./build-host/examples/telematics_gateway_fota_client/fota_client_example
```
