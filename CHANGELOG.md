# Changelog

All notable changes to this project are documented here. The project follows semantic-versioning principles for tagged reference baselines; firmware conformance and product compatibility still depend on the exact board, Object Dictionary, toolchain, and build personality.

## [Unreleased]

### UDS Issue #16 reference implementation

- Added a protocol-independent classic-CAN ISO-TP/UDS reference subset enabled by default in the current reference build, with static buffers, bounded timers, FIFO1 STM32 adapter, DID registry, replaceable SecurityAccess provider, and callback-driven download architecture.
- Added host and fake-HAL tests for ISO-TP, UDS services, DIDs, security policy, download state transitions, runtime integration, adversarial inputs, and CANopen coexistence.
- Added the required [`docs/uds/`](docs/uds/) architecture, ISO-TP, service, configuration, CAN-ID, timing, security, Flash, STM32F767, CubeMX, HIL, and troubleshooting guides.
- Actual target timing distributions, physical HIL, bus-error/watchdog campaigns, production cryptography, and bootloader-backed Flash activation remain external evidence gates; this release does not claim complete ISO 14229/ISO 15765-2 conformance.

### Review hardening

- Enforced `-Wconversion` as a hard error for project-owned sources in the production build; the known third-party narrowing idioms are contained by a single project include boundary (`canopen_reference_co.h`) instead of a global `-Wno-error=conversion` escape hatch.
- Extracted the bxCAN acceptance-list policy into the transport-neutral `CANopenAcceptanceFilter_Add()` helper with host unit tests covering masking, de-duplication, global-ID rejection, and capacity bounds.
- Added post-mortem fault capture: `Error_Handler()` now records a fatal-fault code in RTC backup registers, and `CANopenReferenceWatchdog_PreviousFault()` exposes it after reboot even when the optional IWDG is disabled.

### Audit remediation

- Propagated STM32 HAL CAN errors into CANopenNode diagnostics and added bounded hardware-fault counters.
- Added regression assertions for CAN1/TIM7 interrupt priorities and corrected the default 500 kbit/s sample point to 83.33%.
- Added validated standalone-facade bitrate selection for common 54 MHz APB1 configurations.
- Enabled OD 1010h/1011h storage through a project-owned, CRC-validated persistence seam with weak board hooks.
- Added an opt-in dual-rate IWDG supervisor that requires both TIM7 and mainline progress.
- Replaced the bxCAN accept-all filter with node-specific CANopen and LSS acceptance lists.
- Replaced the RAM-only default persistence path with a CRC-validated two-slot STM32F7 Flash backend and reserved the final 512 KiB in the linker memory map.
- Added bounded CAN bus-off STOP/WAIT/REINIT/FAULT recovery, safe-fault latching on failed communication reset, and recovery diagnostics.
- Derived bxCAN acceptance filters from active OD PDO/SDO mappings while retaining mandatory CANopen and LSS traffic.
- Added bounded timeout behavior and explicit `-ENOTSUP` failure for unsupported standalone CAN targets.
- Hardened the opt-in IWDG path with startup grace, LSI readiness bounds, timer-progress deadlines, mainline-only refresh, and reset-cause capture.
- Added deterministic recovery state-machine tests and source contracts for error mapping, Flash reservation, filters, transport deadlines, watchdog behavior, and lifecycle ordering.
- Added `docs/final_status_remediation_2026.md`, which separates implemented reference controls from required HIL, electrical, endurance, and formal-conformance evidence.
- Added `docs/feature_matrix.md` and expanded the profile documentation with explicit CiA 401 timing/debounce/scaling and CiA 402 mode boundaries.
- Added six machine-readable core CANopen regression vectors with a deterministic software-only validator; these are not official conformance evidence.
- Added JSON reproducibility manifests containing source, submodule, toolchain, personality, Object Dictionary, and linker hashes, with CI and local validation checks.
- Added explicit storage slot-size/write-rate contracts, filter-capacity overflow checks, observable runtime lifecycle states, and corresponding source-contract coverage.
- Expanded the deterministic CANopen corpus to 105 vectors covering NMT, heartbeat, EMCY, SDO transfer variants and aborts, PDO/mapping, SYNC, TIME, LSS, reset, invalid OD, timeout, bus-off, controller errors, and recovery, with category-count validation.
- Added host AddressSanitizer/UndefinedBehaviorSanitizer and gcov targets and executed them in CI and the local reproducible validation script.
- Added a capability-gated release-tag SocketCAN job that runs native integration tests when `vcan0` is available and otherwise executes the deterministic regression fallback while preserving an explicit unavailable evidence status.
- Added `docs/production_validation_plan.md` with objective physical CAN, bus-off, Flash power-loss/endurance, watchdog timing, CiA 401/402/302/LSS, security, and formal-conformance evidence procedures.
- Added source contracts for release gating, hardening targets, local validation alignment, and production evidence artifacts.
- Added release JUnit XML, gcov JSON coverage, sanitizer, reproducibility-manifest, OD-hash, and target memory-budget artifact validation to CI.
- Added explicit host coverage thresholds of 90% line, 95% function, and 85% branch, raised by real CiA 302 and CAN-recovery boundary tests; the measured baseline is 97.97%/100%/87.32%.
- Added a structured libFuzzer-compatible CAN-frame harness and a build-only `test-fuzz` target with truthful toolchain boundaries.
- Added pinned clang-tidy analysis for project-owned protocol sources and a compiler-hardening flag evaluation report; target firmware adoption remains a separate build review.
- Recorded `v0.9.0` as an immutable historical hardware-validation candidate; current qualification work must reference the superseding `v0.9.0-rc2` lineage and remain blocked on external evidence gates. See [`docs/release_v0.9.0_historical.md`](docs/release_v0.9.0_historical.md) and [`docs/v1_release_readiness_gate.md`](docs/v1_release_readiness_gate.md).

The reference firmware remains a non-certified baseline. Production release still requires board-specific Flash endurance and power-loss testing, watchdog timing measurement, physical CAN/HIL validation, EMC testing, and applicable official conformance evidence.

## [0.1.0] - 2026-08-15

### Added

- STM32F767 CAN1/bxCAN CANopenNode reference firmware with a CiA 401 default personality.
- Optional CiA 402 drive-control reference, CiA 302 NMT-master adapter, and bounded gateway foundations.
- CMake ARM builds, host protocol tests, deterministic contract tests, static analysis, and GitHub Actions validation.
- UDS/ISO-TP and CiA 302/NMT hardware acceptance runner and procedure.
- Build, hardware, security, contribution, and third-party dependency documentation.

### Notes

- The repository is a reference implementation, not a device-profile, functional-safety, or production-board certification.
- The exact release artifact must retain its build manifest, Object Dictionary, linker script, dependency revisions, and hardware validation evidence.
