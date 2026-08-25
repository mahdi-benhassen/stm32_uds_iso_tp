# UDS

The UDS layer is an independent ISO 14229 service dispatcher above the ISO-TP transport. Its public callbacks cover application-owned DIDs, diagnostic trouble-code data, SecurityAccess, reset policy, routine policy, and download policy without requiring an object dictionary or another protocol stack.

The generic implementation is [`library/include/uds_iso_tp/uds.h`](../../library/include/uds_iso_tp/uds.h) and [`library/src/uds.c`](../../library/src/uds.c), with DID, download, and endpoint modules beside it. Security algorithms are application-owned callbacks; the deterministic reference/test implementation is intentionally outside the generic library at [`tests/security/`](../../tests/security/). Request and response lengths are deliberately bounded by the documented `uint16_t` callback API. See [`docs/standalone/release_audit.md`](../standalone/release_audit.md) for security and production boundaries.

The implemented policies are documented in [`session_control.md`](session_control.md) for Diagnostic Session Control (`0x10`) and [`security_access.md`](security_access.md) for Security Access (`0x27`). The repository-generated, non-production key calculator and vectors are specified in [`security_reference.md`](security_reference.md).
