# UDS Security

The checked-in SecurityAccess provider is a deterministic **NON-PRODUCTION TEST provider**. It exists to exercise seed generation, key verification, invalid-attempt counting, lockout timing, session reset, and constant-time comparison in automated tests. It must not be used as a deployed ECU security mechanism.

A production provider must replace it without changing the UDS core and must define entropy, secret storage, key derivation or verification, security levels, attempt persistence, lockout policy, audit events, service authorization, debug-port lifecycle, secure boot, signed updates, and vulnerability response. Secrets must not be committed to this repository or embedded as public reference constants.

The default UDS runtime also keeps CommunicationControl, RoutineControl, I/O control, Flash programming, and DID writes policy-gated. Enabling a compile-time service switch is not equivalent to authorizing a production action. Product security review and HIL evidence are mandatory before enabling destructive operations.

Common SecurityAccess NRCs are `0x33` SecurityAccessDenied, `0x35` InvalidKey, `0x36` ExceedNumberOfAttempts, and `0x37` RequiredTimeDelayNotExpired` [1].

## References

[1]: https://uds.readthedocs.io/en/v0.1.0/autoapi/uds/messages/nrc/index.html "UDS NRC public reference documentation"
