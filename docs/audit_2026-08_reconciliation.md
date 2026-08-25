# Engineering Audit Reconciliation

## Purpose

This document reconciles the attached engineering audit with the checked-in STM32F767 CANopen reference implementation at the current `main` tip. It prevents previously resolved findings from being reimplemented and prevents software evidence from being presented as physical or formal conformance evidence.

## Current execution model

The target is **bare-metal HAL firmware**, not FreeRTOS. `Core/Inc/stm32f7xx_hal_conf.h` defines `USE_RTOS 0U`; there are no task creation calls, scheduler startup, CMSIS-RTOS layer, or `taskENTER_CRITICAL()` calls. CANopenNode uses a cooperative mainline plus interrupt contexts. OD/CAN-send protection is implemented with PRIMASK-based `__disable_irq()` / `__set_PRIMASK()` critical sections in `CO_driver_target.h`.

The 1 ms TIM7 path is statically configured at 108 MHz timer input, 108000 timer divisor, and one timer tick. CAN1 interrupts use priority 5 and TIM7 uses priority 6 under `NVIC_PRIORITYGROUP_4`. The current CAN bit timing is Prescaler 6, BS1 14TQ, BS2 3TQ at a 54 MHz APB1 clock, which gives exactly 500 kbit/s and an 83.33% nominal sample point.

## Finding status

| Audit item | Current status | Reconciliation |
|---|---|---|
| FINDING-01: NVIC priority and OD concurrency | **Resolved in source; hardware timing still pending** | Explicit priority grouping, CAN priority 5, TIM7 priority 6, and OD locking are present. This does not replace silicon measurement of ISR latency, jitter, or bus-load margin. |
| FINDING-02: 88.89% sample point and hardcoded bitrate | **Resolved in source** | The production CAN1 configuration uses 14TQ/3TQ and the standalone facade contains the supported bitrate table. The review describes the prior state, not the current state. |
| FINDING-09: bus-off recovery | **Resolved in source; physical qualification pending** | The bounded recovery state machine exists. Current default maximum attempts is 3, not the review’s historical value of 5. The configured value is intentional and must be validated against product FTTI requirements. |
| FINDING-10: Flash persistence and linker overlap | **Resolved in source; power-loss/endurance pending** | The dual-slot CRC/sequence backend and reserved linker region exist. Brownout interruption, endurance, and exact-board behavior require hardware testing. |
| FINDING-11: failed communication-reset fail-safe | **Resolved in source** | The runtime safe-fault latch stops the timer, disables CAN, forces safe application behavior, and records diagnostics. Silicon fault-injection evidence remains external. |
| FINDING-12: dynamic acceptance filters | **Resolved in source; hardware bus-load pending** | Active OD-derived COB-IDs and configured CiA 302 peer heartbeat are included within the bounded filter list. Filter capacity and noisy-bus behavior require HIL testing. |
| FINDING-13: standalone CAN facade timeout/unsupported target | **Resolved in source** | The facade has a bitrate table, bounded timeout polling, and `-ENOTSUP` for unsupported target builds. |
| FINDING-05: CAN error/FIFO-overrun callback | **Resolved in source; physical disturbance pending** | HAL CAN errors are decoded into CANopen error state and diagnostics. Actual overrun thresholds and recovery traces require hardware load testing. |
| FINDING-06: dual-rate IWDG | **Implemented as opt-in; production qualification pending** | The watchdog requires progress from both TIM7 and mainline, captures reset cause, and has startup grace. It is disabled by default until LSI and reset behavior are validated on the board. The current default grace is 100 ms and timeout is 200 ms; the review’s 500 ms statement is not current source fact. |
| FINDING-07: work in the 1 ms ISR | **Architecture bounded; timing claim unproven** | The ISR contract prohibits blocking drivers, Flash, and printf. O(1) application/Object-Dictionary reads are a board-integration requirement. No source or host test proves a universal less-than-10-us ISR duration; DWT/GPIO measurement is required. |
| FINDING-08: CiA 402 state-machine separation | **Software coverage present; conformance not claimed** | Host state-machine tests and mutually exclusive profile builds exist. Physical PWM/encoder control remains application-specific and is not implemented by the reference project. |

## Missing components and disposition

| Missing component | Disposition |
|---|---|
| Board-level Flash power-loss/endurance campaign | Pending external hardware evidence; the repository already contains the qualification procedure and fail-closed evidence structure. |
| Physical CAN transceiver/HIL campaign | Pending external hardware evidence; the repository already contains a CiA 401 campaign plan and pending initializer. |
| CiA 402 PWM/encoder driver | Application-specific extension, not inferable from the reference board abstraction. No implementation should be fabricated without motor, inverter, encoder, safety, and pinout requirements. |
| Embedded UDS/ISO-TP server | Still absent; current Python contract is host-only. Requires a product diagnostic protocol decision before implementation. |
| Embedded NMEA 2000 router | Still absent; current Python gateway is host-only. Requires CAN2 hardware, 29-bit filtering, address-claim, PGN, and coexistence requirements. |
| CiA 304 SRDO | Still absent. Requires an explicit safety architecture, redundant-data timing policy, OD generation, and independent safety review; enabling a configuration macro alone would not establish SIL capability. |

## Ordered next actions

1. Preserve the current software fixes and add an opt-in measurement path for TIM7 ISR duration, period jitter, mainline duration, and CAN RX callback load.
2. Run the software contract, sanitizer, coverage, and production ARM builds with the instrumentation path disabled by default.
3. Execute the board HIL campaign using a real transceiver and independent CAN equipment, recording firmware SHA, clock/bit timing, bus load, IRQ occupancy, and FIFO counters.
4. Execute Flash interruption/endurance and watchdog reset-cause campaigns on the target board.
5. Make a product decision before implementing embedded UDS, NMEA 2000, CiA 304 SRDO, or CiA 402 motor hardware drivers.
6. Do not assign a production or conformance status until the external evidence package contains reviewed hardware, EMC, manufacturing, security, and formal conformance records.

## Release claim boundary

The current repository can be described as a validated **STM32F767 bare-metal CANopenNode reference implementation** with software-tested recovery, persistence, filter, watchdog, and profile contracts. It must not be described as a FreeRTOS system, a complete embedded UDS/NMEA 2000 gateway, a CiA 304 safety implementation, a motor-ready CiA 402 product, or a formally certified SIL/PL device without the missing product and external evidence.
