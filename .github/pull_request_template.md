Describe the change and why it is needed.

## Scope

- [ ] ISO-TP transport behavior
- [ ] UDS service or application callback behavior
- [ ] CAN/CAN-FD transport abstraction
- [ ] STM32 adapter or board integration
- [ ] Host tooling or tests
- [ ] CI/build system
- [ ] Documentation only

Affected transport profiles, CAN identifiers, bounds, callbacks, and target assumptions:

## Architecture

- [ ] No CANopen headers, sources, object-dictionary concepts, or protocol symbols were introduced.
- [ ] Protocol code remains independent of vendor HAL and application policy.
- [ ] Interrupt paths remain bounded, non-blocking, and free of UDS dispatch or Flash operations.

## Validation

- [ ] `python3 tests/architecture/check_standalone_architecture.py`
- [ ] Standalone CMake configure/build
- [ ] CTest contracts
- [ ] Sanitizer build and test
- [ ] clang-format
- [ ] clang-tidy
- [ ] cppcheck
- [ ] STM32F767 cross-build, when target code is affected
- [ ] HIL or SocketCAN test procedure, when applicable
- [ ] `git diff --check`

Commands and results:

## Hardware and release impact

Describe board assumptions, transceiver and wiring, timing, safe-state behavior, generated artifacts, release notes, and tests that could not be run. State explicitly when evidence is host-only or when physical CAN-FD HIL remains pending.

## Checklist

- [ ] No secrets or generated build artifacts are included.
- [ ] Public APIs and documentation are updated.
- [ ] Bounds and malformed-input behavior are covered by tests.
- [ ] The change is limited to the stated scope.
- [ ] Production security, Flash, reset, and authentication behavior are not implied by host tests.
