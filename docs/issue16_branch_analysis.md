# Issue #16 Branch Analysis

## Scope and repository state

This report is the result of the first Issue #16 prompt. It inspects the two existing remote branches before any UDS or ISO-TP production implementation is added. The inspected branch tips are `main` at `e04d9135419643e9ce290d78679003a783cdee60` and `stm32f767_canopen_cubemx` at `b97c6ca48f02127ff28851491be876027754ac6a`. The repository currently exposes exactly these two remote branches, with `origin/HEAD` pointing to `main`.

The branch comparison shows that `main` is the established reference architecture, whereas `stm32f767_canopen_cubemx` is a substantial STM32CubeMX-generated platform port. The CubeMX branch contains a generated `.ioc` file, vendored STM32F7 HAL/CMSIS sources, generated startup and linker files, CMake presets, and a ported copy of the application and validation ecosystem. The branches share the conceptual CANopen application and test layers but do not share the same platform build boundary.

## Architecture summary

| Area | `main` | `stm32f767_canopen_cubemx` | Issue #16 consequence |
|---|---|---|---|
| Runtime model | Bare-metal main loop; no FreeRTOS or RTOS task layer was found in the inspected build and source paths. | Bare-metal main loop; the CubeMX project does not enable FreeRTOS, and no scheduler/task layer is present. | Use a bounded deferred service call from the main loop. Do not design a FreeRTOS task path for the current targets. |
| MCU and CAN | STM32F767 reference using STM32 HAL, bxCAN, and the pinned CanOpenSTM32/CANopenNode integration. | STM32F767 CubeMX project using the generated HAL/CMSIS platform and the same pinned CanOpenSTM32/CANopenNode integration. | UDS must share the existing bxCAN peripheral through a separate adapter and must not create a second CAN interrupt owner. |
| CAN pins and rate | The documented reference uses CAN1 receive PA11 and transmit PA12, AF9, at 500 kbit/s. | The `.ioc` declares PA12 as `CAN1_TX`, PI9 as `CAN1_RX`, and `CAN1.CalculateBaudRate=500000`; the generated source comments and current README also need to be treated as authoritative for the actual CubeMX port. | The adapter must derive its GPIO/filter assumptions from each platform’s actual configuration. The apparent PA11-versus-PI9 receive-pin difference is a mandatory reconciliation item before hardware acceptance. |
| CAN interrupt priorities | Existing HAL callbacks and interrupt handlers are owned by the CANopen integration. | CubeMX-generated NVIC entries enable `CAN1_TX_IRQn`, `CAN1_RX0_IRQn`, `CAN1_RX1_IRQn`, and `CAN1_SCE_IRQn`; current generated code assigns CAN interrupts priority 5,0. | UDS must consume frames after the common CAN ISR path and must not register competing handlers. |
| CANopen timing | TIM7 provides the documented 1 ms real-time service cadence; CANopen processing runs in the mainline around the existing `CO_process` lifecycle. | `TIM7` is configured by CubeMX with `Prescaler=108-1` and `Period=1000-1`; the generated application documents a 1 ms CANopen processing interrupt. | UDS work must be budgeted separately from the existing CANopen timing path and must never delay the 1 ms service. |
| Clock | The main branch requires the externally supplied STM32CubeF7 package and a board-specific clock/linker configuration. | CubeMX declares a 25 MHz HSE and 216 MHz system-clock reference. | Timing calculations and ISO-TP timeout documentation must be platform-specific where clock assumptions differ. |
| Memory | The main linker reference defines 1536 KiB application FLASH, a 512 KiB `FLASH_NVM` region, and 512 KiB RAM. | `STM32F767xx_FLASH.ld` is a generated 1024 KiB FLASH / 512 KiB RAM reference and does not by itself provide a bootloader or UDS download activation scheme. | Firmware download must not guess a memory map. It requires explicit linker-region validation and a non-overlapping image-storage design. |
| Build system | CMake plus `cmake/arm-none-eabi-gcc.cmake`; the main build requires `STM32_CUBE_F7_DIR` and an explicit linker script. | CMake plus `cmake/gcc-arm-none-eabi.cmake`, `CMakePresets.json`, and generated `cmake/stm32cubemx/CMakeLists.txt`; HAL/CMSIS are vendored in the branch. | Shared UDS/ISO-TP sources should be added through platform-neutral CMake lists, with one small platform adapter source list per branch. |
| CI | The main branch runs static analysis, host validation, SocketCAN-capability checks, cross-builds, evidence generation, and release gates. | The CubeMX branch now runs the ported validation workflow, including static analysis, host tests, multiple STM32F767 personalities, memory checks, coverage, sanitizer checks, and firmware artifact validation. | CI must build the UDS-disabled default and explicitly enabled UDS profiles, without making hardware-only tests appear to pass. |

