# UDS SecurityAccess and DID Policy

## Policy-first design

The reference server treats diagnostic data exposure and mutation as product policy. The table-driven DID registry in `library/compat/legacy_diagnostics/uds/uds_did.*` associates each identifier with read/write permissions, minimum session, minimum security level, maximum value length, and a source binding. The registry does not discover memory by address and does not expose arbitrary Object Dictionary or RAM locations.

A product integration must replace the reference values with authoritative, lifecycle-owned data. The default runtime exposes bounded reference identity and status views so that the transport and service paths can be tested without claiming a production identity, cryptographic policy, or bootloader implementation.

## Session and permission matrix

| Operation | Default session | Extended session | Programming session | Security level required |
|---|---:|---:|---:|---:|
| Read approved identity/status DID | Per registry entry | Per registry entry | Per registry entry | Per registry entry. |
| Write DID | Read-only by default | Explicit registry permission | Explicit registry permission | Explicit registry permission. |
| ECU reset | Callback and policy controlled | Callback and policy controlled | Callback and policy controlled | The reference runtime requires a non-zero security level. |
| RequestDownload/TransferData | Denied unless a valid memory callback is installed | Product policy | Product policy | Product policy; the reference download callbacks are intentionally non-production. |
| CommunicationControl | Unsupported by default | Unsupported by default | Unsupported by default | Must be explicitly implemented and authorized. |
| RoutineControl | Unsupported by default | Unsupported by default | Unsupported by default | Must be explicitly implemented and authorized. |

The server resets the security level when a new diagnostic session is selected. The S3 timeout returns the session to default behavior through `uds_server_tick()`. Session changes, SecurityAccess state, and reset/download transitions must be included in product threat modeling and audit logs where required.

## Replaceable SecurityAccess provider

`uds_security_provider.*` defines a replaceable interface for seed generation and key verification. The checked-in provider is explicitly a **deterministic non-production test provider**. It supports a bounded number of attempts, a lockout delay, session reset, generated seeds, and constant-time byte comparison. It is present to make the UDS state machine testable, not to provide a deployable vehicle security scheme.

A production provider must define the level map, seed entropy source, key derivation or verification algorithm, secret storage, attempt persistence, lockout behavior, reset policy, diagnostics logging, and failure recovery. Secrets must not be compiled into the public reference firmware. The provider must also be reviewed against the applicable product cybersecurity process and secure-update architecture.

## SecurityAccess response behavior

The implementation distinguishes the following externally visible conditions:

| Condition | NRC |
|---|---:|
| Unsupported or malformed subfunction | `0x12` |
| Key does not match | `0x35` |
| Attempt limit exceeded | `0x36` |
| Lockout timer active | `0x37` |
| Missing or unsatisfied security policy | `0x33` or `0x22`, depending on the callback result. |

The server does not expose key material in diagnostic responses. A seed response must fit the bounded UDS response buffer. Repeated invalid keys must not create unbounded work, heap allocation, or persistent side effects outside the provider contract.

## DID registry rules

Every supported DID should have a documented owner and evidence source. A registry entry should answer the following questions before it is enabled in a product profile:

| Question | Required evidence |
|---|---|
| What is the source of the value? | A project-owned runtime view, not a guessed address. |
| Is it readable in every session? | Explicit minimum-session policy and negative tests. |
| Is it writable? | Explicit write callback and bounds/format tests; read-only by default. |
| Is security required? | Explicit minimum security level and SecurityAccess test. |
| What is the maximum encoded length? | Registry capacity and response-size test. |
| Can the value change concurrently? | Ownership, snapshot, or critical-section contract. |
| What happens when the source is unavailable? | Deterministic NRC mapping and diagnostic evidence. |

Unknown DIDs, oversized values, disallowed sessions, insufficient security, and writes to read-only entries are rejected deterministically. The registry tests in `tests/uds/test_uds_did.c` cover these cases.

## References

[1]: https://www.iso.org/standard/72439.html "ISO 14229-1:2020 — Road vehicles — Unified diagnostic services (UDS) — Part 1: Application layer"

[2]: https://uds.readthedocs.io/en/v0.1.0/autoapi/uds/messages/nrc/index.html "UDS NRC public reference documentation"
