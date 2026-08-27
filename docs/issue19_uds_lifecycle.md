# Issue #19 UDS/ISO-TP ECUReset lifecycle review

## Scope and evidence

This review follows the reporter’s latest instruction to stop treating the STM32C092 CAN driver as the primary suspect and to inspect the generic UDS/ISO-TP transaction lifecycle. The comparison used the latest reporter archive attached to Issue #21, the maintained generic sources, and the newest Issue #21 discussion [1] [2]. The archive is treated as source evidence only; it is not executed.

> `51 01 must be completely transmitted BEFORE the reset is executed.`

That invariant is now tested at the generic endpoint boundary with a deliberately delayed TX-completion callback. The test is a protocol-level model and does not prove the electrical bus transaction.

## End-to-end maintained call chain

| Stage | Maintained source and function | State/evidence |
|---|---|---|
| RX CAN frame | Application adapter hands an `IsoTpCanFrame` to `uds_isotp_endpoint_receive()` | ISO-TP receives a Classic CAN single-frame request. |
| ISO-TP RX | `library/src/endpoint.c:132–140` calls `isotp_rx_feed()` | A complete UDS payload is produced before UDS dispatch. |
| UDS dispatcher | `library/src/endpoint.c:144–146` calls `uds_server_handle_addressed()` | Service `0x11` is selected by `library/src/uds.c:1078–1083`. |
| ECUReset service | `library/src/uds.c:466–501`, `service_ecu_reset()` | The callback validates `0x01`, response `51 01` is generated, and `reset_pending` is set. |
| Response staging | `library/src/endpoint.c:147–159`, `start_response()` | The response is marked as reset-bearing and staged for ISO-TP TX. |
| ISO-TP TX | `library/src/endpoint.c:178–205` | The response frame is submitted; `tx_in_flight` becomes true. |
| Physical completion boundary | `library/src/endpoint.c:189–192`, then `uds_isotp_endpoint_tx_complete()` | Queue acceptance is not completion. The configured callback must report the defined transport completion. |
| Reset scheduling/execution | `library/src/endpoint.c:75–87`, `uds_server_complete_reset_at()`, and `uds_server_tick()` | Final-frame completion only schedules the reset; the platform callback is invoked once from the mainline tick after the configured non-blocking guard. |
| Reboot | Application-owned reset callback | A real MCU reset must rerun the complete application initialization. |
| First request after reboot | Reinitialized endpoint receives `10 01` | UDS starts in default session and produces `50 01`. |

## Exact generic lifecycle defect found

The reporter archive and the maintained endpoint both carried `tx_reset_completion` as a protocol-owned marker. The marker is set when `start_response()` stages an ECUReset response and is copied into `pending_reset_completion`/`in_flight_reset_completion` during TX submission. The previous `complete_in_flight()` implementation cleared `in_flight_final` and `in_flight_reset_completion`, but did **not** clear `tx_reset_completion`.

That left the endpoint’s reset-bearing TX marker asserted after the completion callback had already executed the reset action. A later endpoint reuse without a full object reinitialization could therefore inherit stale reset-completion state. The new correction clears `tx_reset_completion` at the same completion boundary as the other in-flight fields. TX-error abort already cleared this marker, and full `uds_isotp_endpoint_init()` also cleared it; the missing path was successful delayed completion.

The new regression `library/tests/uds/test_ecu_reset_lifecycle.c` fails against the reporter-era implementation at the post-completion clean-state assertion and passes after the correction. It asserts `reset_count == 0` while the final `51 01` frame is only queued/in flight, releases TX completion explicitly, verifies the reset enters `WAIT_GUARD`, checks that a five-millisecond guard is non-blocking, and then asserts exactly one mainline reset execution. It reinitializes the endpoint and sends `10 01` without an artificial post-reset delay, verifying `50 01` for 100 cycles.

This is a real generic lifecycle defect. It is not a claim that the stale marker alone proves the reporter’s physical C092 failure: a correctly functioning `NVIC_SystemReset()` normally clears RAM by rebooting the MCU. The reporter’s exact hardware stop point still requires board instrumentation. The defect is nevertheless important because the endpoint contract must be correct when reset callbacks return in tests, when a platform reset is deferred or intercepted, and whenever the endpoint object is reused by a supervisory application.

## Why the earlier C092 fixes did not solve this protocol-level gap

The earlier C092 changes addressed generated-project startup ordering, bounded RX handoff, FDCAN TX-event correlation, Classic CAN filtering, and diagnostic readiness. Those changes cannot detect a generic endpoint flag that remains set after a successful delayed completion because they operate below or beside the UDS lifecycle boundary.

The previous host reset test used an immediate `tx_complete` callback and then reinitialized the endpoint. That proved response-before-reset only in the immediate-completion path and erased the stale marker during reinitialization. The new lifecycle test deliberately separates queue acceptance from completion and checks the endpoint before reinitialization, which exposes the missing clear operation.

The C092 mock and host tests remain useful but are not substitutes for this generic regression. The final test stack now has three separate responsibilities: the generic lifecycle test proves reset ordering and clean protocol state; the C092 application test proves immediate post-start RX acceptance and bounded handoff; and physical HIL must prove the actual STM32C092 interrupt, reset, and bus behavior.

## Validation status and limits

The new generic test covers 100 cycles of `11 01 → 51 01 → delayed TX completion → non-blocking reset guard → reset execution → immediate model reinitialization → 10 01 → 50 01`. It asserts clean RX/TX/UDS/reset state after each completion and reinitialization. Normal and sanitizer CTest, target-oriented checks, and CI remain required after integration.

This review does not claim a Keil/Arm Compiler 6 build, STM32C092 flash, CAN-analyzer trace, reset-cause capture, measured reset-to-ready timing, or 100 physical cycles. Issue #19 should remain open until that evidence exists.

## References

[1]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/21 "Issue #21 reporter discussion and latest archive"
[2]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/19 "Issue #19 ECUReset recovery report"
[3]: https://github.com/user-attachments/files/31490398/STM32C092_UDS.zip "Reporter STM32C092 project archive attached to Issue #21"
