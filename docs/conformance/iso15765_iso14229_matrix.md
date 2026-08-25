# ISO-TP and UDS conformance matrix

## Purpose and limits

This matrix is an engineering traceability artifact, not a certification statement. The requirement identifiers below are project identifiers and are not ISO clause numbers. The applicable purchased edition must be reviewed by a qualified owner before a formal conformance claim is made. ISO identifies ISO 15765-2:2016 as a transport-protocol and network-layer-services specification for diagnostic communication over CAN, including Classical CAN payloads up to eight bytes and CAN-FD payloads up to 64 bytes.[1] The current ISO 15765-2:2024 page describes the published DoCAN transport and network-layer protocol for CAN-based vehicle networks and notes that it does not decide whether Classical CAN, CAN FD, or both are required by referencing standards.[2] ISO identifies ISO 14229-1:2020 as a data-link-independent UDS application-layer specification and states that it does not specify implementation requirements; ISO 14229-1:2026 is now the replacement edition.[3]

The project therefore reports four evidence classes:

| Evidence class | Meaning |
|---|---|
| `HOST-COVERED` | A deterministic repository test exercises the behavior and passes in the current host build. |
| `TARGET-CROSS-BUILD` | The behavior is compiled into the STM32F767 target, but no silicon execution is implied. |
| `PHYSICAL-HIL` | A board, transceiver, analyzer/tester, wiring, timing, and captured trace are required. |
| `REVIEW-REQUIRED` | The requirement must be reconciled against the applicable standard edition or product specification before claiming coverage. |

## ISO-TP transport matrix (ISO 15765-2)

| ID | Requirement area | Current implementation/evidence | Status | Remaining action |
|---|---|---|---|---|
| TP-001 | Classical CAN frame data length | Eight-byte Classical CAN frame model and strict adapter contract | `HOST-COVERED` | Run physical frame-format and electrical checks on the selected F767 board |
| TP-002 | CAN-FD data lengths | Valid 8/12/16/20/24/32/48/64-byte lengths and BRS metadata | `HOST-COVERED` | Execute on a selected FDCAN-capable board and CAN-FD analyzer |
| TP-003 | Single Frame encoding | Classical CAN SF and CAN-FD escaped SF, including 62-byte data case | `HOST-COVERED` | Capture wire traces for each selected DLC boundary |
| TP-004 | First Frame and Consecutive Frame sequencing | FF/CF generation, reassembly, sequence counter validation, and 5,000-byte extended-FF host case | `HOST-COVERED` | Verify interoperability against an independent tester/analyzer |
| TP-005 | Extended First-Frame length | 32-bit big-endian extended length above 4,095 bytes; 5,000-byte test | `HOST-COVERED` | Add traces from a real CAN-FD campaign |
| TP-006 | Flow Control CTS | Explicit TX states and CTS transition to consecutive-frame sending | `HOST-COVERED` | Capture CTS-to-CF timing on target |
| TP-007 | Flow Control WAIT | Bounded WAIT counter and timeout/abort behavior | `HOST-COVERED` | Measure repeated WAIT behavior on target |
| TP-008 | Flow Control OVERFLOW | Immediate TX abort on OVERFLOW | `HOST-COVERED` | Verify peer-visible stop behavior on target |
| TP-009 | Block Size | BS validation, block counter, and block-FC transition | `HOST-COVERED` | Verify with an independent peer using BS values 0 and nonzero |
| TP-010 | STmin | Millisecond and microsecond encodings, invalid values, and reserved values are checked | `HOST-COVERED` | Measure target inter-CF spacing at configured bit rates |
| TP-011 | Flow Control addressing | FC CAN-ID validation against configured peer ID | `HOST-COVERED` | Verify acceptance/rejection on a shared physical bus |
| TP-012 | Frame validity | PCI and profile-specific DLC validation, including malformed Classical CAN and CAN-FD cases | `HOST-COVERED` | Run malformed-frame injection on target |
| TP-013 | Timing limits | TX/RX timeout tests and bounded state aborts | `HOST-COVERED` | Measure deadline behavior with target clock and bus load |
| TP-014 | Addressing modes | Normal 11-bit diagnostic identifiers are implemented; extended, mixed, and functional addressing are not claimed | `REVIEW-REQUIRED` | Select addressing requirements for the product and add a separate matrix |
| TP-015 | Network-layer service semantics | Callback endpoint exposes frame ingress/egress and timing; physical conformance is not implied | `REVIEW-REQUIRED` | Reconcile service primitives against the applicable ISO edition |