## Existing diagnostic and protocol code

The repository already contains `middleware/diagnostics/uds_isotp.py`, `tests/test_uds_isotp.py`, and `tests/run_uds_isotp_contract.py`. The Python module is explicitly a deterministic host-side contract model. It supports a small ISO-TP/UDS test surface, including single-frame and multi-frame helpers, session control, ECU reset, tester present, read/write DID callbacks, and basic negative responses. Its module documentation explicitly states that the STM32F767 firmware contains no embedded ISO-TP transport or UDS server. It must therefore be treated as a test oracle and contract reference, not copied blindly into firmware.

The existing embedded diagnostics layer is `App/Src/canopen_reference_diagnostics.c` with its public header. It reports bounded CAN hardware and runtime diagnostic state from mainline code and may emit optional UART diagnostics on a periodic schedule. It is not a UDS service dispatcher, ISO-TP transport, DID registry, security provider, or download manager. The new implementation should use it only as an error/status integration point.

The existing CAN abstraction is split between the production CANopenNode STM32 binding and the project’s validation facade under `middleware/canopen/port/can_port.c` and `can_port.h`. The production firmware’s CANopenNode binding remains the owner of bxCAN setup, callbacks, acceptance filtering, and CANopen RX/TX lifecycle. The new UDS transport adapter must not replace that owner. It should receive a bounded copy of relevant frames from the common CAN receive path or a carefully integrated shared dispatcher, then defer ISO-TP and UDS work to the main loop.

## Files and layers that should be shared

The following layers are natural candidates for one protocol-neutral implementation shared by both branches:

| Shared layer | Proposed contents |
|---|---|
| `middleware/diagnostics/isotp/` | Classic-CAN ISO-TP state machines, frame parsing, SF/FF/CF/FC handling, block size, STmin, sequence validation, bounded payload storage, and timeout state. No STM32 HAL symbols and no CANopenNode symbols. |
| `middleware/diagnostics/uds/` | UDS request dispatch, session state, NRC generation, table-driven DIDs, SecurityAccess provider interface, download state machine interface, and compile-time service switches. |
| Host tests | Pure C tests for ISO-TP and UDS state transitions, malformed frames, timeouts, overflow, NRCs, and bounded-resource behavior. Existing Python contracts can remain as a separate interoperability reference. |
| Project configuration | Service-enable macros, request/response CAN identifiers, payload limits, timer tick units, processing budgets, and default-disabled UDS profile settings. |
| Documentation | Protocol behavior, configuration, CAN coexistence policy, security limitations, download boundaries, and test evidence. |
| Application data callbacks | DID callbacks should reference authoritative project state such as node ID, CAN error counters, watchdog state, reset cause, and diagnostic counters instead of duplicating state. |

The shared layer must not include `HAL_CAN_*`, `HAL_GPIO_*`, `HAL_TIM_*`, `__disable_irq`, direct Flash programming, or platform-specific reset code. Those belong behind explicit callback/provider interfaces.

## Files that must remain platform-specific

The following files and responsibilities must remain separate between `main` and the CubeMX branch:

