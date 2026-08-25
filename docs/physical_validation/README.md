# Physical validation

## Status

Physical validation is not complete in the current environment. No CAN interface, USB CAN adapter, serial probe, or debug probe is exposed to the sandbox. The repository is ready for a board-specific campaign once the hardware profile is filled and the target images are flashed through the operator’s approved path.

## Required setup

The Classical CAN campaign uses the STM32F767 bxCAN target, an external CAN transceiver, a correctly terminated two-node bus, an independent CAN tester or analyzer, and the request/response identifiers configured by the firmware. The STM32C092 campaign uses FDCAN1 in Classical CAN mode with the `c092-fdcan-classic` profile, a compatible transceiver, generated message-RAM/filter settings, and a truthful TX-completion path based on stored TX Event FIFO entries. The CAN-FD campaign requires a selected FDCAN-capable STM32 target or external CAN-FD controller, CAN-FD transceiver, compatible analyzer, nominal and data bit rates, BRS configuration, and generated message-RAM/filter settings.

Before sending traffic, record the board revision, MCU, transceiver part and mode, power/ground, pin mapping, termination, CAN IDs, addressing mode, nominal/data bit rates, BRS, message-RAM configuration, filter configuration, firmware commit, compiler, linker script, and recovery method in [`board_profile.yaml`](board_profile.yaml).

## Test sequence

| Stage | Evidence |
|---|---|
| Bring-up | Board powers safely, SWD/recovery path works, transceiver is enabled, and the analyzer sees the expected nominal bus rate. |
| Positive transport | Tester Present and short UDS requests/responses; Classical CAN SF; C092 FDCAN Classic CAN SF; CAN-FD SF at selected DLC boundaries; FF/CF transfers. |
| C092 padding | With the C092 Classical CAN profile enabled, capture a short response such as `02 3E 00 CC CC CC CC CC`; verify DLC 8, valid logical bytes, and `0xCC` in every unused byte. Repeat for FF, FC, and final CF. |
| Flow Control | CTS, nonzero and zero BS, valid millisecond/microsecond STmin, repeated WAIT within bound, and OVERFLOW abort. |
| Malformed traffic | Wrong CAN ID, invalid DLC, invalid PCI, reserved STmin, invalid BS/STmin combination, wrong CF sequence, and timeout. |
| UDS services | Session Control, ReadDataByIdentifier, SecurityAccess using an approved test policy, CommunicationControl, RoutineControl, and transfer-policy cases. |
| Recovery | Controlled reset and error recovery only after the signed test plan and recovery path are confirmed. |
| C092 ECUReset | Send physical request `11 01`; capture positive response `51 01`; record response-observed timestamp and reset-observed timestamp; accept only if response observation precedes reset. Repeat with suppressed request `11 81` according to the approved UDS policy. |
| CAN-FD interoperability | BRS, DLC 64, escaped Single Frame, extended First Frame above 4,095 bytes, peer reassembly, and negative cases. |

Do not enable Flash erase/program, firmware activation, reset, or other destructive cases during initial bring-up. Those cases require an approved test plan, sacrificial hardware/image, power control, recovery access, and explicit operator confirmation.

## Capture requirements

Capture raw CAN/CAN-FD frames with timestamps, direction, arbitration ID, frame format, DLC, BRS, data bytes, tester command, ECU response, measured latency, inter-frame timing, bus-load condition, and verdict. For C092 ECUReset, also record the independent reset-observed event or reset/reconnect timestamp; for C092 padding, retain the complete eight-byte data field including unused bytes. Preserve the analyzer configuration and export the raw trace in an unmodified format. Summarize the result in [`evidence_template.md`](evidence_template.md), then attach the trace and firmware/build hashes.

## Execution boundary

The sandbox can run host tests and dry-run report generation, but it cannot claim physical evidence without connected hardware. The operator must supply the selected board and analyzer details before a live run. The HIL runner is safety-gated and non-destructive by default. Generate the C092-specific inventory with:

```sh
python3 tests/standalone/run_uds_iso_tp_hil.py \
  --dry-run --profile c092-fdcan-classic \
  --board-profile docs/physical_validation/board_profile.yaml \
  --analyzer <analyzer-name> \
  --json c092.json --csv c092.csv --report c092.md
```

A temporary SocketCAN `vcan0` creation attempt in this environment returned `Operation not permitted`; no simulated SocketCAN result is therefore reported either. If a future runner provides `vcan0`, its report must remain labeled simulated and must not satisfy the `PHYSICAL-HIL` evidence class.
