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

## Server Timing & Polling (Issue #146)

The UDS server maintains several time-sensitive state machines:
- **S3 Server Timeout (5000 ms)**: Automatically resets session back to `DEFAULT` if no tester communication is received.
- **Security Access Lockout (10 seconds)**: Prevents brute-force key attempts after failed seed/key requests.
- **ECU Reset Grace Period (50 ms)**: Allows pending positive responses (`0x51`) to be transmitted over CAN before triggering a soft or hard system reset.

To maintain accurate timing on bare-metal microcontrollers (e.g., STM32, RP2350), two integration patterns are supported:

### Option A: Main Loop Superloop Integration (Recommended for bare-metal)
Call `uds_server_poll_ms(HAL_GetTick())` inside your `while (1)` main loop. It automatically calculates elapsed milliseconds and prevents rapid timer expiration:

```c
while (1) {
    /* CAN message processing ... */
    
    /* Safely step UDS timers using millisecond ticks */
    uds_server_poll_ms(HAL_GetTick());
}
```

### Option B: 1ms Timer Interrupt / SysTick Callback
If you have a dedicated 1ms hardware timer interrupt or RTOS tick hook, call `uds_server_poll()` directly:

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) { /* 1ms period */
        uds_server_poll();
    }
}
```
