# Issue #5 — ISO-TP TX Flow-Control Block Size

## Executive finding

The audit found that the transmitter already decoded the FC BS byte and implemented the required block-boundary state management in `library/src/isotp.c`. The existing state machine uses `remote_block_size` as the active peer-provided BS limit and `block_count` as the number of successfully emitted CF frames in the current block. The original issue was therefore not a missing BS decoder or a broken block transition. The gap was insufficiently explicit regression evidence for the required BS cases, plus the lack of a distinct return value for an explicit peer OVERFLOW versus a reserved flow-status value.

## Existing implementation

`isotp_tx_feed_flow_control()` accepts FC only while the TX state is `WAIT_FIRST_FLOW_CONTROL` or `WAIT_BLOCK_FLOW_CONTROL`. It validates the configured peer CAN ID, frame profile/DLC, FC PCI type, and STmin. CTS stores the received BS and STmin, resets `block_count` to zero, and enters `SEND_CONSECUTIVE`. WAIT leaves the active BS counter unchanged and is bounded by `max_wait_frames`. OVERFLOW resets the active transfer immediately. In `isotp_tx_next()`, each emitted CF increments `block_count`; when a nonzero BS is reached and payload remains, the state becomes `WAIT_BLOCK_FLOW_CONTROL`. BS zero does not create a block boundary, and the CF sequence number is not reset at an FC boundary.

The implementation now returns `ISOTP_ERR_FLOW_OVERFLOW` only for FC flow status `0x02` (OVERFLOW). Reserved flow-status values and invalid STmin remain on the generic `ISOTP_ERR_FLOW_CONTROL` path. This is a diagnostic distinction for callers and does not change the transfer-abort behavior.

## Fix and tests

The redundant Classical CAN/CAN-FD single-frame condition in `isotp_tx_start()` was simplified to `if (length <= 7U)` without changing the following CAN-FD escaped-SF or multi-frame branches. The public status enum was extended with `ISOTP_ERR_FLOW_OVERFLOW`, and the endpoint integration assertion was updated to verify propagation.

The ISO-TP contract test now contains named regression cases for BS zero, BS one, BS three, BS larger than the remaining CF count, BS changes between blocks, WAIT not consuming a BS block, CTS resetting the BS counter, sequence-number continuity, and STmin independence. The tests use payloads larger than a Single Frame and assert emitted CF sequence numbers and state transitions. Existing tests continue to cover Classical CAN and CAN-FD transport, malformed FC frames, reserved STmin, timeouts, sequence errors, CAN-ID validation, CAN-FD escaped Single Frame, and extended First Frame lengths.

| Acceptance item | Evidence |
|---|---|
| FC BS byte decoded | `isotp_tx_feed_flow_control()` stores `frame->data[1]` in `remote_block_size` |
| BS zero unlimited | `test_tx_fc_bs_zero` |
| BS one boundary | `test_tx_fc_bs_one` |
| BS three boundary | `test_tx_fc_bs_three` |
| BS larger than remaining | `test_tx_fc_bs_larger_than_remaining` uses BS 255 |
| BS changes between blocks | `test_tx_fc_bs_changes_between_blocks` uses 3, then 2, then 4 |
| WAIT does not consume BS | `test_tx_fc_wait_does_not_consume_bs` |
| CTS resets BS counter | `test_tx_fc_cts_resets_bs_counter` |
| Sequence continuity | `test_tx_bs_preserves_sequence_number` |
| STmin remains independent | `test_tx_bs_respects_stmin` checks BS 0 and BS 3 with 10 ms STmin |
| OVERFLOW aborts | Existing FC test plus endpoint propagation test; explicit status is `ISOTP_ERR_FLOW_OVERFLOW` |
| No duplicate implementation/CANopen | Standalone architecture check |

## Validation status

The requested validation gates completed successfully at the current repository tip. The normal and sanitizer configurations both execute all four CTest suites.

| Gate | Result |
|---|---|
| Architecture regression check | PASS; no CANopen dependency or duplicate authoritative implementation detected |
| Normal CMake/Ninja build | PASS |
| Normal CTest | PASS; 4/4 suites |
| ASan/UBSan CMake/Ninja build | PASS |
| ASan/UBSan CTest | PASS; 4/4 suites |
| Coverage build and CTest | PASS; 4/4 suites and 54% total source-line coverage |
| ISO-TP source coverage | 86% for `library/src/isotp.c` |
| UDS source coverage | 67% for `library/src/uds.c` |
| clang-format | PASS |
| clang-tidy | PASS |
| cppcheck | PASS; only the existing informational configuration-count note |
| STM32F767 cross-build | PASS; 22,692 B Flash and 39,064 B RAM reported by the linker |
| Classical CAN HIL runner | DRY-RUN only; 15 cases |
| CAN-FD HIL runner | DRY-RUN only; 18 cases |
| Physical HIL | NOT EXECUTED; no real hardware or traces were available |

Physical HIL is not claimed unless real STM32 hardware, a transceiver, an independent peer/analyzer, and timestamped CAN traces are available. The STM32F767 target is bxCAN/Classical CAN only; CAN-FD evidence requires a separate FDCAN-capable target.

Issue #5 is resolved for the repository implementation and host evidence after these gates pass. This report does not claim formal ISO certification, production security, bootloader correctness, or physical interoperability without the corresponding evidence.
