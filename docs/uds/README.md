# UDS

The UDS layer is an independent ISO 14229 service dispatcher above the ISO-TP transport. Its public callbacks cover application-owned DIDs, diagnostic trouble-code data, SecurityAccess, reset policy, routine policy, and download policy without requiring an object dictionary or another protocol stack.

The implementation is [`library/include/uds_iso_tp/uds.h`](../../library/include/uds_iso_tp/uds.h) and [`library/src/uds.c`](../../library/src/uds.c), with DID, security-provider, download, and endpoint modules beside it. Request and response lengths are deliberately bounded by the documented `uint16_t` callback API. See [`docs/standalone/release_audit.md`](../standalone/release_audit.md) for security and production boundaries.
