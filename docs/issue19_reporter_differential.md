# Issue #19: Reporter-project differential analysis

**Repository:** `mahdi-benhassen/stm32_uds_iso_tp`
**Reporter archive:** `STM32C092_UDS11 01 → 51 01 → reset → 10 01 → 50 01.zip`
**Analysis basis:** reporter archive downloaded from the latest Issue #19 comment, repository `main` at the current validated implementation, the archive’s Keil map, and the two attached PCAN-View captures.

## Executive conclusion

The latest reporter archive does contain the newer C092 adapter and newer generic endpoint. The core C092 adapter sources are byte-for-byte identical to the maintained repository sources apart from include-path spelling. The remaining defect is in the **generated-project integration and test sequencing**, not another missing ECUReset branch in the generic library.

The strongest directly observed fact is that the tester sends `10 01` only approximately **1.3–1.6 ms after** receiving `51 01`. The generated project has no externally observable `DIAGNOSTIC_READY` boundary, and the application intentionally cannot dispatch traffic while the MCU is between `NVIC_SystemReset()` and completion of the normal `main()` startup path. Consequently, the capture demonstrates a request sent during the post-reset startup window, followed by no response; it does not demonstrate that the reset response was lost or that the post-reset FDCAN RX path is permanently dead.

The archive also still violates the maintained integration contract: its generated `main.c` does not attach or mark the bounded readiness diagnostics, uses a permissive `0x7E0..0x7FF` filter, does not install the optional TX-event callback, passes the RX header’s raw `DataLength`, and hard-codes the FD/BRS arguments to zero. Some of these are real integration defects or observability gaps, but not all are causal for the captured Classic CAN `0x7E0` request. In particular, the C092 HAL defines Classic DLC values `0..8`, so the raw cast is numerically valid for this Classic-only capture; the conversion helper is nevertheless required for a portable and future FD-safe integration.

## File-by-file comparison

| Path / layer | Reporter archive | Maintained repository | Differential result |
|---|---|---|---|
| `library/src/endpoint.c` | Contains the TX-completion-gated reset state machine. | Same implementation. | **Not the remaining difference.** `51 01` is held as an in-flight response until transport completion. |
| `library/src/uds.c` | `0x11` validates the subfunction, calls the prepare callback, builds `51 xx`, and sets reset pending. | Same implementation. | **Not the remaining difference.** |
| `stm32c092/can_transport_fdcan.c` | Sets `FDCAN_STORE_TX_EVENTS`, assigns a marker, polls the TX Event FIFO, and exposes completion/error callbacks. | Same implementation under `examples/stm32c092/`. | **Not the remaining difference.** |
| `stm32c092/uds_app_fdcan.c` | Polls TX events before notifying the endpoint, then calls endpoint processing and ticking. | Same implementation. | **Not the remaining difference.** |
| `stm32c092/uds_platform_fdcan.c` | `uds_c092_platform_reset_execute()` calls `NVIC_SystemReset()` for hard/soft reset. | Same implementation. | **Not the remaining difference.** |
| `Src/main.c` | Initializes HAL/clock/GPIO/FDCAN, configures filter and RX notification, starts FDCAN, initializes the adapter, and runs `uds_c092_app_process()`. It does not attach readiness diagnostics, does not mark `READY`, and its RX callback uses the compatibility wrapper with raw DLC and `0, 0` FD/BRS flags. | The maintained README requires an explicit readiness boundary and recommends `_ex()` with converted DLC and actual frame metadata. | **Remaining generated-integration defect and observability gap.** |
| `Src/fdcan.c` | `MX_FDCAN1_Init()` enables Classic CAN and FIFO mode, but the IOC contains no TX-event notification setting. | The maintained adapter stores TX events in each outgoing adapter-owned header; TX-event IRQ is optional when mainline polling is used. | Missing IRQ is **not by itself causal** because the application polls the stored FIFO. The generated project still lacks the optional callback and diagnostics. |
| `Src/stm32c0xx_it.c` | Routes only `FDCAN1_IT0_IRQHandler()` to `HAL_FDCAN_IRQHandler()`. | The maintained adapter supports an optional TX-event callback plus mandatory mainline polling. | Confirms no ISR TX-event hook is linked, but does not defeat the mainline polling path. |
| Keil map | `HAL_FDCAN_IRQHandler` resolves `HAL_FDCAN_TxEventFifoCallback` to the weak HAL callback. `main.o` owns only `HAL_FDCAN_RxFifo0Callback`. | The maintained source provides `uds_c092_fdcan_on_tx_event()` but does not require the IRQ hook if polling is active. | **Binary evidence of incomplete optional wiring**, not proof that completion is impossible. |
| Keil map / dead-code report | The generated image retains `uds_c092_app_process`, `uds_c092_fdcan_poll_tx_events`, and `uds_c092_fdcan_tx_complete`; the map’s “Removing” records also include unused exception-table sections. | Same symbols are available. | Confirms the mainline poll path is linked; the missing ISR callback is not enough to explain the failure. |

