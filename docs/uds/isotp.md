# ISO-TP

The reference transport implements a bounded classic-CAN ISO-TP subset. Classic CAN frames carry at most eight data bytes; the implementation limits a logical payload to `ISOTP_MAX_PAYLOAD` (4095 bytes). The transport is independent from UDS and can be tested with any bounded payload.

| Frame | Purpose | Validation |
|---|---|---|
| SF | Carries a short payload in one frame. | Length nibble must fit the DLC and be non-zero. |
| FF | Starts a multi-frame payload. | Twelve-bit length must be greater than seven and within the configured maximum. |
| CF | Carries seven payload bytes and a four-bit sequence number. | Active transfer and expected sequence are required. |
| FC | Controls a sender. | Flow status, block size, and STmin must be valid. |

The receiver emits `FC CTS` with configured block size and STmin after a valid First Frame. The sender honors remote block size and separation time. Reserved STmin encodings are rejected; microsecond encodings are conservatively rounded to at least one millisecond because the reference scheduler uses a millisecond clock. Timeouts use wrap-safe unsigned deadline comparisons.

The embedded adapter uses independent RX and TX rings. RX copies are performed by the ISR callback and UDS processing occurs in mainline. A full queue increments an overflow counter and drops the frame deterministically. TX mailbox exhaustion is non-blocking and increments transport statistics for later diagnostics.

The implementation covers standard 11-bit identifiers only in the default STM32 adapter. Extended addressing, CAN FD payloads, mixed addressing, padding policy, wait-frame retransmission policy, and network-layer confirmation are outside the reference claim and require product-specific design.

## References

[1]: https://www.iso.org/standard/66574.html "ISO 15765-2:2016 — Road vehicles — Diagnostic communication over CAN — Part 2: Transport protocol and network layer services"
