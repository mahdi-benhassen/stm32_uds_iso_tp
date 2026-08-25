# Standalone Architecture

The standalone library implements ISO 15765-2 ISO-TP and ISO 14229 UDS independently of CANopen. CANopen is neither required nor included. It accepts and emits abstract CAN frames through callbacks; it does not own a CAN peripheral, scheduler, GPIO, clock tree, interrupt vector, Flash controller, or bootloader.

| Layer | Path | Responsibility |
|---|---|---|
| Frame model | `library/include/uds_iso_tp/isotp.h` | CAN identifier, DLC, Classical/CAN-FD format, BRS flag, bounded data bytes, padding, and full-duplex configuration. |
| ISO-TP | `library/src/isotp.c` | SF, FF, CF, FC, independent RX/TX state, sequence, block size, STmin, timeout, overflow, Classical CAN and CAN-FD length formats, and serialization-boundary padding. |
| UDS services | `library/src/uds.c` | Centralized service/subservice metadata, address/session/security policy, sessions, NRCs, deferred reset state, and product callback policy. |
| Product policy | `uds_did.c`, `uds_download.c`, application-owned UDS callbacks | DIDs, authorization, SecurityAccess, memory boundaries, integrity, and lifecycle callbacks. The deterministic reference helper under `tests/security/` is test-only. |
| Endpoint | `library/src/endpoint.c` | Mainline composition of independent ISO-TP RX/TX contexts and UDS using an injected send function, optional functional CAN ID, bounded control/response queues, clock, and TX-completion boundary. |
| Hardware binding | `examples/` | Thin STM32 bxCAN or FDCAN callback contracts. |

> A transport library can be reused by a bootloader, a test fixture, or another application without making any of those systems a dependency.

The implementation uses static arrays sized by `ISOTP_MAX_PAYLOAD` and `UDS_MAX_*` configuration macros. A caller must run `uds_isotp_endpoint_receive()` from its received-frame handoff and call `uds_isotp_endpoint_process()` from a bounded mainline or task context. The frame-send callback must be non-blocking; a false return leaves the pending frame for a later call. With asynchronous transport completion, the caller configures `tx_complete` and calls `uds_isotp_endpoint_tx_complete()` after the final frame; this is also the deferred ECUReset execution boundary.

No UDS dispatch, Flash erase/program operation, reset operation, logging, or heap allocation belongs in an interrupt callback. The STM32F767 example is now a minimal UDS-only application and does not initialize or link another protocol stack.

## References

[1]: https://www.iso.org/standard/72439.html "ISO 14229-1 — UDS application layer"
[2]: https://www.iso.org/standard/66574.html "ISO 15765-2 — DoCAN transport protocol and network layer services"
