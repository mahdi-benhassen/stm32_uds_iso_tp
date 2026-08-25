# Issues #10 and #12 final implementation report

## Executive summary

Issues [#10][1] and [#12][2] were audited against the current repository and the supplied STM32C092 FDCAN project before source changes. The generic ISO-TP implementation already supported configurable Classical CAN padding and deferred ECUReset completion. The reporter project did not enable the padding policy and treated FDCAN TX FIFO acceptance as transmission completion. The fix therefore preserves the generic defaults and portability while adding explicit application profiles, a C092-specific FDCAN adapter, completion instrumentation, regression tests, and a reproducible HIL procedure.

The changes were committed and pushed to `main` in commit `6c7e159`.

| Area | Result |
|---|---|
| Issue #10 padding policy | Explicitly enabled with `0xCC` for the maintained F767 and C092 Classical CAN profiles; generic default remains disabled. |
| Issue #12 reset ordering | Reset execution remains deferred until the final response frame reaches the controller-specific TX-complete boundary. |
| C092 controller integration | Added FDCAN TX FIFO + stored TX Event FIFO adapter with marker matching and abort/lost-event failure handling. |
| Generic portability | No HAL, CMSIS, FDCAN, bxCAN, register, heap, blocking wait, or reset primitive was added to `library/`. |
| Physical C092 HIL | **Not executed.** No C092 board, CAN analyzer, or debug probe was available in the sandbox. Issues #10 and #12 must not be closed as hardware-validated based on this run. |

## Root causes and fixes

### Issue #10 — Classical CAN `0xCC` padding

The generic library’s behavior is intentional: `padding_enabled` defaults to false, while the configurable default fill byte is `0xCC`. The supplied C092 application called `isotp_config_classic_can()` but never enabled padding, so its emitted frame used the logical DLC instead of DLC 8. The maintained F767 application had the same profile omission.

`App/Inc/uds_app_config.h` now defines the F767 profile as enabled with `0xCC`, and `examples/stm32c092/uds_app_config.h` defines the corresponding C092 profile. Each application calls `isotp_config_set_padding(&config.isotp_config, true, 0xCC)`. Serialization-only behavior is unchanged: the logical ISO-TP N_PDU length and receive semantics remain based on N_PCI, while unused Classical CAN bytes are filled with `0xCC`. Existing host tests continue to cover Single Frame, exact seven-byte Single Frame, First Frame, Flow Control, final Consecutive Frame, disabled/custom fill, CAN-FD fill, and logical-length preservation.

### Issue #12 — ECUReset response before reset

The supplied C092 transport used `HAL_FDCAN_AddMessageToTxFifoQ()` and had no TX completion callback. `HAL_OK` only establishes that the frame was accepted by the controller’s transmit queue; it does not establish that the final response was physically transmitted. The project also did not provide the current application-owned ECUReset preparation/execution path.

The new C092 adapter configures Classic CAN headers with `FDCAN_STORE_TX_EVENTS`, assigns a rotating message marker, and drains `FDCAN_TxEventFifoTypeDef` entries through the generated `HAL_FDCAN_TxEventFifoCallback()`. Completion is reported only for a matching marker and successful `FDCAN_TX_EVENT`. TX Event FIFO lost/full conditions and non-success event types fail closed. Mainline code, not the ISR, calls `uds_isotp_endpoint_tx_complete()` after the adapter reports completion. The generic endpoint then emits the optional lifecycle events and invokes the application-owned reset executor exactly once.

The endpoint instrumentation events are `REQUESTED`, `RESPONSE_READY`, `TX_SUBMITTED`, `TX_COMPLETE`, and `EXECUTED`. Host tests assert the normal sequence, prove that reset does not occur before completion, call completion twice to verify exactly-once behavior, and verify suppressed `0x11 0x81` semantics as an immediate no-response execution path.

## Changed deliverables

| Path | Purpose |
|---|---|
| `library/include/uds_iso_tp/endpoint.h` and `library/src/endpoint.c` | Optional reset lifecycle instrumentation while retaining the existing deferred completion contract. |
| `library/tests/uds/test_endpoint.c` | Response-before-reset, ordered lifecycle, suppressed reset, and exactly-once regression coverage. |
| `App/Inc/uds_app_config.h`, `App/Src/uds_app.c` | Explicit F767 bxCAN Classical CAN padding profile. |
| `examples/stm32c092/` | C092 FDCAN transport, application composition, platform callbacks, profile constants, and integration guide. |
| `tests/portability/check_c092_fdcan_gcc.sh` | Reproducible C99 freestanding syntax check against an externally supplied C092 CubeMX/HAL project. |
| `tests/standalone/run_uds_iso_tp_hil.py` | Dedicated `c092-fdcan-classic` dry-run inventory. |
| `tests/physical/hil_test_plan.json`, `docs/physical_validation/board_profile.yaml`, `docs/physical_validation/README.md` | C092 profile, padding capture criteria, reset-order evidence, and operator procedure. |
| `.github/workflows/standalone-uds.yml` | CI format coverage for C092 sources and C092 HIL dry-run report generation. |

## Validation results

All available software and cross-build gates passed locally. The published GitHub Actions workflow also completed successfully for commit `6c7e159` in run [32908737478][3]. The C092 syntax check was run against the extracted reporter project at `ISOTP_MAX_PAYLOAD=4095`; it passed with the vendor-header warning suppression define enabled.

| Gate | Result |
|---|---|
| Normal CMake/Ninja build with examples | PASS |
| Normal CTest | PASS, 6/6 suites |
| ASan/UBSan CTest | PASS, 6/6 suites |
| Architecture check | PASS, 183 tracked paths |
| Validation assets | PASS, 13 conformance vectors and 20 physical cases |
| Strict Cortex-M0+ freestanding ARM GCC compile | PASS, `ISOTP_MAX_PAYLOAD=4095` |
| STM32F767 root cross-build | PASS, 14,036 B Flash and 43,312 B RAM in the local Release build |
| Coverage | PASS, 67% total generic-library line coverage; `isotp.c` 88%, `uds.c` 79%, `endpoint.c` 87% |
| clang-format | PASS, including C092 files |
| clang-tidy | PASS |
| cppcheck | PASS; informational configuration-count note only |
| C092 adapter syntax check against supplied HAL headers | PASS |
| C092 HIL inventory | PASS as dry-run only, 15 cases |
| Physical C092/F767 HIL | **NOT EXECUTED** |

## Required next step before closing either issue

An operator must build and link the C092 project with the selected Keil/ARM toolchain, review the map file and RAM margin, flash the target, and capture raw analyzer evidence. For Issue #10, the trace must show DLC 8 and complete unused-byte data equal to `0xCC` for a short response, First Frame, Flow Control, and final Consecutive Frame. For Issue #12, the trace and independent reset/reconnect timestamp must show the positive `51 01` response observed before reset, with exactly one reset execution. A dry-run, host test, or controller syntax check cannot substitute for this evidence.

The credentials previously pasted into the conversation are secrets and should be revoked and rotated immediately if they have not already been rotated. No credential was included in this commit or report.

## References

[1]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/10 "Issue #10 — ISO-TP Classical CAN 0xCC padding"
[2]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/12 "Issue #12 — UDS ECUReset response ordering"
[3]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/actions/runs/32908737478 "GitHub Actions validation run for commit 6c7e159"
