# Issue #19 — ECUReset Recovery Review

**Repository:** `mahdi-benhassen/stm32_uds_iso_tp`
**Issue:** [#19 — ECUReset (0x11) service error after reset][1]
**Target:** STM32C092, FDCAN operated as Classical CAN, external transceiver, Keil MDK / Arm Compiler 6
**Review revision:** local post-review changes, not yet HIL-verified

## Executive status

Issue #19 is a **platform and application reset-handoff issue**, not a reason to add a fixed delay to the generic UDS library. The maintained implementation now keeps ECUReset reset ownership in the C092 application: the platform callback arms a pending reset, the application main loop polls it, and the current C092 profile waits 50 ms before invoking the MCU reset. After the final positive ECUReset frame completes, the endpoint ignores further diagnostic requests until reset. This handoff value is not a measured reset-to-diagnostic-ready interval.

The software changes are validated by host and ARM GCC checks. **The issue cannot be marked hardware-fixed yet** because no STM32C092 board, CAN analyzer trace, Keil MDK/Arm Compiler 6 build, reset-cause capture, or measured reset-to-diagnostic-ready interval was available in this environment.

## Root-cause analysis

The attached reporter project and the Ecu_test solution are useful behavioral references but cannot prove the physical stop point without a trace. The Ecu_test solution keeps the CAN send driver unchanged and arms a platform-owned pending reset from the callback, then polls it from the C092 application main loop. The maintained C092 transport completion contract remains available for transport bookkeeping; it is not used to execute the platform reset policy.

The source-level defects and acceptance risks identified are as follows.

| Finding | Evidence | Consequence |
|---|---|---|
| Queue acceptance is not physical TX completion | The generic endpoint previously treated a successful `send_frame()` with no `tx_complete` callback as immediately complete. | ECUReset could execute without a transport-defined final-frame completion boundary. |
| ECUReset requires an explicit completion contract | The maintained C092 application supplies `uds_c092_fdcan_tx_complete()`, which waits for a matching stored TX event. A reset-capable generic endpoint previously did not enforce that requirement. | An application could configure reset without the information needed to safely execute it. |
| Readiness gating could discard a valid post-start frame | The first implementation dropped a valid frame whenever the higher-level diagnostic trace was not yet `READY`. | A request received after FDCAN start could be lost even though the bounded RX handoff was usable. The corrected application retains it in the mailbox and records `RX_ACCEPTED`. |
| RX could arrive before endpoint initialization | The reporter archive starts FDCAN and enables RX notification before calling transport/endpoint initialization; `s_initialized` is false in the callback during that window. | The safety guard must remain. The correction initializes transport/ISO-TP/UDS before notification and `HAL_FDCAN_Start()`, and records `RX_REJECTED_NOT_INITIALIZED` if an invalid integration still triggers the callback early. |
| The reporter project’s exact runtime stop point is unproven | The supplied project has no physical trace in the repository showing whether the next frame was received, parsed, responded to, queued, or transmitted. | A fixed delay would conceal the failing stage rather than identify it. |

The corrected conclusion is therefore: **ECUReset execution is application-owned and deferred through a platform pending-reset poll; the endpoint is silent after the final positive ECUReset response until reset; C092 readiness and bounded mailbox handling remain explicit; and the exact hardware failure stage remains unconfirmed until instrumentation is run on the board.**

## Corrected architecture

The generic library remains independent of STM32 HAL, CMSIS, registers, delays, heap allocation, and board startup. The C092 layer owns readiness, startup instrumentation, FDCAN callback wiring, and platform reset execution.

```text
11 xx
  -> generic UDS validates reset and prepares 51 xx
  -> ISO-TP prepares the response
  -> C092 transport accepts the frame
  -> response path completes
  -> application-owned platform reset callback arms a pending reset timestamp
  -> ECU ignores new diagnostic requests during the handoff window
  -> platform reset poll reaches 50 ms and calls NVIC_SystemReset()
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
| `FDCAN_INIT_START` | The board-owned FDCAN initialization call began. |
| `FDCAN_INIT_DONE` | `MX_FDCAN1_Init()` / `HAL_FDCAN_Init()` completed. |
| `FDCAN_FILTER_DONE` | Exact physical/functional filters and the global filter completed. |
| `FDCAN_NOTIFICATION_DONE` | Required RX notification was enabled successfully. |
| `FDCAN_START_DONE` | `HAL_FDCAN_Start()` completed successfully. |
| `ISOTP_INIT_DONE` | Transport and endpoint ISO-TP state were initialized to clean state. |
| `UDS_INIT_DONE` | UDS server state was initialized to clean state. |
| `DIAGNOSTIC_READY` | All required stages are complete; the application may report readiness. Valid frames received after FDCAN start are captured in the bounded mailbox even before this mark. |

The generic library intentionally does not define a reset delay. The C092 platform currently defines a 50 ms post-response handoff timer in its own pending-reset object; this is application policy, not P2 timing and not a measured reset-to-ready interval. During that window, the endpoint ignores diagnostic requests, consistent with ISO 14229-1's recommendation that an ECU not accept requests or send responses after an ECUReset positive response until reset completes. Separately, once FDCAN is started and RX notification is active, a valid frame is accepted into the single bounded mailbox even if the diagnostic trace is still `BOOTING`; it is not rejected merely because the higher-level READY mark has not yet been recorded. The tester should still wait for the project-defined readiness indication, and the actual reset-to-ready time must be measured on the selected board.

## ECUReset ordering

The endpoint event order remains:

```text
REQUESTED -> RESPONSE_READY -> TX_SUBMITTED -> TX_COMPLETE -> EXECUTED
```

`NVIC_SystemReset()` is reachable only from the application-owned C092 reset poll after the platform callback has armed a pending reset. The CAN send driver remains unchanged. The handoff timer must not be confused with physical CAN transmission completion or diagnostic readiness.

## Validation performed

The following software evidence is available:

| Check | Result |
|---|---|
| Standalone host contracts | **13/13 passed** after the readiness/mailbox correction. |
| ASan/UBSan host suite | **13/13 passed** after the readiness/mailbox correction; no hardware claim follows from it. |
| Reset recovery contract | 100 reset cycles with 10 normal requests per cycle, totaling 1,000 post-reset requests in the host model. |
| Service sequence contract | Post-reset `0x10`, `0x22` with multi-frame response, `0x3E`, invalid request recovery, and subsequent valid request. |
| Endpoint safety contract | ECUReset configuration without `tx_complete` is rejected. |
| C092 diagnostic contract | Readiness ordering, pre-initialization rejection, post-start mailbox acceptance/full handling, fault state, reset reinitialization, and lifecycle counters. |
| C092 portability | **Passed** against the supplied reporter HAL headers after the final source changes. |

These are host, static, and cross-compile contracts. They are not physical CAN evidence.

## HIL procedure still required

On a selected STM32C092 board and CAN transceiver, execute the following with timestamped analyzer capture:

```text
Power-on -> 10 01 -> 50 01
11 01 -> 51 01 -> ECU silent -> delayed MCU reset
wait for measured DIAGNOSTIC_READY after reboot
10 01 -> 50 01
22 DID -> 62 DID...
```

Repeat at least 100 reset/reconnect cycles, then 1,000 post-reset transactions. Run the exact startup-race experiment at 10, 20, 50, 100, and 200 ms, plus “wait until diagnostic-ready.” Record the first RX, ISO-TP, UDS, response, TX submission, and TX completion counters together with analyzer timestamps. Do not conclude that the 50 ms handoff is the complete solution unless the measured readiness timeline and analyzer trace support it.

## Remaining limitations

The current revision does not claim a measured `ECU_RESET_TO_DIAGNOSTIC_READY_TIME`, 100-cycle hardware result, 1,000-transaction hardware result, Keil link/map result, or physical reset-cause result. The reporter’s broad filter and generated startup code remain project-owned; the maintained README now requires narrow configured IDs, correct HAL DLC conversion, RX notification, complete FDCAN startup, and explicit diagnostic readiness integration.

[1]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/19 "GitHub Issue #19"
