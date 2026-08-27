# ECUReset Timing Contract

This document defines the software ordering for UDS ECUReset on the STM32C092 FDCAN Classical CAN profile. It does not invent a fixed delay and does not claim a measured hardware interval until a board and analyzer campaign is completed.

```text
Tester                         ECU / C092 platform
  |                                  |
  |-------- 11 xx ----------------->|
  |                                  | UDS validation
  |                                  | positive response 51 xx prepared
  |                                  | ISO-TP response submitted
  |<-------- 51 xx ------------------|
  |                                  | transport-defined TX completion
  |                                  | WAIT_RESET_GUARD (non-blocking)
  |                                  | mainline reset poll
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

The response must be generated before reset execution. Queue acceptance is not automatically physical completion. A C092 implementation must provide a transport completion callback; the maintained adapter uses a matching stored TX Event FIFO record. After the completion callback confirms the final response, the generic library enters a non-blocking reset-guard state and executes the platform callback from the mainline tick path. The default guard is zero milliseconds, so the reset occurs on the next eligible tick unless the application configures a measured guard interval. If the completion callback reports an error, reset execution is not performed and the endpoint clears the failed in-flight state.

After `NVIC_SystemReset()`, all board-owned initialization must run again. The platform should mark `UDS_C092_DIAG_READY` only after HAL, clock, GPIO, FDCAN initialization, filters, required notifications, FDCAN start, transport initialization, and UDS endpoint initialization have succeeded.

After FDCAN start and RX-notification activation, valid frames are captured in the bounded application mailbox even if the diagnostic trace is still `BOOTING`; they are not discarded merely because the higher-level READY mark has not yet been recorded. If the mailbox is occupied, the additional frame is counted as `RX_MAILBOX_FULL`. Frames arriving before FDCAN can receive them, or after a fatal initialization failure, cannot be recovered in software. The tester should still wait for the platform’s diagnostic-ready indication so the reset-to-ready interval can be measured and reported.

## Measurement record

While ECUReset is pending, the endpoint does not dispatch subsequent diagnostic requests; this prevents a second request from racing with an already accepted reset operation. The tester must wait for the platform’s post-reset diagnostic-ready indication before sending the next request. This is a readiness/boot contract, not a P2_server_max delay.

The required hardware measurement is:

```text
ECU_RESET_TO_DIAGNOSTIC_READY_TIME = DIAGNOSTIC_READY_timestamp - MCU_reset_timestamp
```

The repository currently has no physical measurement. The board campaign must record the MCU, firmware SHA, compiler version, CAN nominal bit rate, transceiver, analyzer, timestamp source, and raw trace. The startup-race experiment may compare measured guard/readiness strategies, including wait-until-ready timing. A fixed delay must not be adopted merely because one delay appears to work.

## Required acceptance sequence

The minimum physical sequence is:

```text
Power-on:  10 01 -> 50 01
Reset:     11 01 -> 51 01 -> MCU reset
Recovery:  wait until DIAGNOSTIC_READY -> 10 01 -> 50 01
Extended:  22 DID -> 62 DID
```

Run at least 100 reset/reconnect cycles and then 1,000 post-reset diagnostic transactions. Capture first RX, ISO-TP acceptance, UDS request, response generation, TX submission, and TX completion evidence for every failure.
