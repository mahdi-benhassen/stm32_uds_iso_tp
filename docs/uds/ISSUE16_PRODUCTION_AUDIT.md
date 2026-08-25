# Issue #16 Independent Production Audit

**Review role:** independent senior embedded-systems review.  
**Review date:** 2026-08-24.  
**Review baseline:** main local baseline `284e3ad` plus the uncommitted audit/documentation follow-up; the corresponding CubeMX review covers `e0a65d5` plus its synchronized working-tree changes.

This review does not infer production correctness from passing unit tests. It compares the implementation and its evidence against the intended boundaries of ISO 14229 UDS [1], ISO 15765-2 DoCAN [2], classic CAN constraints in ISO 11898 [3], and the existing CANopen architecture. The public standards define the protocol families; they do not certify this repository.

## Status legend

| Status | Meaning |
|---|---|
| **PASS** | The repository contains sufficient implementation and repeatable software evidence for the narrow stated condition. It is not a production certification. |
| **FAIL** | A tested or reviewed condition is contradicted by the implementation or an unacceptable defect is present. |
| **PARTIAL** | The software contract exists, but required coverage, target evidence, product policy, or standard scope is incomplete. |
| **NOT IMPLEMENTED** | The requested capability is intentionally absent or only an interface exists. |
| **NOT APPLICABLE** | The condition does not apply to these bare-metal branches. |

## Requirement audit

| # | Requirement | Status | Evidence reviewed | Independent finding and release consequence |
|---:|---|---|---|---|
| 1 | No CANopen regression | **PARTIAL** | `tests/integration/test_uds_canopen_coexistence.c`, FIFO0/FIFO1 configuration, both UDS-enabled ARM builds | Software filter and queue isolation are covered, but no physical high-utilization CANopen regression campaign proves deadlines, EMCY delivery, or board behavior. Do not claim production non-regression. |
| 2 | No UDS ISR blocking | **PASS** | `middleware/diagnostics/uds_stm32/uds_stm32.c`, `App/Src/canopen_reference_uds.c`, strict fake-HAL and source review | ISR work is frame copy/queue/counter handling; UDS dispatch, Flash, and reset action are mainline-owned. Keep this as a source-contract pass, not a timing pass. |
| 3 | No dynamic allocation in UDS | **PASS** | UDS/ISO-TP/adapter source review, strict builds, sanitizer runs | Diagnostic modules use static state and fixed arrays. The wider CANopen application may have its own allocation lifecycle; this status is limited to Issue #16 diagnostic code. |
| 4 | No unbounded loops | **PASS** | ISO-TP and UDS loops, bounded mainline budgets, adversarial tests | Loops are bounded by payload lengths, configured budgets, or fixed table sizes. A target watchdog campaign is still needed to validate combined execution time. |
| 5 | No buffer overflow | **PASS** | bounds checks, `test_uds_adversarial`, CTest, ASan/UBSan, strict warnings | Host evidence found no failure in exercised paths. HIL cannot prove all memory safety, and stack high-water measurements are not present. |
| 6 | Correct ISO-TP state machine | **PARTIAL** | `middleware/diagnostics/isotp`, `tests/isotp/test_isotp_contract.c`, adversarial tests | SF/FF/CF/FC, BS, STmin, sequence, timeout, overflow, and bounded FC WAIT are covered. The implementation is a deliberately narrow classic-CAN subset and lacks full ISO 15765-2 conformance evidence for every network-layer option. |
| 7 | Correct P2/P2* handling | **PARTIAL** | `UdsServer` P2/P2* fields, session/tick code, timing documentation | Server timing fields and inactivity handling exist, but no target measurements, pending-response campaign, or evidence of all service-specific P2/P2* behavior is available. |
| 8 | Correct NRC behavior | **PARTIAL** | `uds.c`, service tests, NRC documentation | Core negative-response mapping is exercised for common malformed, unsupported, sequence, security, and programming cases. Complete ISO 14229 service/session/condition matrix validation is not present. |
| 9 | Correct session transitions | **PARTIAL** | session-control implementation and core tests | Default/programming/extended session paths and inactivity state exist. A complete S3 transition matrix, timing campaign, and product-specific authorization review are pending. |
| 10 | Correct SecurityAccess behavior | **PARTIAL** | `uds_security_provider`, provider tests, security documentation | Attempt counting, lockout, constant-time compare, and session reset are tested. The checked-in provider is explicitly deterministic and **non-production**; no approved entropy, secret storage, cryptographic design, secure boot, or penetration evidence exists. |
| 11 | Correct ECU reset behavior | **PARTIAL** | deferred reset in `CO_app_STM32_reference.c`, runtime test | Reset is deferred out of UDS processing and the application is forced safe first. No physical reset, reset-cause, heartbeat restart, recovery, or post-reset HIL evidence exists. |
| 12 | Correct TesterPresent behavior | **PARTIAL** | `0x3E` service implementation and host tests | Basic response and suppression policy are present. No target bus campaign proves S3 refresh, suppression, interleaving with ISO-TP, or timeout behavior under load. |
| 13 | Correct CAN error handling | **PARTIAL** | adapter statistics, existing CAN diagnostics, fake-HAL tests | Counters and existing CANopen error hooks are present, but diagnostic error-state semantics and physical error-frame behavior have not been validated on STM32F767 hardware. |
| 14 | Correct bus-off recovery | **PARTIAL** | existing CANopen recovery state machine, adapter counters, coexistence tests | The CANopen recovery path is bounded and mainline-owned. End-to-end UDS queue flushing, filter reinstallation, callback lifecycle, and bus-off recovery on a real transceiver are not evidenced. |
| 15 | Correct watchdog interaction | **PARTIAL** | main-loop order, TIM7/watchdog code, timing hooks | UDS executes after CANopen processing and does not refresh the watchdog from an ISR. No measured watchdog margin or six-campaign target evidence proves that UDS load preserves the existing safety policy. |
| 16 | Correct Flash boundaries | **PARTIAL** | `uds_download`, linker maps, Flash documentation, download tests | Address/alignment/protection/CRC/timeout state checks are tested. Runtime Flash callbacks are intentionally `NULL`, activation is unsupported, and there is no production bootloader, signed-image verification, power-loss rollback, or endurance evidence. |
| 17 | Correct CubeMX integration | **PARTIAL** | CubeMX generated CMake build, `stm32f767_canopen.ioc`, `cubemx_integration.md` | Generated CAN handle, GPIO/NVIC/TIM7 infrastructure and FIFO1 adapter build successfully. The CubeMX pin assignment is PI9 RX/PA12 TX while main documentation historically used PA11 RX/PA12 TX; the actual board schematic must resolve this before hardware acceptance. |
| 18 | Correct FreeRTOS integration where applicable | **NOT APPLICABLE** | both branches’ bare-metal main-loop architecture | FreeRTOS is not enabled in either branch. The implementation documents a bounded bare-metal budget; no FreeRTOS claim is made. |
| 19 | Correct endian behavior | **PASS** | explicit UDS/ISO-TP field encoding/decoding, strict unit tests | Multi-byte diagnostic fields use explicit byte-order operations in the exercised core. Product DIDs and download metadata still require review when new application payloads are added. |
| 20 | Correct CAN ID filtering | **PARTIAL** | exact 11-bit IDs, FIFO0/FIFO1 code, coexistence test, ARM builds | Software filter construction and defaults are covered. Filter-bank capacity, complete network-ID collision analysis, transceiver acceptance behavior, and target capture evidence are pending. |
| 21 | Correct coexistence with CANopen | **PARTIAL** | independent modules, FIFO separation, concurrent runner inventory, host coexistence test | The ownership boundary is sound in source and host tests. The requested concurrent NMT/heartbeat/SDO/PDO/EMCY campaign at real bus load has not been run on target hardware. |
| 22 | Correct documentation | **PASS** | all required files under `docs/uds/`, README/BUILD/CHANGELOG updates | The required documentation tree exists on both working trees and explicitly records limitations, branch differences, memory snapshots, services, IDs, timing, security, Flash, CubeMX, bare-metal integration, HIL, and troubleshooting. |

