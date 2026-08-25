# UDS Validation and Acceptance Plan

## Evidence hierarchy

The repository separates deterministic software tests from target and product evidence. A passing host test demonstrates a software contract; it does not demonstrate electrical interoperability, exact interrupt latency, transceiver behavior, EMC robustness, or production security.

| Evidence layer | Command or procedure | Claim supported |
|---|---|---|
| ISO-TP/UDS unit tests | `tests/uds/test_uds_core.c`, `test_uds_adversarial.c`, DID, SecurityAccess, and download tests | Deterministic state-machine, bounds, NRC, sequence, timeout, and callback contracts. |
| Adapter host tests | `tests/uds/test_uds_stm32.c`, `test_uds_runtime.c` | Queue, identifier, mailbox, and runtime integration behavior using fake HAL. |
| Static analysis | `cppcheck` over project-owned C sources, Python repository validators | Warning, performance, portability, configuration, and source-policy checks. |
| Firmware build | CMake with the Arm GNU toolchain and exact STM32CubeF7 inputs | Compile and link correctness for the selected STM32F767 project. |
| SocketCAN | `tests/hardware/run_uds_cia302_acceptance.py` on a Linux CAN interface | Wire-level ISO-TP and UDS behavior on a configured CAN network. |
| STM32F767 HIL | `tests/hardware/run_stm32f767_uds_acceptance.py` with an approved rig | Target behavior, reset/power state, timing, CAN error handling, and hardware interoperability. |
| Production qualification | Product-specific safety, cybersecurity, EMC, Flash, and manufacturing evidence | Release decision for a real product. |

## Required host checks

Run the following from the repository root before creating a review or release candidate:

```sh
python3 tests/test_firmware_configuration.py
python3 tests/test_canopen_wire_contract.py
python3 tests/conformance/run_core_vectors.py
PYTHONPATH=.:tests python3 tests/run_uds_isotp_contract.py
python3 scripts/validate_repository.py
cppcheck --enable=warning,performance,portability --error-exitcode=1 \
  --inline-suppr --suppress=missingIncludeSystem --suppress=unusedFunction \
  --std=c11 -I App/Inc -I middleware/canopen/core \
  -I library/compat/legacy_diagnostics/isotp -I library/compat/legacy_diagnostics/uds \
  -I library/compat/legacy_diagnostics/uds_stm32 App/Src library/compat/legacy_diagnostics
PYTHONPATH=tests/hardware python3 tests/hardware/run_stm32f767_uds_acceptance.py --dry-run
```

The adversarial tests must cover malformed single and first frames, impossible lengths, wrong sequence counters, reserved STmin values, flow-control overflow, timeout, zero-capacity response buffers, malformed UDS request lengths, unsupported services, and protected or unavailable download targets. The checked-in tests provide deterministic coverage for those categories; products must extend them for their enabled DIDs and callbacks.

## Hardware procedure

Before connecting a target, record board revision, MCU marking, transceiver, CAN bitrate, sample point, oscillator, termination, power supply, firmware SHA, tool version, and operator. Start with non-destructive checks: session control, TesterPresent, ReadDataByIdentifier, negative-response policy, and SecurityAccess seed behavior. Do not enable reset or download checks on an uncontrolled or irreplaceable target.

The HIL runner defaults to a non-destructive suite and produces a JSON result when `--json-out` is supplied. `--enable-reset` is required for reset testing, and `--enable-download` is required for download policy testing. The target must have an approved recovery path before either option is enabled.

Timing qualification must capture DWT maxima for the TIM7 ISR, CAN IRQ contexts, UDS RX handoff, ISO-TP processing, UDS dispatch, UDS TX, and total UDS mainline processing. The measured values are evidence inputs, not automatic pass/fail claims. Acceptance thresholds must be approved for the exact clock, CAN load, interrupt priorities, transceiver, application profile, and board revision.

## Release decision

A release claim is allowed only when the selected product profile, enabled UDS services, DID permissions, SecurityAccess provider, download policy, linker map, hardware acceptance, timing measurements, and production security controls are all identified and evidenced. The reference repository intentionally does not claim a complete bootloader, cryptographic update chain, or certification to the latest ISO editions [1] [2].

## References

[1]: https://www.iso.org/standard/72439.html "ISO 14229-1:2020 — Road vehicles — Unified diagnostic services (UDS) — Part 1: Application layer"

[2]: https://www.iso.org/standard/66574.html "ISO 15765-2:2016 — Road vehicles — Diagnostic communication over CAN — Part 2: Transport protocol and network layer services"
