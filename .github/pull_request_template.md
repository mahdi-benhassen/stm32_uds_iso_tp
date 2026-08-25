## Summary

Describe the change and why it is needed.

## Scope

- [ ] Firmware runtime
- [ ] CANopen protocol or Object Dictionary
- [ ] Hardware or board integration
- [ ] Host tooling or tests
- [ ] CI/build system
- [ ] Documentation only

Affected profiles, CAN interfaces, configuration flags, and OD objects:

## Validation

- [ ] `python3 scripts/validate_od.py`
- [ ] `python3 scripts/validate_cia418.py`
- [ ] Firmware configuration tests
- [ ] CANopen wire-contract tests
- [ ] UDS/ISO-TP or gateway contract tests, when applicable
- [ ] Host C tests
- [ ] Affected ARM firmware personality build
- [ ] `git diff --check`
- [ ] HIL or SocketCAN test procedure, when applicable

Commands and results:

## Hardware and release impact

Describe board assumptions, wiring, timing, safe-state behavior, generated artifacts, release notes, and any tests that could not be run.

## Checklist

- [ ] No secrets or build artifacts are included.
- [ ] Generated Object Dictionary files match their source EDS/XDD.
- [ ] Default safety and authorization gates remain unchanged unless explicitly justified.
- [ ] Public APIs and documentation are updated.
- [ ] The change is limited to the stated scope.
