# Hardware Acceptance Runner

`run_uds_cia302_acceptance.py` is the Linux/SocketCAN acceptance runner for the STM32F767 UDS/ISO-TP diagnostic path and CiA 302 NMT behavior. It sends raw Classical CAN frames, validates UDS and heartbeat responses, exercises NMT transitions, and optionally writes a JSON result summary.

The full bench procedure is documented in [`docs/hardware/uds_cia302_test_procedure.md`](../../docs/hardware/uds_cia302_test_procedure.md).

## Requirements

The runner requires Python 3.9 or newer, Linux SocketCAN, a CAN interface such as `can0`, and permission to open an `AF_CAN/SOCK_RAW` socket. The target firmware must be configured with the expected CAN bitrate, node-ID, diagnostic IDs, UDS session behavior, and heartbeat producer. A CAN analyzer and a current-limited bench supply are strongly recommended.

The runner does not configure the interface or power-cycle the target. Prepare the interface separately, for example:

```bash
sudo ip link set can0 up type can bitrate 500000
```

Use a real hardware interface for acceptance. A `vcan0` interface is appropriate only for transport-level development with an external simulator; it cannot prove embedded timing, transceiver behavior, reset recovery, or NMT state integration.

## Dry run and safe run

Check that the runner can be imported and that all named tests are discoverable without opening a CAN socket:

```bash
python3 -m py_compile tests/hardware/run_uds_cia302_acceptance.py
python3 tests/hardware/run_uds_cia302_acceptance.py --dry-run
```

Run the default non-destructive set and archive results:

```bash
mkdir -p results
GIT_SHA="$(git rev-parse HEAD)" \
python3 tests/hardware/run_uds_cia302_acceptance.py \
  --iface can0 \
  --remote-node 2 \
  --uds-tx-id 0x7E0 \
  --uds-rx-id 0x7E8 \
  --json-out results/uds_cia302.json
```

For a two-node network, add the second monitored node to make broadcast and
address-isolation checks meaningful:

```bash
GIT_SHA="$(git rev-parse HEAD)" \
python3 tests/hardware/run_uds_cia302_acceptance.py \
  --iface can0 --remote-node 2 --additional-remote-node 3 \
  --heartbeat-period 1.0 --heartbeat-max-gap 2.0 \
  --json-out results/uds_cia302_multi_node.json
```

The timing values must match the heartbeat producer configuration. Do not use
`--heartbeat-period` unless the producer period is known from the target OD or
product configuration; `--heartbeat-max-gap` is always checked by the timing
test. Broadcast tests verify every node listed by `--remote-node` and repeated
`--additional-remote-node` options.


The process exits with status `0` when there are no failures and `1` when at least one test fails. Runtime availability problems, such as a missing interface or permission, terminate with status `2`. Destructive tests that are not explicitly enabled are recorded as `SKIP` and do not fail the run.

## Destructive gates

`uds-write-did` requires `--enable-destructive`. `uds-reset` requires `--enable-reset`. These gates are independent and remain off by default:

```bash
python3 tests/hardware/run_uds_cia302_acceptance.py \
  --iface can0 --remote-node 2 \
  --enable-destructive --tests uds-write-did

python3 tests/hardware/run_uds_cia302_acceptance.py \
  --iface can0 --remote-node 2 \
  --enable-reset --bootup-id 0x702 --tests uds-reset
```

Run these only after verifying the DID is safe to write and the target is isolated from uncontrolled motion or power switching. The operator remains responsible for the external reset and power state.

## Selecting tests

Use `--tests` with one or more names to run a focused check:

| Group | Test names |
|---|---|
| UDS/ISO-TP | `uds-default-session`, `uds-extended-session`, `uds-tester-present`, `uds-read-did`, `uds-unknown-service`, `uds-multiframe`, `uds-write-did`, `uds-reset` |
| CiA 302/NMT slave/wire | `cia302-bootup`, `cia302-start`, `cia302-preop`, `cia302-stop`, `cia302-reset-node`, `cia302-broadcast-start`, `cia302-broadcast-preop`, `cia302-broadcast-stop`, `cia302-broadcast-reset-communication`, `cia302-target-reset-communication`, `cia302-targeted-isolation`, `cia302-heartbeat`, `cia302-heartbeat-timing`, `cia302-malformed-nmt`, `cia302-malformed-nmt-matrix` |

For product-specific behavior, adjust `--did`, `--multiframe-request`, `--write-did`, `--write-value`, `--reset-type`, `--timeout`, `--nmt-timeout`, `--reset-wait`, `--heartbeat-window`, `--min-heartbeats`, `--heartbeat-period`, `--heartbeat-jitter`, and `--heartbeat-max-gap` rather than modifying the test code.

The runner is an external NMT-master test tool. It validates the DUT’s NMT
slave behavior and heartbeat producer over the physical bus. Embedded CiA 302
master supervision is a separate opt-in firmware personality, built with:

```bash
cmake -S . -B build-cia302 \
  -DSTM32_CUBE_F7_DIR=/path/to/STM32CubeF7 \
  -DSTM32_F7_LINKER_SCRIPT=/path/to/STM32F767xx_FLASH.ld \
  -DCANOPEN_REFERENCE_ENABLE_CIA302_MASTER=ON
cmake --build build-cia302
```

That personality monitors the configured peer node (default node 11), exposes
bounded `network_ready`, boot-timeout, heartbeat-timeout, and invalid-frame
counters through the UART diagnostic line when
`CANOPEN_REFERENCE_UART_DIAGNOSTICS=1`, and must be tested with an independent
CANopen peer or deterministic simulator. The default image remains an NMT
slave and does not emit master commands.


## JSON results

When `--json-out` is supplied, the runner writes schema version `1` with the interface, CAN identifiers, remote node, optional `GIT_SHA`, and one result object per selected test. Each result contains the test name, `PASS`/`FAIL`/`SKIP` status, detail string, and elapsed milliseconds. Store the JSON file with the raw CAN trace, firmware metadata, and bench configuration.

The runner is intentionally a bounded diagnostic contract and does not claim to implement an embedded UDS server. It validates activated target behavior over the physical CAN link and complements the host-side contract model in `library/compat/legacy_diagnostics/uds_isotp.py`. The CiA 302 test names in this file cover external NMT control; the embedded-master path additionally requires the opt-in firmware build and peer-network evidence described above.
