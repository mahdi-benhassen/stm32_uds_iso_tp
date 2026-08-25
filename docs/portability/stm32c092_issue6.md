# Issue #6 — STM32C092RC compile error

## 1. Executive result

**Classification: PARTIALLY FIXED — HIL REMAINS.**

The attached STM32C092RC project was inspected before repository changes. Its source compilation completed with no warnings, and the failure occurred during the ARM linker stage. The exact diagnostics are repeated `L6406E` execution-region placement errors followed by `L6407E` aggregate-size overflow. The primary root cause is the application’s static RAM footprint: the default `ISOTP_MAX_PAYLOAD=16384` allocates two large transport buffers inside one `UdsIsoTpEndpoint`, which cannot fit in the project’s 30 KiB SRAM together with the stack, FDCAN/UART/HAL state, and other globals.

This was not a demonstrated UDS/ISO-TP C-language syntax failure. The attached project uses ARM Compiler 6.22 with C99 enabled, and all listed UDS/ISO-TP sources compiled before linking failed. The repository now removes remaining C11 `_Static_assert` and compound-literal initialization dependencies from the authoritative core, adds a strict C99 freestanding compile target, and documents the explicit C092 memory configuration. A proprietary Keil compiler and a physical STM32C092 board are not available in the validation environment, so Keil link success and physical FDCAN/UDS interoperability remain release gates.

## 2. Original failure and exact evidence

The reporter’s attached project was downloaded from Issue #6 and inspected passively. The attached µVision build log identifies the following environment:

| Item | Observed value |
|---|---|
| IDE | µVision 5.41.0.0 |
| MDK | MDK-ARM Plus 5.41.0.0 |
| C compiler | ArmClang 6.22 |
| Linker | ArmLink 6.22 |
| Target | STM32C092RCTx |
| Core | Cortex-M0+ |
| Language mode | C99 enabled (`uC99=1`) |
| Optimization | Level 4 |
| Warning level | Level 3 |
| SRAM region | `0x20000000–0x200077FF`, 30 KiB |
| Flash region | `0x08000000–0x0803FFFF`, 256 KiB |
| Project sources | Standalone `library/src/*.c` plus `App/Src/*.c` and generated STM32C0 HAL/CMSIS sources |

The exact linker diagnostics in the attached `FDCAN_Classic_Frame_Networking.build_log.htm` are:

```text
FDCAN_Classic_Frame_Networking\FDCAN_Classic_Frame_Networking.axf: Error: L6406E: No space in execution regions with .ANY selector matching uds_app.o(.bss.s_endpoint).
FDCAN_Classic_Frame_Networking\FDCAN_Classic_Frame_Networking.axf: Error: L6406E: No space in execution regions with .ANY selector matching startup_stm32c092xx.o(STACK).
FDCAN_Classic_Frame_Networking\FDCAN_Classic_Frame_Networking.axf: Error: L6406E: No space in execution regions with .ANY selector matching usart.o(.bss.huart2).
FDCAN_Classic_Frame_Networking\FDCAN_Classic_Frame_Networking.axf: Error: L6406E: No space in execution regions with .ANY selector matching fdcan.o(.bss.hfdcan1).
FDCAN_Classic_Frame_Networking\FDCAN_Classic_Frame_Networking.axf: Error: L6406E: No space in execution regions with .ANY selector matching uds_app.o(.bss..L_MergedGlobals).
FDCAN_Classic_Frame_Networking\FDCAN_Classic_Frame_Networking.axf: Error: L6406E: No space in execution regions with .ANY selector matching main.o(.bss..L_MergedGlobals).
FDCAN_Classic_Frame_Networking\FDCAN_Classic_Frame_Networking.axf: Error: L6406E: No space in execution regions with .ANY selector matching stm32c0xx_hal.o(.data..L_MergedGlobals).
FDCAN_Classic_Frame_Networking\FDCAN_Classic_Frame_Networking.axf: Error: L6406E: No space in execution regions with .ANY selector matching stm32c0xx_hal.o(.bss.uwTick).
FDCAN_Classic_Frame_Networking\FDCAN_Classic_Frame_Networking.axf: Error: L6406E: No space in execution regions with .ANY selector matching system_stm32c0xx.o(.data.SystemCoreClock).
FDCAN_Classic_Frame_Networking\FDCAN_Classic_Frame_Networking.axf: Error: L6407E: Sections of aggregate size 0x96e4 bytes could not fit into .ANY selector(s).
Finished: 2 information, 0 warning and 10 error messages.
"FDCAN_Classic_Frame_Networking\FDCAN_Classic_Frame_Networking.axf" - 10 Error(s), 0 Warning(s).
```

