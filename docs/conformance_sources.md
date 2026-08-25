# Conformance source notes

## ISO 15765-2

Official source: https://www.iso.org/standard/66574.html

The ISO page identifies ISO 15765-2:2016 as the transport protocol and network-layer services specification for diagnostic communication over CAN. Its abstract states that Classical CAN frames carry 0–8 data bytes and CAN-FD frames carry 0–64 data bytes, and that the transport supports the standardized service primitive interface associated with UDS. The page also states that the 2016 edition is withdrawn and that ISO 15765-2:2024 is the current replacement. This repository must therefore describe its matrix as standards-oriented evidence, not a certification against a purchased edition.

## ISO 15765-2:2024

Official source: https://www.iso.org/standard/84211.html

The current ISO page identifies ISO 15765-2:2024 as a published fourth edition from April 2024. Its abstract describes a transport and network-layer protocol and services tailored to CAN-based vehicle network systems specified in ISO 11898-1, supporting the standardized abstract service primitive interface associated with UDS. The page notes that the standard does not decide whether CAN Classical, CAN FD, or both are required by standards that reference it. This repository therefore reports separate Classical CAN and CAN-FD implementation profiles rather than assuming that one target proves both.

## ISO 14229-1

Official source: https://www.iso.org/standard/72439.html

The ISO page identifies ISO 14229-1:2020 as the UDS application-layer specification. Its abstract states that it specifies data-link-independent requirements for diagnostic services that allow a tester/client to control diagnostic functions in an ECU/server, and that it does not specify implementation requirements. The page also states that the 2020 edition is withdrawn and that ISO 14229-1:2026 is the current replacement. The UDS conformance matrix should therefore separate service semantics from transport behavior and clearly identify the target edition used for review.

## Validation interpretation

The repository can execute deterministic protocol contracts and physical interoperability tests, but it cannot reproduce the copyrighted ISO text or claim formal conformance without a requirements matrix reviewed against the applicable purchased standard edition. Physical evidence must identify the ECU, tester/analyzer, MCU, transceiver, bit rates, frame format, addressing, traces, timing, and verdict for each case.

The source pages are metadata and scope references only; no copyrighted standard text has been copied into the repository.
