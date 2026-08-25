# Changelog

All notable changes to this project are documented here. Host validation and target compatibility remain dependent on the exact compiler, MCU, HAL revision, transceiver, and board configuration.

## [Unreleased]

### Standalone architecture

- Completed the migration to an independent ISO 15765-2 ISO-TP and ISO 14229 UDS repository.
- Removed the unrelated inherited protocol stack, profile artifacts, object-dictionary files, gateway tooling, and associated build and test dependencies.
- Added a minimal STM32F767 bxCAN application with explicit diagnostic identifiers, deferred RX processing, injected timing, and application-owned UDS callbacks.
- Retained only STM32F7 HAL/CMSIS material needed for the target build and documented its upstream licensing requirements.

### ISO-TP and UDS validation

- Added explicit ISO-TP transmit states with CTS, bounded WAIT, immediate OVERFLOW abort, BS/STmin validation, reserved-STmin rejection, CAN-ID and DLC validation, timeout handling, and consecutive-frame sequence checks.
- Covered Classical CAN and CAN-FD profiles, including valid 64-byte data lengths, Single-Frame escape behavior, and extended First-Frame payloads above 4,095 bytes.
- Added strict CMake/CTest, sanitizer, gcovr coverage, clang-format, clang-tidy, cppcheck, architecture, adapter-contract, and safety-gated HIL dry-run checks.
- Kept production SecurityAccess, Flash activation, authenticated boot, and physical CAN-FD HIL as explicit product-owned evidence gates.

## [0.1.0] - 2026-08-15

### Added

- Bounded heap-free ISO-TP and UDS core APIs for Classical CAN and CAN FD.
- STM32F767 bxCAN and FDCAN-capable STM32 adapter contracts.
- Host tests, sanitizer validation, static analysis, HIL inventory tooling, and target build infrastructure.
- Documentation for architecture, transport profiles, UDS boundaries, STM32 integration, HIL, safety, and release readiness.

### Notes

- The repository is an engineering baseline, not a production-board certification or a formal ISO 15765-2 / ISO 14229 conformance claim.
- Physical electrical validation, timing evidence, EMC testing, production cryptography, Flash power-loss testing, and authenticated firmware activation require a board-specific validation program.