The historical build log shows that `endpoint.c`, `isotp.c`, `uds.c`, `uds_did.c`, the former `uds_security_provider.c`, `uds_download.c`, `can_transport.c`, `uds_app.c`, and `uds_platform.c` all reached the link stage. Therefore, the available evidence does not support attributing the original failure to an unaccepted protocol syntax construct. In the current architecture, SecurityAccess remains application-owned through callbacks; the deterministic reference helper is test-only under `tests/security/`.

## 3. Root cause

The attached application declares a static `UdsIsoTpEndpoint s_endpoint`. The endpoint contains one `IsoTpRx`, one `IsoTpTx`, a `UdsServer`, the UDS response buffer, and a pending CAN frame. Both `IsoTpRx` and `IsoTpTx` contain an `ISOTP_MAX_PAYLOAD` byte array. With the default 16,384-byte bound, a host ABI probe reports:

| Object | Size at `ISOTP_MAX_PAYLOAD=16384` | Size at `ISOTP_MAX_PAYLOAD=4095` |
|---|---:|---:|
| `IsoTpCanFrame` | 72 B | 72 B |
| `IsoTpConfig` | 20 B | 20 B |
| `IsoTpRx` | 16,432 B | 4,144 B |
| `IsoTpTx` | 16,440 B | 4,152 B |
| `UdsServer` | 136 B | 136 B |
| `UdsIsoTpEndpoint` | 37,352 B | 12,776 B |

The C092 linker has only 30 KiB of SRAM, so the 37,352-byte endpoint alone exceeds the available region before the startup stack, FDCAN handle, UART handle, HAL globals, and system globals are placed. The attached log’s `.bss.s_endpoint`, `STACK`, FDCAN, UART, and HAL placement errors are consistent with this memory pressure.

The recommended C092 project configuration is therefore an explicit application/product definition of `ISOTP_MAX_PAYLOAD=4095`, or another measured value that fits the complete application. This does not silently change the library default and does not remove the library’s extended First-Frame implementation for larger-memory targets.

## 4. Portability audit

The authoritative `library/` implementation was audited for hardware and compiler coupling. No `HAL_*`, CMSIS, POSIX, pthread, malloc, calloc, realloc, free, GCC `__attribute__`, GCC builtins, `typeof`, `restrict`, or unaligned integer-pointer protocol decoding was found in `library/src` or `library/include`. Protocol decoding uses explicit byte shifts and indexed byte access. Callback types are declared in public headers and are used without incompatible casts.

Before this change, six compound literals existed in initialization paths: `isotp.c` used `(IsoTpConfig){0}`, `uds.c` used `(UdsCallbacks){0}`, and `uds_download.c` used zero compound literals for memory maps, callbacks, and metadata. The C092 project uses C99 and therefore does not establish these as the observed failure, but the core now uses explicit assignments and existing default initialization functions to improve compatibility across ARM Compiler variants. The two C11 `_Static_assert` lines in `uds.h` were replaced by the existing preprocessor `#error` bound check, allowing strict C99 compilation without weakening the `uint16_t` API limit.

No protocol behavior was intentionally changed by these initialization edits. The library remains hardware-independent, heap-free, bounded, and free of STM32 HAL calls. The FDCAN adapter remains outside the protocol state machine.

## 5. Implemented repository changes

| Change | Result |
|---|---|
| Explicit `isotp_rx_init()` and `isotp_tx_init()` fallback initialization | Removes compound-literal dependency and preserves default configuration behavior |
| Explicit `uds_server_init()` callback fallback initialization | Removes compound-literal dependency and preserves null callback semantics |
| Explicit `uds_download_init()` memory, callback, and metadata initialization | Removes compound-literal dependency and preserves zero-state behavior |
| Portable UDS length bound check | Removes C11 `_Static_assert`; retains compile-time rejection above `uint16_t` API limits |
| Strict C99 freestanding CMake target | Compiles all core sources independently of STM32 HAL with `-std=c99 -ffreestanding -Wall -Wextra -Wconversion -Wsign-conversion -Werror` |
| `examples/stm32c092/README.md` | Documents C092 memory configuration, Keil settings, FDCAN translation, source list, and HIL requirements |
| `docs/portability/stm32c092_issue6.md` | Records this reproduction, root cause, evidence, validation boundary, and remaining work |

The current fix commit is the commit that contains these changes after validation and publication. No duplicate ISO-TP implementation or CANopen dependency was introduced.

## 6. Portability matrix

