# ECUReset Timing Contract

This document defines the software ordering for UDS ECUReset on the STM32C092 FDCAN Classical CAN profile. ECUReset arms a pending flag; the UDS service and CAN path do not execute the MCU reset. The application mainline performs the platform reset handoff. The current C092 example uses a 5 ms application-owned handoff before `NVIC_SystemReset()`; this value is not a measured reset-to-diagnostic-ready interval.

```text
Tester                         ECU / C092 platform
  |                                  |
  |-------- 11 xx ----------------->|
  |                                  | UDS validation
  |                                  | positive response 51 xx prepared
  |                                  | ISO-TP response submitted
  |<-------- 51 xx ------------------|
  |                                  | reset_pending = true
  |                                  | return to application mainline
  |                                  | reset_ready after response path
  |                                  | C092 HAL_Delay(5 ms)
  |                                  | platform reset executor
  |                                  | NVIC_SystemReset()
  |                                  |-------------------->
  |                                  | HAL_Init()
  |                                  | clock ready
  |                                  | GPIO ready
  |                                  | FDCAN init
  |                                  | filters ready
  |                                  | RX notification ready
  |                                  | FDCAN started
  |                                  | ISO-TP/UDS initialized
  |                                  | DIAGNOSTIC_READY
  |-------- 10 01 ------------------>| only after readiness
  |<-------- 50 01 ------------------|
```

## Software invariants

The response must be generated before reset execution. ECUReset is only marked ready after the response path has completed; reset execution itself occurs from the application mainline tick. The generic library does not call `HAL_Delay()` and contains no STM32-specific reset code. The C092 platform callback owns its 5 ms handoff and then calls `NVIC_SystemReset()`.

After `NVIC_SystemReset()`, all board-owned initialization must run again. The platform should mark `UDS_C092_DIAG_READY` only after HAL, clock, GPIO, FDCAN initialization, filters, required notifications, FDCAN start, transport initialization, and UDS endpoint initialization have succeeded.

After FDCAN start and RX-notification activation, valid frames are captured in the bounded application mailbox even if the diagnostic trace is still `BOOTING`; they are not discarded merely because the higher-level READY mark has not yet been recorded. If the mailbox is occupied, the additional frame is counted as `RX_MAILBOX_FULL`. Frames arriving before FDCAN can receive them, or after a fatal initialization failure, cannot be recovered in software. The tester should still wait for the platform’s diagnostic-ready indication so the reset-to-ready interval can be measured and reported.

## Measurement record

The required hardware measurement is:

```text
ECU_RESET_TO_DIAGNOSTIC_READY_TIME = DIAGNOSTIC_READY_timestamp - MCU_reset_timestamp
```

The repository currently has no physical measurement. The board campaign must record the MCU, firmware SHA, compiler version, CAN nominal bit rate, transceiver, analyzer, timestamp source, and raw trace. The startup-race experiment must compare immediate request, the C092 5 ms handoff, measured reset-to-ready timing, and wait-until-ready timing. The 5 ms pre-reset handoff must not be confused with diagnostic readiness and must not be treated as physical proof until measured on the target board.

## Required acceptance sequence

The minimum physical sequence is:

```text
Power-on:  10 01 -> 50 01
Reset:     11 01 -> 51 01 -> MCU reset
Recovery:  wait until DIAGNOSTIC_READY -> 10 01 -> 50 01
Extended:  22 DID -> 62 DID
```

Run at least 100 reset/reconnect cycles and then 1,000 post-reset diagnostic transactions. Capture first RX, ISO-TP acceptance, UDS request, response generation, TX submission, and TX completion evidence for every failure.
