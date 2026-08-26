# Issue #19 — ECUReset Recovery Review

**Repository:** `mahdi-benhassen/stm32_uds_iso_tp`
**Issue:** [#19 — ECUReset (0x11) service error after reset][1]
**Target:** STM32C092, FDCAN operated as Classical CAN, external transceiver, Keil MDK / Arm Compiler 6
**Review revision:** local post-review changes, not yet HIL-verified

## Executive status

Issue #19 is a **platform and transport lifecycle issue**, not a reason to add a fixed 50 ms delay to the generic UDS library. The implementation now provides an explicit C092 diagnostic readiness state, optional bounded boot timestamps and counters, deterministic dropping of frames received before readiness, stricter ECUReset endpoint initialization, and host coverage for repeated reset/reconnect behavior.

The software changes are validated by host and ARM GCC checks. **The issue cannot be marked hardware-fixed yet** because no STM32C092 board, CAN analyzer trace, Keil MDK/Arm Compiler 6 build, reset-cause capture, or measured reset-to-diagnostic-ready interval was available in this environment.

## Root-cause analysis

The attached reporter project is useful evidence but cannot prove the physical stop point without a trace. It configures Classic CAN, normal mode, TX FIFO operation, a broad standard-ID range filter, RX FIFO0 notification, and `FDCAN_NO_TX_EVENTS` in its sample TX header. Therefore the earlier C092 TX Event FIFO hypothesis from the separate repeated-request issue must not be copied blindly to Issue #19.

The source-level defects and acceptance risks identified are as follows.

| Finding | Evidence | Consequence |
|---|---|---|
| Queue acceptance is not physical TX completion | The generic endpoint previously treated a successful `send_frame()` with no `tx_complete` callback as immediately complete. | ECUReset could execute without a transport-defined final-frame completion boundary. |
| ECUReset requires an explicit completion contract | The maintained C092 application supplies `uds_c092_fdcan_tx_complete()`, which waits for a matching stored TX event. A reset-capable generic endpoint previously did not enforce that requirement. | An application could configure reset without the information needed to safely execute it. |
| Post-reset readiness was implicit | The C092 application had no platform-owned BOOTING/READY/FAULT state or bounded trace showing when HAL, FDCAN, filters, notifications, ISO-TP, and UDS were ready. | A tester request arriving during startup could be lost or be delivered to a partially initialized path without a defined policy. |
| The reporter project’s exact runtime stop point is unproven | The supplied project has no physical trace in the repository showing whether the next frame was received, parsed, responded to, queued, or transmitted. | A fixed delay would conceal the failing stage rather than identify it. |

The corrected conclusion is therefore: **the repository had an unsafe generic completion fallback and no explicit C092 readiness contract; the reporter’s exact hardware failure stage remains unconfirmed until instrumentation is run on the board.**

## Corrected architecture

The generic library remains independent of STM32 HAL, CMSIS, registers, delays, heap allocation, and board startup. The C092 layer owns readiness, startup instrumentation, FDCAN callback wiring, and platform reset execution.

```text
11 xx
  -> generic UDS validates reset and prepares 51 xx
  -> ISO-TP prepares the response
  -> C092 transport accepts the frame
  -> transport-specific completion callback proves the defined completion boundary
  -> endpoint calls uds_server_complete_reset()
  -> application-owned platform reset callback calls NVIC_SystemReset()
  -> MCU starts from reset vector
  -> HAL / clock / GPIO / FDCAN / filter / notification / start / UDS init
  -> DIAGNOSTIC_READY
  -> next tester request is accepted
```

For the maintained C092 adapter, the completion boundary is a matching stored TX Event FIFO record. `HAL_FDCAN_AddMessageToTxFifoQ() == HAL_OK` is only controller queue acceptance. The adapter keeps ISR and mainline TX-event FIFO draining serialized, rejects idle false-completion, and exposes a one-shot TX-error callback for endpoint recovery.

For any application configuring `UdsCallbacks.ecu_reset`, `uds_isotp_endpoint_init()` now requires a non-NULL `tx_complete` callback. A legacy non-reset transport may continue to use queue-acceptance semantics for ordinary services, but it cannot silently claim that semantics for ECUReset.

## Startup and readiness timeline

The platform-owned `UdsC092DiagnosticTrace` records optional first-event timestamps when `UDS_C092_DIAGNOSTIC_BOOT_TRACE=1` and always maintains bounded counters. The required readiness stages are:

| Stage | Meaning |
|---|---|
| `HAL_INIT_DONE` | `HAL_Init()` returned and system reset peripherals are available. |
| `CLOCK_READY` | Clock configuration completed successfully. |
| `GPIO_READY` | GPIO and transceiver-control setup completed. |
| `FDCAN_INIT_DONE` | `MX_FDCAN1_Init()` / `HAL_FDCAN_Init()` completed. |
| `FDCAN_FILTER_READY` | Standard physical/functional filters and global filter completed. |
| `FDCAN_NOTIFICATION_READY` | Required RX notification was enabled successfully. |
| `FDCAN_STARTED` | `HAL_FDCAN_Start()` completed successfully. |
| `UDS_INIT_DONE` | Transport and endpoint state were initialized to clean ISO-TP/UDS state. |
| `DIAGNOSTIC_READY` | All required stages are complete; the application may accept diagnostic frames. |

The implementation intentionally does not define a magic 10, 20, or 50 ms delay. A frame received while `BOOTING` is counted and dropped rather than buffered indefinitely. The tester must wait for a project-defined readiness indication, and the actual reset-to-ready time must be measured on the selected board.

## ECUReset ordering

The endpoint event order remains:

```text
REQUESTED -> RESPONSE_READY -> TX_SUBMITTED -> TX_COMPLETE -> EXECUTED
```

`NVIC_SystemReset()` is reachable only from the application-owned C092 reset executor after the endpoint has observed the configured transport completion callback. If the transport reports TX error, the endpoint clears in-flight protocol state and does not execute the reset completion callback.

## Validation performed

The following software evidence is available:

| Check | Result |
|---|---|
| Standalone host contracts | **12/12 passed** after Issue #19 additions. |
| ASan/UBSan host suite | Planned and required in the final validation run; no hardware claim follows from it. |
| Reset recovery contract | 100 reset cycles with 10 normal requests per cycle, totaling 1,000 post-reset requests in the host model. |
| Service sequence contract | Post-reset `0x10`, `0x22` with multi-frame response, `0x3E`, invalid request recovery, and subsequent valid request. |
| Endpoint safety contract | ECUReset configuration without `tx_complete` is rejected. |
| C092 diagnostic contract | Readiness ordering, boot-time drop, fault state, reset reinitialization, and lifecycle counters. |
| C092 portability | Must be rerun against the supplied reporter HAL headers after final source changes. |

These are host, static, and cross-compile contracts. They are not physical CAN evidence.

## HIL procedure still required

On a selected STM32C092 board and CAN transceiver, execute the following with timestamped analyzer capture:

```text
Power-on -> 10 01 -> 50 01
11 01 -> 51 01 -> MCU reset
wait for measured DIAGNOSTIC_READY
10 01 -> 50 01
22 DID -> 62 DID...
```

Repeat at least 100 reset/reconnect cycles, then 1,000 post-reset transactions. Run the exact startup-race experiment at 10, 20, 50, 100, and 200 ms, plus “wait until diagnostic-ready.” Record the first RX, ISO-TP, UDS, response, TX submission, and TX completion counters together with analyzer timestamps. Do not conclude that 50 ms is the solution unless the measured readiness timeline supports it.

## Remaining limitations

The current revision does not claim a measured `ECU_RESET_TO_DIAGNOSTIC_READY_TIME`, 100-cycle hardware result, 1,000-transaction hardware result, Keil link/map result, or physical reset-cause result. The reporter’s broad filter and generated startup code remain project-owned; the maintained README now requires narrow configured IDs, correct HAL DLC conversion, RX notification, complete FDCAN startup, and explicit diagnostic readiness integration.

[1]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/19 "GitHub Issue #19"