| Platform-specific area | Reason |
|---|---|
| `Core/Src/main.c`, `Core/Src/stm32f7xx_it.c`, and `Core/Src/stm32f7xx_hal_msp.c` | These files own the generated startup, IRQ entry points, GPIO alternate functions, NVIC setup, and HAL callbacks. The CubeMX branch must keep its generated infrastructure authoritative. |
| `stm32f767_canopen.ioc` and generated CubeMX files | Only the CubeMX branch has the `.ioc` source of truth. UDS integration should use USER CODE regions or narrowly documented generated-code changes. |
| `cmake/stm32cubemx/CMakeLists.txt`, `cmake/gcc-arm-none-eabi.cmake`, and main’s external CubeF7 variables | The two branches use different vendor-source and toolchain boundaries. A shared source list must not assume the same HAL layout. |
| Startup and linker scripts | `main` uses an externally selected linker script with a reserved NVM region; the CubeMX branch uses `STM32F767xx_FLASH.ld` and a different generated memory layout. UDS download storage requires separate validation. |
| CANopen port fixups | The CubeMX branch contains `App/Inc/canopen_reference_port_fixup.h` and its source, while `main` uses its established application/port boundary. These must not be replaced by a generic UDS transport shortcut. |
| Hardware Flash and reset providers | Flash erase/program, image validation, ECU reset, bootloader handoff, and board-specific safety behavior require separate platform providers. |
| CI workflow entry points | The branches have different CMake invocation requirements and evidence paths. CI should call the shared tests but retain branch-specific cross-build commands. |

Generated STM32 HAL/CMSIS files and the pinned `third_party/CanOpenSTM32` submodule must not be hand-edited to implement UDS. The production UDS integration belongs in project-owned middleware and adapter files.

## Exact integration points

The first embedded integration point is the existing CAN receive path. On both branches, CAN IRQ entry eventually reaches HAL/CANopenNode callbacks. The adapter must register through a single shared dispatch point or receive a bounded frame handoff from the existing callback. The ISR-side operation should copy only the CAN identifier, DLC, and up to eight data bytes into a statically allocated ring buffer, increment overflow/error counters, and return. It must not parse UDS, run ISO-TP state machines, call service callbacks, access Flash, or reset the MCU.

The second integration point is the existing application mainline. The current application initializes the CANopen instance and timer, starts the CAN service, processes CANopen state, reports diagnostics, services the watchdog, and handles reset requests. UDS should be called from a bounded diagnostic-processing slot in that mainline. It must have an explicit maximum number of queued frames or microseconds per iteration, and it must expose progress to the existing watchdog supervision.

The third integration point is the existing 1 ms TIM7 path. TIM7 should remain the authoritative CANopen timing source. A small monotonic tick provider may be shared with UDS/ISO-TP, but UDS must not add a competing timer ISR or perform protocol work from `TIM7_IRQHandler`. If DWT timing instrumentation is enabled, it should measure the existing ISR and deferred budgets without changing the scheduling contract.

The fourth integration point is diagnostics and application state. DID callbacks should read the existing node identity, CAN error state, watchdog state, reset cause, and diagnostic counters. A new UDS diagnostic status model should not silently create a second source of truth. Existing `CANopenReferenceDiagnostics_*` functions can receive summarized UDS errors from mainline code.

The fifth integration point is reset and Flash policy. ECU reset must be represented as a deferred request consumed by the application, not executed inside a UDS or CAN callback. UDS download services must use a separate firmware-image storage provider and must not call the existing CANopen parameter NVM backend. Until a bootloader and validated inactive-image region exist, final activation/reboot must remain explicitly pending.

## Risks and constraints

The most immediate hardware risk is the CAN receive-pin discrepancy: the main documentation describes CAN1 RX on PA11, while the CubeMX `.ioc` declares PI9 for `CAN1_RX` and PA12 for `CAN1_TX`. This must be resolved against the actual board schematic and CubeMX-generated GPIO source before a hardware acceptance claim.

The most significant real-time risk is protocol starvation. ISO-TP multi-frame traffic can create sustained work, and UDS services such as DID reads, SecurityAccess, and download operations can be more expensive than CANopen frame handling. Separate bounded queues, a bounded per-loop diagnostic budget, and measured worst-case behavior are required to preserve CANopen timing and watchdog progress.

