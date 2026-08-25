# Standalone Release-Readiness Audit

## Scope and provenance

The repository’s first commit is an exact tree snapshot of the frozen `stm32f767_canopen_cubemx` baseline from `stm32_canopen_reference`. The standalone implementation is layered on top of that snapshot and is authoritative only under `library/`, `examples/`, `tests/standalone/`, and `docs/standalone/`. The original CubeMX firmware and its CANopen application remain present for later integration work.

## Implemented gates

| Area | Evidence in this repository | Status |
|---|---|---|
| Classical CAN ISO-TP | Fixed 8-byte frames, SF/FF/CF/FC, sequence checks, timers, bounded WAIT | Host-tested |
| CAN FD ISO-TP | Valid 8/12/16/20/24/32/48/64-byte lengths, SF escape, BRS metadata | Host-tested |
| Extended First Frame | 32-bit big-endian length form for payloads above 4,095 bytes; 5,000-byte reassembly test | Host-tested |
| UDS composition | Callback-based server with fixed response storage and endpoint integration | Host-tested |
| UDS boundary | UDS dispatcher and callbacks use `uint16_t`; compile-time checks reject `UDS_MAX_*_LENGTH` above `UINT16_MAX` | Explicitly bounded |
| STM32F767 | bxCAN/Classical-CAN adapter contract rejects CAN-FD frames | Compile-checked |
| FDCAN-capable STM32 | FDCAN adapter contract carries actual data length and BRS metadata; no fake vendor project files | Compile-checked |
| Static analysis | Strict warnings, clang-format, cppcheck, and sanitizer workflow gates | CI-configured |
| HIL safety | Dry-run inventory, JSON/CSV/Markdown reports, destructive cases disabled by default | Script-tested |

## Evidence that is not yet present

A host build cannot prove electrical interoperability, CAN transceiver behavior, message-RAM configuration, interrupt latency, timing tolerance, bus-off recovery, or vendor HAL correctness. In particular, the FDCAN example is an adapter contract and not a claim that a specific STM32G4, STM32H5, or STM32H7 board has already been flashed and exercised. A real CAN-FD HIL campaign must select a concrete board, generated CubeMX project, transceiver, nominal/data bit rates, wiring, and peer ECU or analyzer.

The UDS implementation is not a complete production bootloader. SecurityAccess is policy-injected, the checked-in deterministic provider is for tests, and Flash erase/program/activation, authenticated image validation, rollback, anti-rollback, and reset handoff remain product-owned work. Download-related HIL cases are safety-gated and do not write memory by default.

## Release decision

The standalone repository is suitable for a **host-validated engineering baseline** and for board-specific adapter integration. It is not yet suitable for a claim of complete ISO 15765-2 or ISO 14229 conformance, production diagnostic security, or validated CAN-FD hardware support. Those claims require a requirements matrix, negative/interoperability testing, physical evidence, review of enabled optional features, and product security sign-off.

## Optional integration path

After the standalone repository has a green hosted CI run, the original reference can consume it as an optional dependency through a pinned subtree, submodule, or fetched source archive. The recommended first integration keeps the frozen in-tree diagnostics as a compatibility path and adds a `CANOPEN_REFERENCE_USE_STANDALONE_UDS` CMake option. Only after both paths build and pass their existing tests should the duplicated implementation be retired. This integration is intentionally not included in the standalone publication commit.