## End-to-end trace

### 1. `11 01` request

The reporter capture shows `0x7E0`, DLC 8, `02 11 01 55 55 55 55 55`. The generated RX callback reads the frame and calls `uds_c092_app_rx_from_isr()`. The maintained application stores one bounded pending frame. On the next main-loop pass, `uds_isotp_endpoint_receive()` feeds it to ISO-TP and, once complete, to the UDS server.

### 2. `51 01` response construction

The UDS `0x11` handler accepts subfunction `0x01`, invokes the platform prepare callback, creates the positive response `51 01`, and marks reset pending. The endpoint records the response as reset-associated and emits the `RESPONSE_READY` event when applicable.

### 3. TX submission versus TX completion

The endpoint first submits the response through `uds_c092_fdcan_send()`. That function sets `FDCAN_STORE_TX_EVENTS`, assigns a message marker, and calls `HAL_FDCAN_AddMessageToTxFifoQ()`. `HAL_OK` means only that the controller accepted the frame; it is not the reset boundary.

On each subsequent application pass, `uds_c092_app_process()` polls the FDCAN TX Event FIFO. If the matching marker reports a successful TX event, `uds_c092_fdcan_tx_complete()` returns true and the endpoint completes the in-flight response. The reporter map shows `uds_c092_fdcan_poll_tx_events` and `uds_c092_fdcan_tx_complete` retained in the image, so this path is present in the tested binary.

### 4. `NVIC_SystemReset()`

After the matching completion is delivered to the endpoint, `uds_server_complete_reset()` invokes `uds_c092_platform_reset_execute()`, which calls `NVIC_SystemReset()`. This is the only point at which the maintained C092 path executes the MCU reset. The source comparison found no reset-order difference between the archive and the repository.

### 5. FDCAN initialization after reset

A real `NVIC_SystemReset()` restarts `main()` from the reset vector. The reporter `main.c` calls `HAL_Init()`, clock configuration, `MX_GPIO_Init()`, `MX_FDCAN1_Init()`, filter configuration, global-filter configuration, RX notification activation, `HAL_FDCAN_Start()`, transport initialization, and endpoint initialization again. This proves that the normal reinitialization sequence exists in source, but it does not prove the hardware completed it before the next tester request.

### 6. RX interrupt and handoff

The generated callback only consumes one FIFO0 message per callback and calls the compatibility wrapper. The wrapper accepts the configured physical request ID and copies one bounded frame. It does not run UDS in interrupt context. The callback does not expose a readiness state, so a tester cannot distinguish “reset accepted, MCU booting” from “FDCAN RX path failed.”

### 7. ISO-TP and `10 01`

Both attached captures show the next request on `0x7E0` approximately 1.3–1.6 ms after `0x51 01`:

```text
latest: t=2.2586  7E0  02 11 01 ...
        t=2.2591  7E8  02 51 01 ...
        t=2.2607  7E0  02 10 01 ...

prior:  t=1.1884  7E0  02 11 01 ...
        t=1.1889  7E8  02 51 01 ...
        t=1.1902  7E0  02 10 01 ...
```

