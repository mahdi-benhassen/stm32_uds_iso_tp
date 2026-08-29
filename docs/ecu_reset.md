# UDS ECUReset lifecycle

## Scope

This document defines the repository’s ECUReset (`0x11`) behavior for a standalone UDS/ISO-TP endpoint. The generic library remains independent of STM32 HAL, CMSIS, delays, interrupts, and reset registers. The platform/application owns the physical reset operation.

## Required ordering

The positive response is prepared and sent before the platform reset callback is invoked:

```text
Tester                         ECU
  |                             |
  |------ 11 xx -------------->|
  |                             | validate and authorize
  |<----- 51 xx ----------------|
  |                             |
  |                             | transport TX completion
  |                             | optional non-blocking guard
  |                             | mainline reset poll
  |                             | platform reset callback
  |                             | NVIC_SystemReset()
  |                             | reboot
  |                             | HAL / clock / GPIO / FDCAN init
  |                             | filters / notifications / FDCAN start
  |                             | ISO-TP / UDS init
  |                             | DIAGNOSTIC_READY
  |------ 10 01 --------------->|
  |<----- 50 01 ----------------|
```

`HAL_FDCAN_AddMessageToTxFifoQ()` returning `HAL_OK` means that the frame was accepted by the controller queue. It is not automatically the completion boundary. The endpoint waits for the configured `tx_complete` contract. The C092 adapter satisfies this contract through correlated FDCAN TX Event FIFO records.

## Reset state machine

The endpoint uses these states:

| State | Incoming diagnostic request | Transition |
|---|---|---|
| `UDS_RESET_STATE_IDLE` | Process normally; a valid `0x11` is accepted | Valid ECUReset request enters `WAIT_TX_COMPLETE` |
| `UDS_RESET_STATE_WAIT_TX_COMPLETE` | Do not dispatch | Successful final response completion enters `WAIT_GUARD` |
| `UDS_RESET_STATE_WAIT_GUARD` | Do not dispatch | Mainline tick enters `EXECUTE` when the configured guard expires |
| `UDS_RESET_STATE_EXECUTE` | Do not dispatch | Platform callback is invoked once |
| `UDS_RESET_STATE_FAULT` | Do not dispatch; further ECUReset requests receive a conditions-not-correct response when they reach UDS | Sticky failure state if an executor unexpectedly returns |
| C092 `BOOTING` | Existing bounded startup handoff rules apply | Application progresses through initialization stages |
| C092 `DIAGNOSTIC_READY` | Process normally | Normal diagnostic operation |

The guard is configured with `UdsIsoTpEndpointConfig.reset_guard_ms` and defaults to zero. A non-zero value is a platform-owned, non-blocking policy that must be justified by measurement; it is not a substitute for TX completion and is not a generic `P2_server_max` delay. The C092 profile exposes `UDS_C092_RESET_GUARD_MS`, also defaulting to zero.

## Requests during reset transition

After ECUReset is accepted, the endpoint does not dispatch subsequent requests or generate additional diagnostic responses while the reset remains pending. This includes another ECUReset, DiagnosticSessionControl, ReadDataByIdentifier, TesterPresent, and SecurityAccess. The ECUReset positive response itself is not dropped: it remains in the ISO-TP TX path until the configured completion event is observed.

A TX error aborts the in-flight transaction, clears the endpoint’s reset markers, and does not execute the physical reset. A second ECUReset cannot overwrite the first reset type, pending state, completion state, or callback context.

## Platform ownership and post-reset readiness

The generic library exposes `ecu_reset_execute` but does not call `NVIC_SystemReset()` itself. The STM32C092 application owns that callback. After reset, the generated startup path must repeat HAL initialization, clock and GPIO setup, FDCAN initialization, standard/global filters, transport initialization, endpoint initialization, notification activation, and FDCAN start. The application then marks `DIAGNOSTIC_READY` only after ISO-TP and UDS initialization has succeeded.

The C092 ISR only captures accepted frames into the existing bounded mailbox. It does not run UDS dispatch, reset handling, waits, or printing. Frames received before endpoint initialization are rejected and counted; frames received after FDCAN notification is active but before the diagnostic READY marker are retained in the bounded mailbox according to the existing application contract. The tester should wait for the project’s diagnostic-ready indication before sending the first post-reset request.

## Timing and validation

The relevant measured interval is:

```text
RESET_TO_DIAGNOSTIC_READY = diagnostic_ready_timestamp - reset_entry_timestamp
```

The repository has no physical STM32C092 measurement. Host and mock-hardware tests cannot prove board reset duration, FDCAN startup timing, transceiver behavior, interrupt delivery, or electrical CAN completion.

The required HIL sequence is:

```text
10 01 -> 50 01
11 01 -> 51 01 -> confirmed TX completion -> MCU reset
wait for DIAGNOSTIC_READY
10 01 -> 50 01
```

The physical campaign must record request/response timestamps, TX completion, reset entry/cause, FDCAN ready, ISO-TP/UDS ready, diagnostic ready, and first post-reset request/response. It must repeat at least 100 resets and 1,000 post-reset transactions, including race trials where the next `10 01` is sent immediately and at measured offsets. Issue #26 must not be declared physically resolved from compilation or host tests alone.

## Software validation status

The lifecycle regression models delayed TX completion, a non-blocking guard, duplicate-reset suppression, clean endpoint reinitialization, and immediate post-reboot `10 01 -> 50 01` transactions. The normal and sanitizer CTest suites, freestanding ARM GCC compile check, static checks, and deterministic C092 mock campaign are software evidence only. Physical HIL remains required.
