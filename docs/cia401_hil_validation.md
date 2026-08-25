# CiA 401 Hardware and HIL Validation Procedure

## Purpose and boundary

This procedure is the execution record for the selected CiA 401 product personality. It covers tests that cannot be closed by host simulation alone, including physical CAN timing, transceiver behavior, safe-state behavior on real hardware, power interruption, watchdog timing, bus-off recovery, and repeatability. The repository does not claim that these tests have been executed until a controlled rig produces evidence attached to the exact firmware SHA and board identity.

The checked-in initializer is intentionally fail-closed. Running it with `--dry-run` creates a complete case-level record with status `PENDING`; it does not access hardware and cannot emit a `PASS` result.

## Rig topology

| Element | Required record | Minimum role |
|---|---|---|
| Test PC | Host OS, tool versions, UTC clock | Runs the approved campaign harness and stores evidence |
| USB-CAN interface | Manufacturer, model, serial, driver, firmware, calibration status | Injects and captures CAN traffic with timestamps |
| Independent CANopen node | Device identity, firmware, node-ID | Provides an independent master/peer for NMT, SDO, PDO, heartbeat, and CiA 302 tests |
| CAN analyzer | Model, serial, capture format | Independent trace and timing observation |
| Programmable power switch | Model, serial, timing accuracy | Controlled reset and power interruption for Flash and safe-state campaigns |
| Debug/programming interface | Probe model, serial, firmware | Flashes the exact image and captures reset/debug state |
| STM32F767 DUT | Board serial, revision, MCU ordering code | Device under test |

The physical wiring, termination, grounding, supply limits, and safety interlocks must be approved by the hardware owner before power is applied. Unknown board values remain `TBD` in [`PRODUCT_CIA401.md`](../PRODUCT_CIA401.md) and must not be inferred from this reference repository.

## Campaign order

The campaigns are executed in the following order so failures do not contaminate later evidence.

| Order | Campaign | Main acceptance areas |
|---:|---|---|
| 1 | Startup and safe state | Power-on, reset, boot time, initial NMT state, initial heartbeat, de-energized outputs |
| 2 | NMT | Pre-operational, operational, stopped, reset node, reset communication, invalid commands |
| 3 | Heartbeat | Producer, consumer, timeout, recovery, restart |
| 4 | SDO | Upload, download, segmented transfer, timeout, invalid object/subindex/length, abort codes, maximum payload |
| 5 | PDO and mapping | TPDO, RPDO, event behavior, unsupported inhibit field disclosure, SYNC, mapping, invalid mapping, traffic load |
| 6 | SYNC | Producer, consumer, synchronized PDO behavior, jitter, latency |
| 7 | EMCY | Generation, clear, repeated faults, physical trace |
| 8 | LSS | Node-ID, bitrate, store, inquire, timeout, invalid sequence |
| 9 | Bounded CiA 302 peer supervision | Peer boot-up, heartbeat, loss, timeout, recovery, restart, repeated failure, multi-node behavior |
| 10 | Bus-off | Normal, PDO-active, SDO-active, high-load, repeated bus-off, terminal-failure trials |
| 11 | Flash persistence | Power interruption at erase, data, CRC, metadata, commit, post-commit; slot selection, corruption rejection, rollback, factory default, endurance |
| 12 | Watchdog | TIM7 progress, mainline progress, each stall direction, reset and safe outputs |

The CiA 302 campaign is limited to configured peer supervision. It does not claim the absent CiA 302 Network List/Configuration Manager objects `0x1F80–0x1F89`.

## Evidence procedure

Before each campaign, record the release commit, firmware image SHA-256, OD/EDS hashes, board serial and revision, equipment identifiers, operator, and UTC start time. Capture raw CAN traffic, serial/debug output, power-switch events, and measured values. Each case must contain an unambiguous status, a short result statement, measurements where applicable, and a pointer to its raw trace.

Initialize a pending record from the checked-in plan with:

```sh
python3 tests/hardware/run_cia401_hil_campaign.py \
  --dry-run \
  --output build/hil/cia401-hil-pending.json \
  --release-commit "$(git rev-parse HEAD)" \
  --firmware-sha "<firmware-build-sha>" \
  --board-serial "<board-serial>" \
  --board-revision "<board-revision>" \
  --operator "<operator>" \
  --equipment usb_can=<serial> \
  --equipment analyzer=<serial> \
  --equipment power_switch=<serial> \
  --equipment debugger=<serial>
```

The generated artifact is a handoff scaffold, not test evidence. A qualified HIL harness must later update each case from `PENDING` only after capturing the required external evidence. The final release gate must reject records with missing board identity, missing firmware linkage, missing traces, unresolved `PENDING` cases, or a status that is not independently reviewable.

## Bus-off trial matrix