| Toolchain/target | Compile | Link | Warnings | Tests | Evidence status |
|---|---:|---:|---|---:|---|
| Host GCC/CMake | PASS | PASS | No unsuppressed project warnings under strict build | PASS; 4/4 CTest suites | Executed locally and in hosted CI before this report |
| STM32F767 / GNU Arm Embedded | PASS | PASS | Two existing generated `syscalls.c` conversion warnings are non-fatal by scoped policy | Host protocol tests remain PASS | Executed cross-build; F767 is bxCAN/Classical CAN |
| STM32C092RC / ARM Compiler 6.22 | Source compile reached link stage in attached project; exact link failure reproduced from build log | FAIL in reporter project due SRAM placement | 0 warnings, 10 linker errors in supplied log | Not run by sandbox | Original failure is memory/link configuration, not demonstrated core syntax failure |
| STM32C092RC / GNU Arm Embedded | PASS; `arm-none-eabi-gcc` 13.2.1 strict core compile with C092 payload bound | Not a complete vendor HAL/application link | Strict warnings treated as errors | Host tests remain PASS | Deterministic portability substitute; not evidence of Keil compatibility |
| STM32C092RC physical board | NOT EXECUTED | NOT EXECUTED | NOT EXECUTED | NOT EXECUTED | Requires board, transceiver, analyzer/tester, firmware flash, and trace capture |

## 7. Memory and configuration guidance

The default `ISOTP_MAX_PAYLOAD` is 16,384 bytes and remains appropriate only where the complete endpoint and application fit the target memory map. For the attached C092 project’s 30 KiB SRAM, set `ISOTP_MAX_PAYLOAD=4095` in the Keil C/C++ preprocessor definitions and verify the final map file. The resulting measured endpoint size is 12,776 bytes on both the host ABI probe and the Cortex-M0+ ARM GCC size probe, leaving room for application objects and stack, but the final Keil map remains authoritative.

A 4,095-byte bound supports normal ISO-TP First-Frame lengths through 4,095 bytes. It does not provide a 5,000-byte extended transfer because the selected application bound rejects payloads above its configured capacity. Larger-memory targets can select a larger bound while retaining the extended First-Frame path.

## 8. Validation results

The following repository gates passed after the portability changes:

| Gate | Result |
|---|---|
| Standalone architecture check | PASS; 170 tracked paths checked |
| Validation asset check | PASS; 13 conformance vectors and 18 physical cases |
| Normal CMake/Ninja build | PASS |
| Normal CTest | PASS; 4/4 suites |
| ASan/UBSan build and CTest | PASS; 4/4 suites |
| Strict C99 host portability build | PASS; all six core sources compiled freestanding |
| Strict C99 ARM portability compile | PASS; Cortex-M0+ and `ISOTP_MAX_PAYLOAD=4095` |
| Coverage build and CTest | PASS; 4/4 suites; 52% total source coverage |
| ISO-TP source coverage | PASS; 86% for `library/src/isotp.c` |
| clang-format | PASS |
| clang-tidy | PASS |
| cppcheck | PASS; informational configuration-count note only |
| STM32F767 cross-build | PASS; 22,712 B Flash and 39,064 B RAM |
| Classical CAN HIL runner | DRY-RUN only; 15 cases |
| CAN-FD HIL runner | DRY-RUN only; 18 cases |
| Keil/STM32C092 build after fix | NOT EXECUTED; armclang/armlink are unavailable in the sandbox |
| STM32C092 physical HIL | NOT EXECUTED; hardware and trace equipment unavailable |

## 9. Required next steps for the reporter

1. Set `ISOTP_MAX_PAYLOAD=4095` in the C092 Keil target’s C/C++ preprocessor definitions, or choose a smaller measured bound if the application adds more RAM consumers.
2. Rebuild with ArmClang 6.22 and ArmLink 6.22 using the existing project’s map-file generation enabled. Confirm that the linker’s IRAM region has sufficient headroom for the endpoint, stack, FDCAN/UART handles, and HAL globals.
3. Confirm that the FDCAN adapter translates HAL data-length enumerations to actual byte lengths and preserves `is_fd` and BRS metadata. For a Classical CAN frame on FDCAN, use `FDCAN_CLASSIC_CAN`, BRS off, and a data length of at most 8 bytes.
4. Run the host and strict C99 tests, then flash the C092 board and execute Tester Present, Diagnostic Session Control, multi-frame FF/FC/CF, BS, STmin, timeout, and ECU-reset tests with an independent analyzer.
5. Attach the Keil build log/map, compiler settings, firmware SHA, MCU/board identity, bit rates, transceiver identity, and raw timestamped CAN traces to Issue #6.

Until those steps are complete, the correct status is **PARTIALLY FIXED — HIL REMAINS**. The source portability improvements and deterministic GCC checks reduce compiler-compatibility risk, but GCC cannot substitute for the proprietary Keil build, and a successful link cannot substitute for C092 FDCAN/UDS physical evidence.

## References

[1]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/6 "GitHub Issue #6 — STM32C092RC compile error"
[2]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/tree/main/library "Authoritative hardware-independent UDS/ISO-TP library"
[3]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/tree/main/examples/stm32c092 "STM32C092RC integration guide"
[4]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/blob/main/docs/standalone/validation.md "Repository validation procedure"