## Validation performed for this review

The following evidence was executed locally on both branches: strict GCC builds with `-Wall -Wextra -Wpedantic -Wconversion -Werror`; the UDS/ISO-TP host Makefile suite; nine-test CMake/CTest suites; AddressSanitizer/UndefinedBehaviorSanitizer tests; clang-format; clang-tidy; cppcheck; Python dry-run acceptance inventories; and UDS-enabled ARM cross-builds. The main build reported `text=49,476`, `data=3,740`, `bss=24,784`; the CubeMX build reported `text=43,012`, `data=3,660`, `bss=23,476`.

These results are **software and build evidence only**. No actual STM32F767 board, independent CAN interface connected to a target, oscilloscope/analyzer capture, bus-error injection, watchdog timing measurement, Flash power-loss test, or production security review was performed in this session. The timing implementation records maxima through DWT hooks, but it does not yet retain the required minimum, p95, p99, and worst-observed distributions.

## Release decision

Issue #16 is **not ready for production closure**. The current reference configuration now enables the bounded UDS profile by default, but the mandatory release blockers remain: physical HIL and CANopen coexistence evidence, target timing distributions and deadline proof, bus-off/error and watchdog campaigns, board-pin reconciliation, a production SecurityAccess provider, and a bootloader-backed authenticated Flash activation/recovery design. Destructive operations remain denied until product-owned callbacks and authorization are installed; products that do not want diagnostics must explicitly configure `CANOPEN_REFERENCE_ENABLE_UDS=OFF`.

## References

[1]: https://www.iso.org/standard/72439.html "ISO 14229-1:2020 — Road vehicles — Unified diagnostic services (UDS) — Part 1: Application layer"
[2]: https://www.iso.org/standard/66574.html "ISO 15765-2:2016 — Road vehicles — Diagnostic communication over CAN — Part 2: Transport protocol and network layer services"
[3]: https://www.iso.org/standard/63648.html "ISO 11898-1 — Road vehicles — Controller area network — Data link layer and physical signalling"
[4]: https://uds.readthedocs.io/en/v0.1.0/autoapi/uds/messages/nrc/index.html "UDS negative-response-code reference"
[5]: architecture.md "Issue #16 diagnostic architecture"
[6]: isotp.md "Issue #16 ISO-TP contract"
[7]: timing.md "Issue #16 timing evidence requirements"
[8]: hil_testing.md "Issue #16 HIL evidence requirements"
