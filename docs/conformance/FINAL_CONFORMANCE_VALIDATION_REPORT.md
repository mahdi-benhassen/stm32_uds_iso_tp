# Final conformance and physical-validation report

## Executive result

The architecture remained frozen. The work added only standards-oriented traceability, conformance vectors, physical HIL procedures, machine-readable campaign data, validation-asset linting, and HIL report provenance. No CANopen dependency, alternate authoritative implementation, or major source-tree restructuring was introduced.

The local validation-only commit is `7cea102ff53476009572b27c96cfd4389a399762` (`test: add conformance and physical validation evidence`). The repository working tree is clean. The commit is present locally but was not published to GitHub because both Git HTTP and GitHub API publication attempts returned HTTP 403 / `Resource not accessible by integration`; the remote `main` remains `75a09d11921d457e6b392571a30212138c6258cc`.

## Added evidence assets

| Asset | Purpose |
|---|---|
| `docs/conformance/iso15765_iso14229_matrix.md` | Standards-oriented ISO-TP and UDS traceability matrix with explicit evidence classes and no certification claim |
| `docs/conformance_sources.md` | Official ISO metadata and scope notes for ISO 15765-2:2016, ISO 15765-2:2024, and ISO 14229-1:2020 |
| `tests/conformance/conformance_vectors.json` | 13 machine-readable vectors tied to existing host CTest contracts |
| `tests/conformance/check_validation_assets.py` | Deterministic lint for the matrix, vectors, board profile, and HIL plan |
| `docs/physical_validation/README.md` | Board setup, wiring, safety, execution, capture, and simulation boundaries |
| `docs/physical_validation/board_profile.yaml` | F767 bxCAN profile plus fields for the selected FDCAN target and analyzer |
| `docs/physical_validation/evidence_template.md` | Per-campaign provenance, timing, trace, safety, and verdict template |
| `tests/physical/hil_test_plan.json` | 18-case machine-readable Classical CAN/CAN-FD campaign plan with destructive cases explicitly gated |
| `tests/standalone/run_uds_iso_tp_hil.py` | HIL reports now include commit, board-profile, analyzer, trace path, and trace/profile SHA-256 provenance |

## Local validation evidence

| Gate | Result |
|---|---|
| Standalone architecture check | PASS; 159 tracked paths checked |
| Conformance/physical asset lint | PASS; 13 conformance vectors and 18 physical cases |
| STM32F767 ARM cross-build | PASS |
| Strict standalone CTest | PASS; 4/4 tests |
| ASan/UBSan CTest | PASS; 3/3 tests |
| Instrumented coverage | PASS; 54% total source-line coverage, including 86% `isotp.c` and 67% `uds.c` |
| clang-format | PASS |
| clang-tidy | PASS; diagnostics suppressed in non-user/system code |
| cppcheck | PASS; only the existing informational too-many-configurations note |
| Classical CAN HIL dry run | PASS as report generation only; 15 cases marked `DRY_RUN` |
| CAN-FD HIL dry run | PASS as report generation only; 18 cases marked `DRY_RUN` |
| Physical CAN HIL | NOT RUN; no hardware exposed |
| SocketCAN/vcan simulation | NOT RUN; temporary `vcan0` creation returned `Operation not permitted` |

## Standards boundary

The official ISO pages were used as scope and edition metadata only. ISO 15765-2:2024 is the published fourth edition for DoCAN transport and network-layer services; the ISO page notes that referencing standards decide whether Classical CAN, CAN FD, or both are required. ISO 14229-1:2020 describes data-link-independent UDS application-layer requirements and states that it does not specify implementation requirements. The applicable purchased editions must be reconciled by a qualified reviewer before any formal conformance claim.

The matrix deliberately separates host contract evidence, target cross-build evidence, physical HIL evidence, and review-required requirements. The implementation’s `0xF1–0xF9` STmin handling is recorded as a millisecond host-clock limitation; sub-millisecond precision requires target timer and analyzer measurement. The UDS endpoint remains bounded by its configured 16-bit/maximum request-response lengths and is not described as unbounded.

## Physical campaign status

No physical CAN interface, USB CAN adapter, serial probe, debug probe, or CAN-FD board was available in the execution environment. No board, transceiver, bus wiring, timing, peer tester, analyzer trace, or target runtime result is therefore claimed. The STM32F767 target is Classical CAN through bxCAN; it must not be treated as CAN-FD capable. CAN-FD evidence requires a separately selected FDCAN-capable target and compatible transceiver/analyzer.

The operator must fill `docs/physical_validation/board_profile.yaml`, attach raw timestamped traces and hashes, and complete `docs/physical_validation/evidence_template.md`. Initial bring-up must keep download/Flash, reset, and other destructive cases disabled. Any such test requires explicit operator approval, sacrificial hardware/image, power and recovery controls, and post-test integrity verification.

## References

[1]: https://www.iso.org/standard/66574.html "ISO 15765-2:2016 — Road vehicles — Diagnostic communication over Controller Area Network — Part 2: Transport protocol and network layer services"
[2]: https://www.iso.org/standard/84211.html "ISO 15765-2:2024 — Road vehicles — Diagnostic communication over Controller Area Network (DoCAN) — Part 2: Transport protocol and network layer services"
[3]: https://www.iso.org/standard/72439.html "ISO 14229-1:2020 — Road vehicles — Unified diagnostic services — Part 1: Application layer"

## Credential hygiene

Previously exposed GitHub tokens should be revoked and rotated. No token was stored in the repository or included in this report.
