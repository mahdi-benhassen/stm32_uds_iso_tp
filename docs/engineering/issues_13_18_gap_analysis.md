# Issues #13–#18 Gap Analysis — Post-Review

**Repository:** `mahdi-benhassen/stm32_uds_iso_tp`
**Reviewed branch:** `main`
**Latest local implementation revision:** `9954ccf`
**Target:** STM32C092 + FDCAN + external CAN transceiver + Keil MDK / Arm Compiler 6
**Audit date:** 2026-08-26

This document is the required first-audit artifact for the attached acceptance prompt. It separates protocol implementation, backend interface, reference backend, host tests, target compilation, and physical HIL evidence. The issues are intentionally not closed merely because host tests or a backend interface exists.

## Executive finding

The latest review found and corrected a real transport-contract defect: `uds_c092_fdcan_tx_complete()` previously reported success while the adapter was idle and had no matching TX Event FIFO record. It now reports completion only for a latched matching event. A generic `UdsIsoTpTxErrorFn` boundary was added so an aborted or failed transmission can clear in-flight protocol state and allow the next request without endpoint reinitialization. C092 polling and interrupt paths are both covered by host contracts, and endpoint recovery is tested for single- and multi-frame responses.

The review also extended the AES-CMAC helper to arbitrary bounded seed lengths, added best-effort clearing of CMAC temporary state, and added a deterministic three-record DTC fixture. Those changes improve Issues #15 and #17 substantially, but they do not create production key policy, device DTC storage, or physical validation. Issue #16 remains intentionally classified as **PROTOCOL ONLY / BACKEND REQUIRED** for the grouped services because their service-specific parsers and state machines are not yet implemented.

## Issue #13 — STM32C092 FDCAN / ECUReset

| Audit field | Finding |
|---|---|
| Current implementation | C092 Classic CAN transport builds TX headers with `FDCAN_STORE_TX_EVENTS`, rotates message markers, drains stored TX events through an ISR path or mainline polling, and exposes separate completion/error callbacks. The application masks TX-event IRQs while polling, then notifies the generic endpoint. |
| Actual request | Make the CubeMX/Keil-generated C092 application repeatedly process UDS traffic and ECUReset without stale state, power cycling, manual reinitialization, or hidden coupling. |
| Genuinely implemented | Callback/context compatibility, event correlation, idle false-completion fix, endpoint TX-error recovery, deferred reset completion, repeated-request tests, and C092 adapter syntax validation against the supplied HAL headers. |
| Only an abstraction | The C092 HAL adapter and endpoint callbacks; the generic library remains HAL-independent. |
| Only mocked | HAL enqueue, TX Event FIFO, interrupt flags, controller abort, and message marker behavior in host tests. |
| Missing | Keil/ArmClang project build and link, proof of CubeMX message-RAM TX Event FIFO element count, real controller event timing, and real reset response ordering. |
| Required tests | Completed in host contracts for success, stale/mismatched markers, missing event, abort, FIFO-full, enqueue failure, recovery, interrupt path, polling path, and endpoint valid–error–valid sequences. |
| Required HIL | Flash a selected C092 board, exchange real CAN frames, capture TX Event behavior, issue ECUReset, verify reset cause, and repeat after restart. |
| Acceptance status | **PROTOCOL ONLY**; target and HIL evidence remain required. |

## Issue #14 — Only the first request works

