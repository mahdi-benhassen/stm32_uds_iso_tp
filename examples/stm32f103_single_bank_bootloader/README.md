# STM32F103 Single-Bank UDS Bootloader (128 KB Flash)

This directory provides a production-grade single-bank bootloader and single-app upgrade architecture designed for resource-constrained microcontrollers such as the **STM32F103** with **128 KB Flash**.

---

## 1. Flash Memory Layout (128 KB, 2 KB Page Size)

```
0x08000000 ┌──────────────────────────────────────────┐
           │ BOOTLOADER (48 KB)                       │ Pages 0 - 23
0x0800C000 ├──────────────────────────────────────────┤
           │ SLOT A APPLICATION (60 KB)               │ Pages 24 - 53
0x0801B000 ├──────────────────────────────────────────┤
           │ METADATA A (2 KB)                        │ Page 54
0x0801B800 ├──────────────────────────────────────────┤
           │ DTC STORAGE / NVM DATA (18 KB)           │ Pages 55 - 63
0x0801FFFF └──────────────────────────────────────────┘
```

---

## 2. Boot Sequence & 2.0-Second Diagnostic Window

1. **Power-On Reset**:
   - Hardware boots into the Bootloader at `0x08000000`.
   - Bootloader starts CAN controller, ISO-TP transport, and UDS unified server.
   - Arms a **2.0-second diagnostic window** (`BOOTLOADER_WINDOW_MS = 2000`).

2. **Session Request Detection**:
   - If a UDS request requests DiagnosticSessionControl (`0x10 02` ProgrammingSession) within 2.0 seconds:
     - The timer is cancelled immediately.
     - The bootloader stays in **Programming Mode** to service firmware download (Services `0x34`, `0x36`, `0x37`, `0x31`).

3. **Application Verification & Jump**:
   - If the 2.0-second window elapses with no programming session request:
     - Bootloader inspects `app_metadata_t` at `0x0801B000`.
     - Validates `magic == 0x55445331` ('UDS1').
     - Checks application size bounds (`app_size <= 60 KB`).
     - Verifies Cortex-M vector table (Stack pointer within SRAM, Reset vector within Slot A).
     - Computes complete IEEE 802.3 CRC32 over the programmed application flash and checks equality.
     - If all checks pass: jumps to the application.

---

## 3. Anti-Bricking & Power-Loss Recovery

In single-bank bootloaders where dual-slot A/B rollback is not feasible due to flash size limits:
- **Flashing Protocol**:
  1. Erase Slot A and Metadata sector.
  2. Stream firmware chunks via Service `0x36` TransferData.
  3. Verify flash CRC32.
  4. Write `app_size`, `crc32`, `entry_point`, and `version` into `app_metadata_t`.
  5. **Write `magic = 0x55445331` as the very last operation**.
- **Failure Recovery**:
  - If power fails mid-flash, `magic` remains unwritten (`0xFFFFFFFF`).
  - On the next power-on, the bootloader detects invalid metadata, does NOT jump to broken code, remains in bootloader mode, and allows the tester to re-flash without bricking the ECU!
