# Contributing

Thank you for improving the STM32F767 CANopen reference. Contributions should preserve deterministic timing, explicit ownership boundaries, safe default behavior, and reproducible builds.

## Before making a change

Read the [README](README.md), [BUILD.md](BUILD.md), [third-party inventory](THIRD_PARTY.md), the [documentation map](docs/README.md), and the relevant hardware procedure. For third-party Object Dictionary requests, follow [Handling third-party OD requests](docs/handling_third_party_od_requests.md). Confirm whether the change affects the Object Dictionary, CAN wire format, generated code, safety defaults, or a device-profile contract.

Do not commit secrets, private keys, vendor credentials, proprietary board files, generated build output, or a local STM32CubeF7 checkout. Keep application changes in `App/` or project-owned middleware and do not modify the pinned third-party stack unless the change is explicitly justified.

## Development workflow

1. Create a topic branch from `main`.
2. Make one logically complete change at a time.
3. Add or update deterministic tests for pure logic and source contracts.
4. Run the host validation and the relevant ARM personality build.
5. Update the documentation, Object Dictionary, release notes, and hardware procedure when the public behavior changes.
6. Open a pull request using the repository template.

Generated Object Dictionary files must be reviewed together with their source EDS/XDD. A changed CAN-ID, PDO map, SDO access rule, identity value, timing value, or profile index is an interface change and requires explicit review.

## Coding conventions

Use C11-compatible code for project-owned C. Prefer fixed-width integer types, explicit bounds, checked return values, and static storage where practical. Keep CAN and timer interrupt work bounded and non-blocking. Do not allocate memory, write NVM, format strings, or perform blocking I/O from an ISR.

Use the repository `.clang-format` configuration for C formatting. Public interfaces should have concise Doxygen-style comments that state ownership, timing context, valid ranges, and failure behavior. Python code should use the standard library where possible, explicit argument validation, and deterministic output suitable for CI.

## Validation commands

Run these checks from the repository root:

```sh
python3 scripts/validate_od.py
python3 scripts/validate_cia418.py
python3 scripts/validate_inventus_battery.py
python3 scripts/mock_canopen_runner.py
python3 tests/test_firmware_configuration.py
python3 tests/test_canopen_wire_contract.py
python3 tests/run_uds_isotp_contract.py
python3 tests/run_nmea2000_gateway_contract.py
make -C tests/host all test-stm32-facade test-gateway-default-deny test-inventus-battery test-mock-canopen
```

For a target build, follow [BUILD.md](BUILD.md). For the complete local sequence, run `bash scripts/validate_reference.sh`; it includes the Inventus validator, mock protocol runner, host targets, contract checks, and ARM personality builds. The mock runner’s scope and limitations are documented in [In-process CANopen protocol smoke testing](docs/mock_canopen_protocol_smoke_testing.md). For physical CAN testing, use the procedure in `docs/hardware/uds_cia302_test_procedure.md` and attach the JSON result and trace evidence to the pull request or release record.

## Commit and pull-request format

Use imperative, scoped commit subjects such as:

```text
feat(cia402): add bounded homing-state adapter
fix(can): reject invalid receive DLC
ci: build the opt-in CiA 302 personality
docs: clarify CubeMX ownership boundaries
```

A pull request should explain the behavior change, list affected profiles or Object Dictionary objects, identify the validation performed, and state any hardware tests that could not be run. Separate unrelated refactors from functional changes.

## Review and merge requirements

A change is ready for merge when the relevant deterministic tests pass, the affected firmware personality builds, `git diff --check` is clean, documentation is updated, and reviewers understand any hardware or conformance limitations. Changes involving safety outputs, gateway authorization, persistent storage, or production identity require explicit product-owner review before release use.
