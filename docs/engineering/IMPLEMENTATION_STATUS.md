# Implementation Status: Open Issues #13–#18

This document records the status of the dependency-driven open-issue campaign for `mahdi-benhassen/stm32_uds_iso_tp`. The work is split into logical commits on `main`; the issues remain open so the reporter can review the changes and decide whether the behavior matches the target integration.

## Delivered changes

| Issue | Delivered change | Evidence | Status boundary |
|---:|---|---|---|
| #13 / #14 | C092 FDCAN TX Event FIFO correlation now has an ISR drain and a mainline polling fallback. The generic endpoint remains hardware-independent, and the bxCAN adapter does not regain a transport `tx_pending` coupling. | `5dc46ba`, C092 shim contract, 1000-request bxCAN regression, C092 guide | Polling drains stored controller events; it cannot manufacture physical bus completion and still requires nonzero TX Event FIFO elements in CubeMX. |
| #18 | Generic `0x11` accepts reset types `0x01` through `0x05`, preserves suppress-positive-response behavior, and defers execution. The C092 platform policy truthfully supports hard and soft reset and rejects unsupported types. | `68e1f65`, reset and endpoint tests | Keil ArmClang and physical reset/HIL were not executed in this environment. |
| #15 | Reusable dependency-free AES-128, AES-CMAC-128, constant-time equality, and a 16-byte seed derivation helper were added without a production secret or generic `0x27` algorithm assumption. | `617a9e9`, `796de9f`, RFC 4493 KATs and seed-helper test | Application key provisioning, erasure, freshness, authorization, and product security review remain application responsibilities. |
| #17 | `UdsDtcBackend` now uses explicit capability bits and a bounded report callback. `0x19` retains the legacy raw callback path for compatibility; `0x14` uses a separate clear-DTC callback. | `9010404`, DTC backend and clear-routing contract | No device-specific DTC records are bundled. A product must provide the backend and data model; unsupported capabilities return an explicit NRC. |
| #16 | The dispatcher now has separate backend groups for memory, DID extension, transfer extension, timing, periodic/events, link control, authentication, and secured data. Memory preflight and nonzero periodic/event queue bounds are explicit. | `9010404`, `dbb0f8c`, `57d5c83`, service selector/preflight contracts | These are bounded integration contracts, not claims that every device-specific service behavior is complete or production-ready. |

The Phase-0 architecture audit is recorded in `docs/engineering/OPEN_ISSUES_ARCHITECTURE_AUDIT.md`. It explains why #14 was prioritized before service expansion and why transport completion must not be confused with generic endpoint sequencing.

## Validation evidence

The standalone host build currently registers ten CTest contracts. All ten pass in both the normal build and the AddressSanitizer/UndefinedBehaviorSanitizer build. The suite includes ISO-TP, core UDS, session/security, endpoint, modular service selectors, adapter reuse, C092 TX-event correlation, AES-CMAC, and DTC contracts. The adapter contract includes 1,000 consecutive Classic CAN TesterPresent request/response cycles without endpoint or peripheral reinitialization.

The top-level STM32F767 target builds successfully with `/usr/bin/arm-none-eabi-gcc` using `cmake/gcc-arm-none-eabi.cmake`. The strict freestanding ARM GCC portability check also passes for every `library/src/*.c` file with `ISOTP_MAX_PAYLOAD=4095`, which is the bounded C092 compatibility setting. A host-toolchain build of the CubeMX project is not a valid target build and fails in the generated startup assembly; that result is not used as target evidence.

The repository does not contain Keil MDK/Arm Compiler 6, `armlink`, `fromelf`, an STM32C092 board, a CAN analyzer, or a debugger. Therefore, Keil compilation/linking, C092 ELF/HEX/BIN generation, electrical bus traces, physical TX-event measurements, and HIL reset execution are **not executed evidence** here. No production-readiness claim is made.

## Review guidance

The C092 application must configure a nonzero TX Event FIFO element count and use `FDCAN_STORE_TX_EVENTS` for the maintained adapter header. It may use the TX-event interrupt callback for low latency or call `uds_c092_fdcan_poll_tx_events()` from its main loop. The RX callback should convert the generated HAL data-length code through `uds_c092_fdcan_data_length_bytes()` rather than treating the code as a byte count.

The generic library remains free of HAL, CMSIS, registers, delays, heap allocation, arbitrary retries, and hidden mutable application state. Application backends are responsible for bounded execution, permissions, storage, credentials, queues, event cancellation, and target-specific physical actions.
