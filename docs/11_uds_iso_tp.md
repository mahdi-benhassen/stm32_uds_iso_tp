# UDS and ISO-TP Reference Architecture

## Scope and conformance boundary

This repository provides a **bounded reference subset** of Unified Diagnostic Services (UDS) over classic CAN. The application layer is modeled after the data-link-independent service concepts described by ISO 14229-1 [1]. The transport and network behavior is a static-buffer ISO-TP subset aligned with the CAN diagnostic transport described by ISO 15765-2 [2].

The implementation is not a certification claim and does not include every service, parameter encoding, timing option, security mechanism, or network-management requirement from the current standards. ISO’s public pages identify the 2020 ISO 14229-1 edition and 2016 ISO 15765-2 edition as withdrawn, with newer editions available [1] [2]. Product teams must select the applicable edition, obtain the normative text, and complete product-specific conformance and safety evidence.

## Layering

The diagnostic path is deliberately separated into four layers:

| Layer | Repository component | Responsibility |
|---|---|---|
| CAN controller integration | `middleware/diagnostics/uds_stm32/` | Bounded ISR-to-mainline RX queue, FIFO1 filtering, non-blocking mailbox submission, overflow and timeout statistics. |
| ISO-TP | `middleware/diagnostics/isotp/` | Classic-CAN single-frame and multi-frame segmentation, flow control, block size, STmin, sequence counters, and timeouts. |
| UDS server | `middleware/diagnostics/uds/uds.c` | Service dispatch, sessions, NRC encoding, response-size checks, and callback contracts. |
| Product policy | `uds_did.*`, `uds_security_provider.*`, `uds_download.*`, `App/Src/canopen_reference_uds.c` | DIDs, access permissions, security provider, memory policy, callbacks, and board lifecycle decisions. |

The ISR layer copies frames and publishes them to a bounded queue. It must not run UDS callbacks, access Flash, perform long computations, or call diagnostic application code. The mainline invokes `CANopenReference_UDS_Process()` after `CO_process()`, which drains a fixed budget, performs protocol work, and submits a fixed TX budget.

## Classic-CAN addressing

The reference defaults are configurable at compile time:

| Direction | Default identifier | Configuration symbol | Queue/filter path |
|---|---:|---|---|
| Tester to ECU request | `0x7E0` | `UDS_RX_CAN_ID` / `CANOPEN_REFERENCE_UDS_RX_CAN_ID` | CAN FIFO1, then ISO-TP RX. |
| ECU to tester response and ECU flow-control input | `0x7E8` | `UDS_TX_CAN_ID` / `CANOPEN_REFERENCE_UDS_TX_CAN_ID` | CAN FIFO1, then ISO-TP TX flow-control handling. |

The identifiers must be distinct standard 11-bit IDs. A product must reserve them in its network-level identifier plan and verify that they do not collide with CANopen COB-IDs, LSS traffic, gateway traffic, or other diagnostics.

## ISO-TP state machines

The RX state machine accepts a single frame when the payload fits in one classic CAN frame. For larger payloads it accepts a first frame, validates the declared length against `ISOTP_MAX_PAYLOAD`, emits a flow-control frame, and then accepts consecutive frames with monotonically advancing four-bit sequence counters. A block-size flow-control frame is emitted when the configured block count is reached. Any wrong identifier, malformed DLC, invalid length, sequence mismatch, overflow, or timeout is rejected or resets the receive state.

The TX state machine emits a single frame for short responses. For larger responses it emits a first frame, waits for flow control from the tester, honors block size and STmin, and emits consecutive frames from a static payload buffer. Unsupported STmin encodings are rejected. The millisecond implementation treats microsecond STmin encodings conservatively as at least one millisecond, so it never intentionally transmits earlier than the requested separation time.

The protocol implementation uses no heap allocation. `ISOTP_MAX_PAYLOAD` is 4095 bytes, the classic 12-bit ISO-TP length ceiling, and every frame and payload copy is bounded by compile-time capacities. A larger application response must be rejected through the UDS response-size contract rather than truncated.

## UDS service surface

The current server dispatcher exposes compile-time service gates for the following reference paths:

| SID | Service | Reference behavior |
|---:|---|---|
| `0x10` | DiagnosticSessionControl | Default, programming, and extended sessions with P2/P2* timing fields. |
| `0x11` | ECUReset | Callback-controlled and marked pending only after a positive response is prepared. |
| `0x19` | ReadDTCInformation | Callback-provided bounded DTC payload. |
| `0x22` | ReadDataByIdentifier | Table-driven DID registry with session/security permissions. |
| `0x27` | SecurityAccess | Replaceable seed/key provider with lockout and delay state. |
| `0x28` | CommunicationControl | Product callback; unsupported in the default reference runtime. |
| `0x2E` | WriteDataByIdentifier | Registry-controlled write policy; read-only DIDs reject writes. |
| `0x2F` | InputOutputControlByIdentifier | Compile-time disabled by default. |
| `0x31` | RoutineControl | Product callback; default reference routines are not implemented. |
| `0x34` | RequestDownload | Bounded download callback and protected-region checks. |
| `0x36` | TransferData | Block sequence and bounded programming callback. |
| `0x37` | RequestTransferExit | CRC/finish callback and activation-pending state. |
| `0x3E` | TesterPresent | Mainline session-activity keepalive. |
| `0x85` | ControlDTCSetting | Product callback with enable/disable state. |

Unsupported or disallowed requests return a bounded negative response where the buffer allows it. The public NRC reference documents common values and meanings such as `0x11` ServiceNotSupported, `0x13` IncorrectMessageLengthOrInvalidFormat, `0x22` ConditionsNotCorrect, `0x31` RequestOutOfRange, `0x33` SecurityAccessDenied, `0x35` InvalidKey, `0x36` ExceedNumberOfAttempts, `0x37` RequiredTimeDelayNotExpired, `0x70` UploadDownloadNotAccepted, `0x72` GeneralProgrammingFailure, `0x73` WrongBlockSequenceCounter, and `0x78` ResponsePending [3].

## CANopen coexistence

CANopenNode retains ownership of the CANopen receive path and FIFO0. UDS identifiers are placed in a separate FIFO1 exact-ID list, and the UDS adapter registers only the FIFO1 pending callback when HAL callback registration is enabled. CANopen and UDS therefore have separate identifier filters, queues, and mainline budgets. The application must still verify filter-bank allocation, interrupt latency, FIFO overflow behavior, and bus-load acceptance on the exact STM32F767 board and transceiver.

The coexistence test in `tests/integration/test_uds_canopen_coexistence.c` is a software contract only. It cannot prove electrical bus behavior, interrupt priority correctness, or production timing margins.

## References

[1]: https://www.iso.org/standard/72439.html "ISO 14229-1:2020 — Road vehicles — Unified diagnostic services (UDS) — Part 1: Application layer"

[2]: https://www.iso.org/standard/66574.html "ISO 15765-2:2016 — Road vehicles — Diagnostic communication over CAN — Part 2: Transport protocol and network layer services"

[3]: https://uds.readthedocs.io/en/v0.1.0/autoapi/uds/messages/nrc/index.html "UDS NRC public reference documentation"
