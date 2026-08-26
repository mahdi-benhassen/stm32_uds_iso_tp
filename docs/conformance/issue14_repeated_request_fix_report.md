# Issue #14 — reusable STM32C092/FDCAN request-response lifecycle

## Root cause

The reporter’s supplied STM32C092 Issue #14 project contains a successful Keil/Arm Compiler 6 build artifact, but its generated startup code activates only `FDCAN_IT_RX_FIFO0_NEW_MESSAGE`. It does not activate `FDCAN_IT_TX_EVT_FIFO_NEW_DATA` or provide `HAL_FDCAN_TxEventFifoCallback()`. The maintained C092 transport submits responses with `FDCAN_STORE_TX_EVENTS` and tracks the accepted frame until a matching TX Event FIFO record arrives. Without a callback or mainline drain, the first response leaves the adapter’s hardware-specific `tx_pending` state active, and later endpoint responses cannot be submitted.

This explains the reporter’s symptom across `0x10`, `0x11`, and other services: it is a shared FDCAN completion/reusability failure, not a service-specific UDS dispatch error. The reporter project’s FDCAN configuration uses `FDCAN_TX_FIFO_OPERATION`, so the adapter must not use bxCAN mailbox logic.

## Fix

The C092 transport now exposes:

```c
void uds_c092_fdcan_poll_tx_events(UdsC092FdcanTransport *transport);
```

The application’s mainline process calls this function on every pass before querying `uds_c092_fdcan_tx_complete()` and notifying the generic endpoint. It drains stored TX Event FIFO entries and remains safe when the optional TX-event interrupt callback also drains the FIFO. Matching is still protected by a rotating message marker, and FIFO loss/full or aborted events remain terminal errors for the in-flight transfer.

The C092 startup guide now shows both valid paths:

1. Preferred interrupt path: activate `FDCAN_IT_TX_EVT_FIFO_NEW_DATA` and forward `HAL_FDCAN_TxEventFifoCallback()` to `uds_c092_fdcan_on_tx_event()`.
2. Mainline fallback: retain `FDCAN_STORE_TX_EVENTS` and call `uds_c092_fdcan_poll_tx_events()` from `uds_c092_app_process()`.

The fallback does not use a delay, busy loop, peripheral reset, or bxCAN `tx_pending` coupling.

## Regression coverage

A new vendor-free C092 HAL shim test covers successful TX-event completion, empty polling before completion, rotating markers, rejection of a stale event from an earlier frame, aborted event failure, enqueue failure, and recovery for a later send. The existing bxCAN adapter test now submits 1000 consecutive valid TesterPresent requests through one initialized endpoint without power cycling or reinitialization.

The normal host build and CTest pass all seven contracts, including the new C092 TX-event contract. The sanitizer build and target-specific checks are part of the final campaign.

## ECUReset preservation

The generic endpoint still owns protocol-level `tx_pending` and `tx_in_flight`. For `11 01`, it generates `51 01`, submits the response through the transport, and completes the reset only after the transport completion callback reports the accepted frame complete. The C092 adapter’s TX Event FIFO state is retained for this purpose. The Issue #13 bxCAN transport has no transport-level `tx_pending` boolean and remains independent of ECUReset.

## Validation boundary

The supplied reporter project was inspected and its generated FDCAN/mainline configuration was compared with the maintained adapter. The maintained sources and a temporary patched copy of the reporter project pass strict ARM GCC syntax validation. The sandbox does not contain Keil µVision/ArmClang, an STM32C092 board, a CAN analyzer, or a debug probe, so no Keil rebuild, firmware flashing, or physical 1000-request HIL result is claimed.

Issue #14 remains open for reporter confirmation and physical C092 validation. The required HIL campaign must power the board once, send at least 1000 mixed valid/invalid UDS requests without reset, and capture the absence of permanent RX/TX busy state and the correct ECUReset behavior separately.

## References

[1]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/14 "Issue #14 — repeated UDS request failure"
[2]: https://github.com/github/user-attachments/files/31442898/STM32C092_UDS-error.zip "Reporter-supplied STM32C092 Issue #14 project archive"
[3]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/13 "Issue #13 — C092 transport/ECUReset integration"