The primary memory risk is the absence of a common download memory map. The two branches use different linker boundaries, and the main branch reserves a `FLASH_NVM` region while the CubeMX branch’s generated linker script does not establish an inactive-image/bootloader contract. A download interface can be implemented, but activation and reboot must not be represented as production-ready until the memory layout and boot flow are validated.

The protocol risk is treating the current Python model as an embedded implementation. It has useful contract coverage but intentionally omits production concerns such as static-buffer ownership, ISR handoff, transport arbitration, P2/P2* scheduling, service execution budgets, SecurityAccess policy, and Flash safety.

The coexistence risk is acceptance filtering and callback ownership. UDS request/response identifiers must be configurable and must coexist with default, remapped, and extended CANopen COB-IDs without overwriting CANopen buffers or starving CANopen traffic. Filters must be derived from the actual CANopen configuration rather than assuming only the default IDs.

The safety risk is false assurance. Passing host tests or a cross-build cannot establish ISO 14229, ISO 15765-2, CAN physical-layer, functional-safety, or production Flash claims. Hardware-in-the-loop and independent CAN equipment are required for those claims.

## Recommended architecture

Use a three-layer design. The first layer is a pure ISO-TP core with static buffers and explicit state transitions for RX and TX. It accepts and emits transport-neutral CAN-frame structures through callbacks. The second layer is a pure UDS server that consumes complete application payloads and emits complete responses through an ISO-TP interface. It owns session transitions, supported-service dispatch, NRC behavior, DID lookup, SecurityAccess provider calls, and deferred operation requests. The third layer is a platform adapter that owns only bounded frame queues, configurable CAN IDs, CAN filter participation, transport statistics, time access, and deferred handoff to the existing application.

The adapter should be integrated below the existing CANopen application owner, not beside it as a second HAL/CAN interrupt stack. The default compile-time configuration should keep UDS disabled for existing profiles. Enabling UDS should require an explicit `CANOPEN_REFERENCE_ENABLE_UDS=1` option and a selected diagnostic configuration. The UDS core should be usable in host tests without linking STM32 HAL or CANopenNode.

## Implementation sequence

1. Preserve this analysis report and establish the branch-specific integration contract.
2. Add host-tested ISO-TP and UDS cores with static, bounded resources and no hardware dependencies.
3. Add service configuration and negative-response behavior; unsupported services must return defined NRCs rather than fake success.
4. Add the table-driven DID registry and callback/provider interfaces for security, reset, and download operations.
5. Add the STM32 transport adapter on `main`, integrating through the existing CANopen callback owner and main loop.
6. Add the CubeMX transport adapter on `stm32f767_canopen_cubemx`, using its generated CAN handle, GPIO, NVIC, and TIM7 infrastructure.
7. Add coexistence, adversarial, timing, and host regression tests before claiming target readiness.
8. Add the hardware acceptance runner and explicitly distinguish skipped hardware capability from passed runtime evidence.
9. Run compiler hardening, formatting, static analysis, sanitizer, unit, CMake, and target builds.
10. Document the final behavior and produce an independent audit. Issue #16 must remain open until every mandatory requirement is either demonstrably passed or explicitly identified as not implemented/pending.

## Sources inspected

1. [Repository main branch](https://github.com/mahdi-benhassen/stm32_canopen_reference/tree/main), including `README.md`, `CMakeLists.txt`, `middleware/diagnostics/uds_isotp.py`, existing application diagnostics, CANopen integration, linker script, tests, and CI workflow.
2. [STM32F767 CubeMX branch](https://github.com/mahdi-benhassen/stm32_canopen_reference/tree/stm32f767_canopen_cubemx), including `stm32f767_canopen.ioc`, generated `Core/` and `Drivers/` files, `CMakePresets.json`, CubeMX CMake configuration, linker script, application integration, tests, and CI workflow.
3. [Issue #16](https://github.com/mahdi-benhassen/stm32_canopen_reference/issues/16), the requested UDS ISO 14229 and STM32F767 example scope.
4. [CANopenNode](https://github.com/CANopenNode/CANopenNode) and [CanOpenSTM32](https://github.com/CANopenNode/CanOpenSTM32), the pinned upstream CANopen integration sources listed by the repository’s `THIRD_PARTY.md`.
