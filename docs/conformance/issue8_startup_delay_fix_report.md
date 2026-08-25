# GitHub Issue #8 — SecurityAccess startup-delay fix

## Summary

Issue [#8][1] reported that SecurityAccess could not begin until a 10-second timer expired after MCU power-on. The audit confirmed that the generic UDS server initialized SecurityAccess in a combined `DELAY` state, set `security_initial_delay_active`, and rejected the first valid RequestSeed with NRC `0x37`. The same design also conflated startup delay and failed-attempt lockout concepts in one state model.

The implementation now enters `LOCKED_READY` immediately at initialization and after both normal and programming logical resets. The 10-second timer is owned exclusively by `LOCKOUT`, which is entered only after three genuine invalid SendKey results. The generic UDS layer remains independent of application cryptography; seed generation and key verification continue to be injected through callbacks.

## Exact state-machine change

| State | Entry condition | RequestSeed | SendKey | Timer |
|---|---|---|---|---|
| `LOCKED_READY` | Power-on, reset, or lockout expiry | Accepted immediately when the active session permits SecurityAccess | NRC `0x24` if no valid seed exists | None |
| `WAITING_FOR_KEY` | A valid seed was issued | Replaces the pending seed and restarts only the seed-validity timer | Verifies the matching level and seed | Independent seed timeout |
| `UNLOCKED` | Correct key accepted | Existing policy applies | Existing unlocked policy applies | None |
| `LOCKOUT` | Third genuine invalid key | NRC `0x37` | NRC `0x37` | Exactly the configured lockout interval, default 10,000 ms |

At the exact lockout deadline, the server clears lockout, resets `failed_attempts` to zero, and enters `LOCKED_READY`. A subsequent RequestSeed is accepted immediately; no second startup or recovery delay is started.

## Attempt-counter semantics

The counter starts at zero. Only an application callback result of `UDS_RESULT_INVALID_KEY` increments it. RequestSeed, malformed messages, sequence errors, unsupported levels, and other callback denials do not count. The first and second invalid keys return NRC `0x35` and leave the server in `LOCKED_READY` after invalidating the corresponding seed. The third invalid key sets the counter to three, returns NRC `0x36`, invalidates the seed, enters `LOCKOUT`, and starts the 10-second lockout timer. A correct key resets the counter to zero and enters `UNLOCKED`. Lockout expiry also resets it to zero.

The seed-validity timer and lockout timer remain separate. Seed expiry invalidates only the pending seed and returns the server to `LOCKED_READY`; it does not start or extend the lockout timer.

## Compatibility and ownership

The public timing setter retains its former `security_initial_delay_ms` argument and the public `UdsServer` retains compatibility fields so existing source integrations do not fail unnecessarily. Those values are explicitly inert: initialization sets the compatibility active flag false, the setter never activates it, and the state machine never evaluates it. The legacy `UDS_SECURITY_STATE_LOCKED` and `UDS_SECURITY_STATE_DELAY` names remain aliases for `LOCKED_READY` and `LOCKOUT`, respectively. New code should use the explicit names.

The deterministic reference/test provider under `tests/security/` was updated to use the same four-state model and lockout-only timing. The application-owned security callbacks and the test-only key algorithm were not moved into the generic library and were not changed into production cryptography.

## Modified files

| File | Change |
|---|---|
| `library/include/uds_iso_tp/uds.h` | Defines explicit READY/WAITING/UNLOCKED/LOCKOUT states, inert compatibility fields, and the retained timing API. |
| `library/src/uds.c` | Removes startup-delay behavior, makes lockout the sole delay state, resets attempts on lockout expiry, and preserves existing NRC mapping. |
| `library/tests/uds/test_session_security.c` | Adds immediate power-on/session RequestSeed, immediate correct unlock, three wrong-key, exact 9.999/10.000-second boundary, reset, recovery, SendKey-during-lockout, and seed-expiry tests. |
| `tests/security/uds_security_reference.h` | Aligns the deterministic test provider’s public states and timer fields. |
| `tests/security/uds_security_reference.c` | Removes reference-provider startup delay and resets attempts at lockout expiry. |
| `docs/uds/security_access.md` | Documents the immediate-ready and lockout-only contract. |
| `docs/uds/security_reference.md` | Removes the manual instruction to wait 10 seconds before RequestSeed. |
| `docs/uds/session_control.md` | Updates reset/S3 timing descriptions. |
| `docs/conformance/issues7-8_implementation_report.md` | Refreshes the Issue #8 state diagram, NRC matrix, and readiness evidence. |

## Validation results

The final local validation passed after the refactor. The host test suite and sanitizer suite both completed all six CTest contracts. The STM32F767 cross-build remained successful, and the generic library continued to pass freestanding ARM GCC validation at the C092-compatible payload bound.

| Validation gate | Result |
|---|---|
| Normal CMake/Ninja build with examples | PASS |
| Normal CTest | PASS, 6/6 |
| ASan/UBSan CTest | PASS, 6/6 |
| Architecture check | PASS, 193 tracked paths |
| Validation-asset check | PASS, 13 conformance vectors and 20 physical cases |
| Strict Cortex-M0+ ARM GCC compile | PASS, `ISOTP_MAX_PAYLOAD=4095` |
| STM32F767 Release cross-build | PASS, 13,972 B Flash and 43,312 B RAM in the final run |
| Generic-library coverage | PASS, 67% total; `isotp.c` 88%, `uds.c` 79%, `endpoint.c` 89% |
| clang-format | PASS |
| clang-tidy | PASS |
| cppcheck | PASS; informational configuration-count note only |
| Physical SecurityAccess HIL | Not executed; no board, analyzer, or debug probe was available |

The acceptance behavior is therefore demonstrated in deterministic host tests and target cross-builds: after entering a permitted diagnostic session, **power-on/reset → immediate RequestSeed**, **three wrong keys → 10-second lockout**, and **lockout expiry → immediate RequestSeed**. No physical hardware claim is made.

## References

[1]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/8 "Issue #8 — Security Access 0x27"
