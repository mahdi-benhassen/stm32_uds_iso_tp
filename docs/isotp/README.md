# ISO-TP

The authoritative transport implementation is [`library/include/uds_iso_tp/isotp.h`](../../library/include/uds_iso_tp/isotp.h) and [`library/src/isotp.c`](../../library/src/isotp.c). It accepts a generic CAN/CAN-FD frame callback and an injected clock, uses bounded static storage, and has no dependency on a higher-level network protocol.

The supported profiles, Single-Frame and First-Frame length encoding, Flow Control state machine, timing rules, malformed-frame behavior, and explicit limitations are documented in [`docs/standalone/isotp.md`](../standalone/isotp.md). The transport contract tests are under [`library/tests/isotp/`](../../library/tests/isotp/).
