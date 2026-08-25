# UDS Architecture

This repository implements a bounded UDS reference subset over classic CAN; it is enabled by default in the current reference build and can be disabled at compile time. The design separates the CAN controller adapter, ISO-TP transport, UDS service dispatcher, and product policy. The application layer is informed by ISO 14229-1 [1]; the classic-CAN transport subset is informed by ISO 15765-2 [2]. The implementation is not a claim of complete or certified conformance to those standards.

## Runtime layers

| Layer | Location | Contract |
|---|---|---|
| CAN adapter | `middleware/diagnostics/uds_stm32/` | ISR-safe copy into fixed RX storage and bounded non-blocking TX submission. |
| ISO-TP | `middleware/diagnostics/isotp/` | SF/FF/CF/FC, BS, STmin, sequence, timeout, and overflow handling. |
| UDS core | `middleware/diagnostics/uds/uds.c` | Sessions, service dispatch, NRCs, response bounds, and callback ownership. |
| Policy | DID, SecurityAccess, download, and application wrapper modules | Product data, authorization, memory regions, and lifecycle callbacks. |

CANopenNode remains the CANopen owner. UDS is initialized when `CANOPEN_REFERENCE_ENABLE_UDS=1`; the current CMake and configuration-header defaults set this to enabled, while products may explicitly disable it. The normal CANopen process and watchdog order remain unchanged; UDS runs in an explicit mainline budget after CANopen processing.

## Safety boundary

No UDS service is executed from a CAN RX interrupt. No UDS callback erases or programs Flash from an interrupt. No UDS path allocates heap memory or calls `printf`. All queues, payload buffers, and response buffers have compile-time capacities. A production product must replace the reference security and download callbacks and provide target evidence.

## References

[1]: https://www.iso.org/standard/72439.html "ISO 14229-1:2020 — Road vehicles — Unified diagnostic services (UDS) — Part 1: Application layer"

[2]: https://www.iso.org/standard/66574.html "ISO 15765-2:2016 — Road vehicles — Diagnostic communication over CAN — Part 2: Transport protocol and network layer services"