The planned bus-off campaign contains 250 trials: 30 normal, 30 with PDO traffic, 30 with SDO traffic, 30 under high bus load, 100 repeated bus-off trials, and 30 terminal-failure trials. These are planned trials, not completed results. For each trial, record the timestamp, CAN error counters, recovery count, final NMT/CAN state, outputs, and trace path.

## Flash and watchdog safety

Power interruption must be injected only through the approved fixture and within the board owner’s voltage and current limits. Interruptions must cover erase, data programming, CRC/metadata handling, commit, and the immediate post-commit window. Watchdog tests must verify both the progress contract and the board-level result, including reset cause and safe outputs. Host tests and a successful cross-build do not close either campaign.

## Timing and load measurements

The instrumented qualification image is built with `CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION=ON`. The operator must export the `CANopenReferenceTiming_GetStats()` snapshot at the end of each relevant campaign and retain the raw snapshot with the firmware SHA. The snapshot must include maximum TIM7 ISR cycles, maximum TIM7 period spacing, TIM7 warning and hard-budget counts, mainline duration, the complete `canopen_app_interrupt()` duration, each enabled SYNC/RPDO/CiA 401/CiA 402/CiA 418/TPDO phase maximum, and per-source CAN IRQ duration/counts.

The external campaign record must additionally contain the measured core clock, nominal CAN bitrate and sample point, timestamped bus-load percentage, CAN FIFO overflow count, DUT temperature, and supply voltage. The timing counters are evidence inputs only; they do not automatically determine PASS. Acceptance limits must be approved for the selected board, transceiver, clock tolerance, CAN load, and product profile before execution.

At minimum, the operator should correlate the following values:

| Measurement | Required evidence | Reason |
|---|---|---|
| TIM7 ISR maximum | DWT snapshot plus campaign duration | Demonstrates margin against the 1 ms service period. |
| TIM7 period and overrun count | DWT snapshot and GPIO/analyzer trace where available | Detects jitter and missed/late service. |
| CAN IRQ occupancy | Per-source counters and maximum cycles | Quantifies RX/TX/error interrupt pressure. |
| FIFO overflow | Target diagnostic counter and raw CAN trace | Detects loss under legitimate and invalid traffic. |
| Mainline duration | DWT snapshot and watchdog trace | Shows whether synchronous work can starve the superloop. |
| Bus load and physical timing | Independent timestamped CAN capture | Separates wire-time limits from firmware latency. |
| Temperature and supply | Calibrated instrument record | Makes timing and Flash/watchdog results reproducible. |

### Bus-load campaign matrix

Run the same representative CiA 401 PDO/SYNC workload at each target load. The analyzer must calculate load from timestamped CAN traffic over a documented observation window; the operator must retain the raw trace and record the matching firmware timing snapshot. The table is a campaign template, not a result table. Every cell remains `PENDING` until hardware execution.

| Target bus load | TIM7 max cycles | CAN IRQ max cycles | FIFO overflow count | TIM7 overrun count |
|---:|---:|---:|---:|---:|
| 25% | PENDING | PENDING | PENDING | PENDING |
| 50% | PENDING | PENDING | PENDING | PENDING |
| 75% | PENDING | PENDING | PENDING | PENDING |
| 90% | PENDING | PENDING | PENDING | PENDING |
| 95% | PENDING | PENDING | PENDING | PENDING |

The campaign must use the same clock, bitrate, sample-point configuration, PDO mapping, node count, and traffic mix at every load point. If the requested percentage cannot be achieved within the analyzer tolerance, record the measured percentage rather than silently relabeling the row. Acceptance limits for warning counts, hard overruns, FIFO overflow, and application latency must be approved for the selected product before execution; the repository does not convert this matrix into a conformance claim.

### CAN-to-application latency procedure

Latency must be measured with a common time base and retained alongside the DWT snapshot. Mark the CAN RX interrupt entry with the source-specific timing wrapper and correlate it with the captured frame identifier and analyzer timestamp. For `can_rx_to_rpdo_latency_cycles_max`, measure from the matching RX event to completion of `CO_process_RPDO()`. For `can_rx_to_application_latency_cycles_max`, measure from the matching RX event to the update of the application variable under test. For `rpdo_to_gpio_output_latency_cycles_max`, use a board-specific GPIO probe or logic-analyzer channel at the real output transition; the generic reference weak hardware seam cannot supply physical transition evidence by itself. For `sync_to_tpdo_latency_cycles_max`, correlate the SYNC frame timestamp with the corresponding TPDO transmission timestamp and retain the frame IDs, sequence, and observation window.

The host harness must report the clock frequency used to convert cycles to time, probe/analyzer calibration status, frame identifiers, trigger definitions, sample rate, and worst-case/max sample count. Missing correlation, missing board-level output probing, or unavailable timestamps leaves the field `null`/`PENDING`; no value may be inferred from the software phase maximum.

The pending initializer writes these fields as `null` or empty maps and continues to set `status` to `PENDING` and `pass_claim_allowed` to `false`. No empty field may be replaced with a guessed value.