The `10 01` frame is visible on the bus, but the capture contains no evidence that it entered the post-reset RX FIFO, reached the application pending handoff, completed ISO-TP, or reached UDS dispatch. The generated application has no `READY` indication and the reporter project still has no bounded startup instrumentation. The maintained correction now accepts a valid frame into a single bounded mailbox after FDCAN start even while the trace is `BOOTING`; only a simultaneous second frame is counted as `RX_MAILBOX_FULL`. Therefore the capture still most strongly indicates a tester request racing the reboot/startup window in the reporter image, while the corrected implementation provides the evidence needed to distinguish that race from a lower-layer failure.

## What is and is not proven

| Finding | Evidence status |
|---|---|
| The positive `51 01` response is emitted. | **Directly proven** by both PCAN captures. |
| The maintained TX-completion-gated reset code is present in the reporter archive. | **Directly proven** by source comparison and the Keil map. |
| The optional TX-event IRQ callback is absent from the reporter image. | **Directly proven** by `main.c`, `stm32c0xx_it.c`, and map resolution to the weak callback. |
| Mainline TX-event polling is linked. | **Directly proven** by the Keil map. |
| The `10 01` request is sent during the reset-to-ready interval. | **Strongly indicated** by the 1.3–1.6 ms spacing; the interval itself is not instrumented. |
| FDCAN failed to reinitialize. | **Not proven.** The source reruns initialization, but no register or readiness trace is available. |
| The endpoint failed to receive or dispatch `10 01`. | **Not proven.** The CAN capture only proves bus transmission, not internal reception. |
| A fixed 50 ms delay is the correct fix. | **Not supported.** No measured reset-to-ready timing is present. |

## Correct next correction

The generated project must be updated, not the generic reset algorithm copied again. Its application-owned startup should attach a `UdsC092DiagnosticTrace`, mark each successful HAL/FDCAN/endpoint stage, and expose or record the transition to `UDS_C092_DIAG_READY`. The maintained application no longer gates mailbox capture on this higher-level READY mark; it records accepted frames and processes them from mainline once endpoint initialization is complete. Its RX callback should use `uds_c092_fdcan_data_length_bytes(rxHeader.DataLength)` and pass the actual Classic/FD, BRS, identifier-type, and remote-frame metadata through `uds_c092_app_rx_from_isr_ex()`.

The tester must send `10 01` only after the project-defined ready indication, or use a bounded readiness-aware retry policy. A fixed delay may be used experimentally to measure the window, but it must not be treated as the ECUReset correctness mechanism. The next HIL trace must record `RESET_ENTRY`, `FDCAN_STARTED`, `UDS_INIT_DONE`, `READY`, RX arrival, UDS dispatch, TX submission, and TX completion timestamps. Without those points, the remaining distinction between “request during boot” and “post-reset FDCAN/RX failure” cannot be made from the current captures.

## References

[1]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/19 "GitHub Issue #19: ECUReset (0x11) dead bug"

[2]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/19#issuecomment-5424985803 "Latest reporter comment and STM32C092 project archive"

[3]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/tree/main/examples/stm32c092 "Maintained STM32C092 FDCAN adapter"

## Concrete generated-project correction

The repository cannot safely edit the reporter’s CubeMX output in place, but the following application-owned changes are the minimal correction to apply to that generated project. They are deliberately separate from the generic library.

First, increase `hfdcan1.Init.StdFiltersNbr` from `1` to `2` in `Src/fdcan.c` and regenerate or update the IOC accordingly. Configure two exact standard-ID filters, one for `UDS_C092_REQUEST_ID` (`0x7E0`) and one for `UDS_C092_FUNCTIONAL_REQUEST_ID` (`0x7DF`), using `FDCAN_FILTER_DUAL` to route both to FIFO0. Keep the global filter rejecting non-matching standard/extended frames and remote frames.

Second, replace the generated startup handoff with the following application-owned sequence. Every diagnostic mark must occur only after the corresponding real call succeeds; the failure branch must enter `FAULT` through the project’s existing error policy.