| Audit field | Finding |
|---|---|
| Current implementation | The supplied reporter project enabled RX FIFO notification but did not provide TX Event FIFO notification/callback wiring. The maintained adapter now also provides a polling fallback, but stored TX Event FIFO elements must exist in the generated FDCAN message RAM. The endpoint receives a generic TX-error signal and resets only protocol-owned in-flight state after an aborted transmission. |
| Actual request | Indefinite request/response reuse for single-frame, multi-frame, mixed-service, unsupported-service, malformed, timeout, wrong-sequence, overflow, TX-failure, and RX-failure traffic. |
| Genuinely implemented | No delays, retries, per-request peripheral reinitialization, or service-specific workaround; 1,000 consecutive host requests; direct ISR/polling adapter coverage; stale-event rejection; FIFO/enqueue failure handling; generic endpoint recovery for single- and multi-frame TX. |
| Only an abstraction | Host FakeBus/HAL behavior and the HIL procedure generator. |
| Only mocked | Physical arbitration, transceiver, bus-off, RX FIFO overflow, interrupt priority, message RAM, and analyzer traces. |
| Missing | The requested 10,000 randomized stress run, complete mixed-service sequence, parser fuzzing, real C092 1,000-request run, and physical error-recovery evidence. |
| Required tests | Core recovery cases are now present; broader randomized/error matrix remains to be added or run. |
| Required HIL | Power-on, 1,000 requests, mixed services, multi-frame traffic, malformed requests, timeout/recovery, ECUReset, restart, and trace capture. |
| Acceptance status | **PROTOCOL ONLY**; the original host-observable lifecycle defect is addressed, but the issue is not acceptance-complete without C092 HIL. |

## Issue #15 — AES-CMAC SecurityAccess

| Audit field | Finding |
|---|---|
| Current implementation | AES-128 and AES-CMAC-128 are dependency-free, allocation-free primitives. The helper supports a 16-byte compatibility seed and arbitrary caller-owned seeds up to 4095 bytes. Temporary CMAC round-key/block state is cleared after tag generation. Generic `0x27` remains callback-driven. |
| Actual request | Separate the primitive, `0x27` state machine, and application key policy; provide RFC 4493 vectors; do not embed production secrets; define lockout, seed lifetime, freshness, and comparison behavior. |
| Genuinely implemented | RFC 4493 empty/16/40/64-byte vectors; arbitrary, all-zero, repeated, maximum-length, and oversized-seed tests; constant-time comparison; existing startup-ready, three-attempt lockout, expiry, session, and seed-lifetime tests; no repository secret. |
| Only an abstraction | Application key ownership, provisioning, freshness/replay policy, security-level algorithm choice, and integration through the generic callback. |
| Only mocked | Deterministic reference key provider and test secrets. |
| Missing | Product key provisioning/erasure, side-channel assessment, hardware security integration, and complete product SecurityAccess policy. |
| Required HIL | Target timing/provisioning/security review if the application makes those claims. |
| Acceptance status | **BACKEND REQUIRED**; primitive and generic state behavior are host-tested, but no product security implementation is claimed. |

## Issue #16 — Remaining UDS services

| Audit field | Finding |
|---|---|
| Current implementation | Separate backend groups exist for memory, DID extension, transfer extension, timing, periodic/events, link control, authentication, and secured data. The common dispatcher enforces service attribute session/address policy and calls memory access or periodic bound preflight before the backend. |
| Actual request | Implement parser, length/subfunction/security/session/semantic validation, response generation, NRC behavior, unit/integration tests, and reference backends for all listed SIDs. |
| Genuinely implemented | Stable bounded callback signature, service-ID routing, common attribute gate, absent-backend NRC behavior, memory permission hook, and bounded periodic/event configuration. Existing core services such as `0x10`, `0x22`, `0x27`, `0x31`, `0x34`, `0x36`, `0x37`, `0x3E`, and `0x85` retain their existing handlers. |
| Only an abstraction | Group callbacks for `0x23`, `0x24`, `0x29`, `0x2A`, `0x2C`, `0x2E`, `0x35`, `0x38`, `0x3D`, `0x83`, `0x84`, `0x86`, and `0x87`. |
| Only mocked | Selector/preflight tests and simple application callbacks. |
| Missing | Service-specific parsers and reference implementations for memory regions, DID scaling/dynamic definitions, periodic/event lifecycle, upload/file transfer, authentication state, secured-data semantics, timing bounds, and physical link control. Central NRC and parser-fuzz coverage is also incomplete. |
| Required HIL | Only after an application configures concrete memory, transfer, timing, event, authentication, and link backends. |
| Acceptance status | **PROTOCOL ONLY** / **BACKEND REQUIRED**; not fully implemented. |