## ISO 14229-1 UDS matrix

| ID | Service area | Current implementation/evidence | Status | Remaining action |
|---|---|---|---|---|
| UDS-001 | Session Control (`0x10`) | Default/programming/extended session constants and dispatch test | `HOST-COVERED` | Verify P2/P2* timing and session transitions on target |
| UDS-002 | ECU Reset (`0x11`) | Callback-owned reset policy, positive/negative dispatch test | `HOST-COVERED` | Board reset handoff and recovery evidence required |
| UDS-003 | Read DTC Information (`0x19`) | Callback surface exists; product DTC semantics remain caller-owned | `REVIEW-REQUIRED` | Provide a product DTC contract and tests |
| UDS-004 | Read Data by Identifier (`0x22`) | Callback DID read, positive response, unsupported DID, and response-bound checks | `HOST-COVERED` | Freeze product DID list and verify target values |
| UDS-005 | Security Access (`0x27`) | Injected seed/key callbacks and deterministic test provider | `HOST-COVERED` | Replace test provider with approved production policy and security review |
| UDS-006 | Communication Control (`0x28`) | Callback-owned policy with dispatch test | `HOST-COVERED` | Define interaction with the product’s communication manager and verify target behavior |
| UDS-007 | IO Control by Identifier (`0x2F`) | Feature disabled by default in the bounded build | `REVIEW-REQUIRED` | Define safe actuator policy before enabling |
| UDS-008 | Routine Control (`0x31`) | Callback-owned routine policy with positive response test | `HOST-COVERED` | Add product routine requirements and negative cases |
| UDS-009 | Request Download (`0x34`) | Bounded callback and memory-policy seam; default host test uses an application callback | `HOST-COVERED` | Perform only with a signed, sacrificial, board-specific test plan |
| UDS-010 | Transfer Data (`0x36`) | Block sequence and callback path tested | `HOST-COVERED` | Verify Flash/program behavior only in a protected product environment |
| UDS-011 | Request Transfer Exit (`0x37`) | Callback path and response tested | `HOST-COVERED` | Verify integrity and activation policy separately |
| UDS-012 | Tester Present (`0x3E`) | Endpoint integration test and positive response path | `HOST-COVERED` | Measure S3/session timing on target |
| UDS-013 | Negative response behavior | Unsupported service, unsupported DID, callback errors, and bounded response handling | `HOST-COVERED` | Complete product NRC mapping review |
| UDS-014 | Transport composition | UDS endpoint is composed above ISO-TP with non-blocking send retry and deferred processing | `TARGET-CROSS-BUILD` | Execute full request/response traces on both target profiles |
| UDS-015 | Data-link independence | UDS callbacks do not include vendor HAL or another protocol stack | `HOST-COVERED` | Review application callback ownership for each product |

## Physical evidence package

A physical evidence package is complete only when it includes the exact firmware commit, MCU and board revision, transceiver part and mode, CAN IDs and addressing mode, nominal/data bit rates, BRS setting, message-RAM configuration for CAN FD, acceptance filters, termination and wiring, peer tester/analyzer identity, timestamped raw traces, measured latency/inter-frame timing, bus-load conditions, reset/recovery results, and a verdict for every matrix row marked `PHYSICAL-HIL`.

The current sandbox has no exposed CAN interface, USB CAN adapter, serial probe, or debug probe. Consequently, no physical result is claimed in this report. The implementation records the 0xF1–0xF9 STmin encodings at millisecond host-clock resolution; sub-millisecond precision requires a target timer and analyzer measurement. The next execution step is to connect or expose the selected hardware and provide the board profile described in [`docs/physical_validation/board_profile.yaml`](../physical_validation/board_profile.yaml).

## References

[1]: https://www.iso.org/standard/66574.html "ISO 15765-2:2016 — Road vehicles — Diagnostic communication over Controller Area Network — Part 2: Transport protocol and network layer services"
[2]: https://www.iso.org/standard/84211.html "ISO 15765-2:2024 — Road vehicles — Diagnostic communication over Controller Area Network (DoCAN) — Part 2: Transport protocol and network layer services"
[3]: https://www.iso.org/standard/72439.html "ISO 14229-1:2020 — Road vehicles — Unified diagnostic services — Part 1: Application layer"
