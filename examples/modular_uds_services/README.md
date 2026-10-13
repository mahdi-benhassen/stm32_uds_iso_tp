# Modular UDS Services Reference Implementation

This directory provides standalone, modular implementations of core ISO 14229-1 UDS diagnostic services, ISO 15765-2:2016 transport layer with CAN-FD and 29-bit extended ID support, cryptographic security access, and fault lifecycle management with Flash wear-leveling, designed for bare-metal targets (including STM32F103, STM32C0, STM32F7, RP2350):

| Service / Module | File | Subfunctions / Description |
|---|---|---|
| **0x19 ReadDTCInformation** | `svc_read_dtc.c` | `0x01` (count), `0x02` (by mask), `0x03` (snapshot identification, Issue #119), `0x04` (snapshot DIDs: 0xDF00-0xDF04, 0xDD00), `0x06` (extended data: 0x01 occurrence, 0x02 aging), `0x0A` (supported DTCs) |
| **0x14 ClearDiagnosticInformation** | `svc_clear_dtc.c` | Clear all DTCs (`0xFFFFFF`), individual 24-bit DTCs, or group masks safely with ISO 14229-1 status reset |
| **0x85 ControlDTCSetting** | `svc_control_dtc.c` | `0x01` (on), `0x02` (off); directly linked to debouncing switch `dtc_is_setting_enabled()` (Issue #122) |
| **0x27 SecurityAccess** | `svc_security_access.c`, `svc_security_access.h` | Dual-level security: Level 1 (4-byte OEM), Level 2 (16-byte cryptographic AES-CMAC-128, Issue #124) |
| **AES-CMAC-128 Cryptography** | `crypto_aes_cmac.c`, `crypto_aes_cmac.h` | NIST SP 800-38B and FIPS 197 compliant 128-bit MAC derivation and constant-time verification |
| **0x28 CommunicationControl** | `svc_comm_control.c` | `0x00..0x03` (enable/disable Rx/Tx) for Normal and NM traffic |
| **0x23 ReadMemoryByAddress** | `svc_read_address.c` | ALFID decoding, 32-bit overflow check, address range protection |
| **0x3D WriteMemoryByAddress** | `svc_write_address.c` | ALFID decoding, SecurityAccess check, RAM range verification |
| **0x31 RoutineControl** | `svc_routine_control.c` | `0x01` (start), `0x02` (stop), `0x03` (results) with sequence validation |
| **Unified DTC Store** | `dtc_store.c`, `dtc_store.h` | FDC debouncing (-128..+127), driving cycle unlearning/aging, persistence callbacks, setting switch |
| **85 Standard DTCs** | `dtc_table_85.c`, `dtc_table_85.h` | All 85 DTCs extracted from specification (`DTC.uds.xlsx`) |
| **DTC NVM Flash Adapter** | `dtc_nvm_adapter.c`, `dtc_nvm_adapter.h` | Persistent Flash storage with wear-leveling algorithm for 85 DTCs (Issue #120) |
| **ISO 15765-2 CAN-FD & 29-bit** | `isotp_fd.c`, `isotp_fd.h`, `can_frame_fd.h` | 64-byte CAN-FD support and 29-bit extended CAN ID addressing macros (Issue #123) |
| **Drop-in UDS Server** | `uds_server.c`, `uds_server.h` | Complete drop-in server replacement with 21-service registration table and gating (Issue #121) |
| **Lifecycle Example** | `dtc_lifecycle_example.c` | Complete end-to-end usage example for Ignition ON, sensor task, Flash NVM, and Ignition OFF |

## Features & Standards Conformance
- **ISO 14229-1:2013 / 2020**: Complete table of 21 services, addressing gate (physical vs functional), session gate, and security gate.
- **ISO 15765-2:2016**: CAN-FD transport layer specification with payload lengths up to 64 bytes and 29-bit extended ID support.
- **NIST SP 800-38B / FIPS 197**: 128-bit AES-CMAC message authentication for Level 2 security access.
- **SPRMIB Handling**: Masking bit 7 (`0x80`) of subfunctions and suppressing positive responses properly.
- **Flash Endurance**: Wear-leveling parameter store distributes writes across Flash sectors to prevent endurance degradation.
- **Buffer Safety**: All responses verify output capacity against `ISOTP_TX_BUF_SIZE` to prevent memory corruption.
- **MISRA C & Zero-Warning**: Typedefs, strict unsigned constant literals (`U`), bounds checks, and zero warnings under strict compiler flags (`-Wall -Wextra -Werror -std=c99 -pedantic`).