## Issue #17 — Complete ReadDTCInformation (`0x19`)

| Audit field | Finding |
|---|---|
| Current implementation | The DTC subsystem validates the project’s recognized subfunctions and lengths, checks capability bits, and invokes a bounded report callback. A separate clear-DTC callback handles `0x14`. A deterministic test fixture now contains three records with DTC number, status, severity, functional unit, snapshot data, extended data, filtering, unknown-record handling, and clear-all behavior. |
| Actual request | Implement or explicitly document every requested report subfunction and test zero/one/multiple records, status/severity filters, snapshots, extended data, clear/read interaction, invalid requests, unknown DTCs, and recovery. |
| Genuinely implemented | Capability-gated dispatch; bounded response checks; legacy callback compatibility; deterministic three-record `0x19` fixture; status filtering; snapshot and extended record paths; invalid length; unknown record; `0x19 → 0x14 → 0x19`; clear-group rejection. |
| Only an abstraction | Device DTC memory ownership and the generic capability/report callback. The fixture is test/reference data, not ECU nonvolatile storage. |
| Only mocked | Fixture records and clear state in host memory. |
| Missing | Complete normative response-format coverage for every ISO 14229 revision-specific subfunction and a product DTC store with persistence/status lifecycle. The supported-subfunction table must be selected against the project’s exact ISO 14229 revision. |
| Required HIL | Real ECU DTC setting, reading, filtering, clearing, and post-clear persistence behavior. |
| Acceptance status | **BACKEND REQUIRED**; reference fixture is present, but device backend and full revision-specific coverage remain. |

## Issue #18 — ECUReset

| Audit field | Finding |
|---|---|
| Current implementation | Generic UDS recognizes hard reset `0x01`, key-off/on `0x02`, soft reset `0x03`, enable rapid power shutdown `0x04`, and disable rapid power shutdown `0x05`. The response is prepared before deferred execution. Suppressed positive response, callback rejection, TX-failure/no-execution, and exactly-once host contracts exist. C092 supports hard and soft reset through its platform policy and returns NRC `0x12` for unsupported types. |
| Actual request | Recognize all five types without faking platform behavior; transmit the positive response before physical reset; do not use arbitrary delays. |
| Genuinely implemented | Protocol recognition, application/platform separation, C092 explicit partial policy, and host response-before-execution sequencing. |
| Only an abstraction | Platform reset executor and reset-event sink. |
| Only mocked | Host endpoint transport completion and reset execution callback. |
| Missing | Keil startup/link evidence, physical C092 reset response trace, reset-cause capture, and post-reset CAN communication. |
| Required HIL | Real reset trace and reset-cause verification for hard/soft reset; platform-specific evidence or NRC verification for the other types. |
| Acceptance status | **PROTOCOL ONLY**; C092 remains platform-partial and HIL-unverified. |

## Cross-cutting and quality gaps

The latest local validation passes ten normal and ten ASan/UBSan CTest contracts. ARM GCC builds the STM32F767 target, and the strict freestanding portability check passes with `ISOTP_MAX_PAYLOAD=4095`. The supplied reporter project passes the C092 adapter syntax check. The CI workflow now includes sanitizer examples, crypto formatting, and crypto static-analysis paths.

The attached prompt’s 10,000 randomized transaction run, parser fuzzing for the listed SIDs, complete central NRC suite, Keil ArmClang build, C092 flashing, analyzer traces, physical HIL, and hardware security assessment are not yet evidence. The repository’s HIL runner remains a procedure/dry-run generator. No issue should be labeled **IMPLEMENTED + HIL VERIFIED** or production-ready based on the current evidence.

## References

[1]: https://uds.readthedocs.io/en/latest/pages/user_guide/message_translation.html#readdtcinformation "py-uds ReadDTCInformation service translation documentation"