```c
#include "uds_diagnostics.h"

static UdsC092DiagnosticTrace uds_trace;

/* immediately after HAL_Init() */
uds_c092_diagnostic_init(&uds_trace, HAL_GetTick());
uds_c092_diagnostic_mark(&uds_trace, UDS_C092_BOOT_HAL_INIT_DONE, HAL_GetTick());

/* after SystemClock_Config() succeeds */
uds_c092_diagnostic_mark(&uds_trace, UDS_C092_BOOT_CLOCK_READY, HAL_GetTick());

/* after MX_GPIO_Init() succeeds */
uds_c092_diagnostic_mark(&uds_trace, UDS_C092_BOOT_GPIO_READY, HAL_GetTick());

/* immediately before MX_FDCAN1_Init() */
uds_c092_diagnostic_mark(&uds_trace, UDS_C092_BOOT_FDCAN_INIT_START, HAL_GetTick());

/* after MX_FDCAN1_Init() succeeds */
uds_c092_diagnostic_mark(&uds_trace, UDS_C092_BOOT_FDCAN_INIT_DONE, HAL_GetTick());

/* after both exact filters and the global filter succeed */
uds_c092_diagnostic_mark(&uds_trace, UDS_C092_BOOT_FDCAN_FILTER_DONE, HAL_GetTick());

/* Initialize transport and endpoint before enabling RX notification. */
uds_c092_fdcan_transport_init(&uds_transport, &hfdcan1,
                              UDS_C092_REQUEST_ID, UDS_C092_RESPONSE_ID);
uds_c092_app_attach_diagnostics(&uds_trace);
uds_c092_app_init_default(&uds_transport, uds_c092_platform_now_ms());

if (HAL_FDCAN_ActivateNotification(
        &hfdcan1,
        FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_TX_EVT_FIFO_NEW_DATA,
        0U) != HAL_OK) {
    uds_c092_diagnostic_fault(&uds_trace, HAL_GetTick());
    Error_Handler();
}
uds_c092_diagnostic_mark(&uds_trace, UDS_C092_BOOT_FDCAN_NOTIFICATION_DONE, HAL_GetTick());

if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
    uds_c092_diagnostic_fault(&uds_trace, HAL_GetTick());
    Error_Handler();
}
uds_c092_diagnostic_mark(&uds_trace, UDS_C092_BOOT_FDCAN_START_DONE, HAL_GetTick());
uds_c092_diagnostic_mark(&uds_trace, UDS_C092_BOOT_DIAGNOSTIC_READY, HAL_GetTick());
```

Third, replace the reporter RX callback handoff. The C092 HAL’s Classic DLC values happen to be numerically `0..8`, but the conversion is still required so that the application receives byte lengths rather than HAL encoding values and remains correct if the frame format changes.

```c
uds_c092_app_rx_from_isr_ex(
    rxHeader.Identifier, data,
    uds_c092_fdcan_data_length_bytes(rxHeader.DataLength),
    rxHeader.FDFormat == FDCAN_FD_CAN,
    rxHeader.BitRateSwitch == FDCAN_BRS_ON,
    rxHeader.IdType == FDCAN_EXTENDED_ID,
    rxHeader.RxFrameType == FDCAN_REMOTE_FRAME);
```

Finally, add the application-owned TX-event forwarding callback:

```c
void HAL_FDCAN_TxEventFifoCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t flags) {
    if (hfdcan == &hfdcan1)
        uds_c092_fdcan_on_tx_event(&uds_transport, flags);
}
```

The main loop must continue to call `uds_c092_app_process(uds_c092_platform_now_ms())`. The TX-event callback is optional when mainline polling is guaranteed, but enabling it removes the generated-project ambiguity and the mainline poll remains the fallback. The tester must not send `10 01` based only on a fixed elapsed delay; it must wait for the project-defined `READY` indication or the HIL campaign must measure and document the actual reset-to-ready interval.

The new checker can be run against the corrected project with:

```text
python3 tests/conformance/check_c092_generated_integration.py /path/to/STM32C092_UDS
```

It is intentionally a source-contract check, not a substitute for the board trace.
