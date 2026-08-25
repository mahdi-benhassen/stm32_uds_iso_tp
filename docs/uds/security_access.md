# UDS Security Access (`0x27`)

## Security boundary

The UDS core now contains an explicit security state machine, but the configured callback remains application-owned. The repository includes a deterministic reference/test callback implementation outside the generic library; it is not a production cryptographic implementation. Production firmware must inject an approved seed-generation and key-verification policy without placing cryptography or hardware dependencies in `library/`. See [SecurityAccess Reference/Test Infrastructure](security_reference.md) for the exact repository-generated vectors and manual procedure.

The states are:

| State | Meaning |
|---|---|
| `UDS_SECURITY_STATE_LOCKED` | No valid seed is pending and a new RequestSeed is permitted when timing/session policy allows it |
| `UDS_SECURITY_STATE_WAITING_FOR_KEY` | A seed has been issued, is bound to one security level, and is waiting for a matching SendKey |
| `UDS_SECURITY_STATE_UNLOCKED` | The key was accepted for the selected level; the seed is invalidated |
| `UDS_SECURITY_STATE_DELAY` | The initial post-reset delay or the failed-attempt lockout is active |

## Security-level mapping

The issue images show `27h 02h` for the first unlocked level and `27h 0Ah` for a second unlocked level. The authoritative `uds_security_subfunction_level()` function uses the explicit odd/even mapping below:

| Security level | Request Seed | Send Key | Policy |
|---|---:|---:|---|
| Level 1 | `0x01` | `0x02` | Supported in Extended and Programming sessions |
| Level 2 | `0x03` | `0x04` | Callback-defined; wire mapping is supported |
| Level 3 | `0x05` | `0x06` | Callback-defined; wire mapping is supported |
| Level 4 | `0x07` | `0x08` | Callback-defined; wire mapping is supported |
| Level 5 | `0x09` | `0x0A` | Supported in Extended and Programming sessions |

The screenshot labels the `0x0A` path “Unlocked (Level 2)”, while the UDS subfunction pairing convention identifies `0x09/0x0A` as a level-5 pair. This is a documented interpretation of an image-label mismatch; the wire subfunctions are preserved exactly and the callback receives the mapped level value. The current repository reference provider implements only Levels 1 and 5; Levels 2–4 remain application-callback responsibilities.

Other subfunctions are rejected with NRC `0x12` (`SubFunctionNotSupported`). The implementation does not silently treat every odd/even value below `0x80` as supported.

## RequestSeed and SendKey

A RequestSeed is exactly two bytes: `27 <odd-subfunction>`. The server validates the active diagnostic session, delay state, subfunction mapping, and message length before invoking the application seed callback. A successful response is:

```text
67 <request-seed-subfunction> <seed bytes...>
```

The returned seed is marked valid, associated with its security level, and given the configurable seed lifetime. A repeated RequestSeed replaces the previous pending seed and restarts its seed lifetime after the delay/session policy permits it.

A SendKey request must contain at least one key byte and must follow a valid RequestSeed for the same security level. SendKey without a seed, with a seed for another level, or after seed expiry returns NRC `0x24` (`RequestSequenceError`). A successful response is:

```text
67 <send-key-subfunction>
```

The seed is invalidated immediately after successful unlock and cannot be reused. A stale key therefore cannot unlock the server a second time.

## Initial delay

Startup and a normal ECU reset enter the Locked state with a configurable initial delay. The default is `UDS_DEFAULT_SECURITY_INITIAL_DELAY_MS` (`10000 ms`). During the delay, a valid RequestSeed request returns NRC `0x37` (`RequiredTimeDelayNotExpired`). The delay is evaluated by `uds_server_tick()` and never uses blocking waits or real-time sleeps.

A Programming reset is distinct. When the application applies `UDS_RESET_PROGRAMMING`, the server returns to Default Session and Locked security without starting the normal 10-second initial delay. This implements the issue’s reprogramming-time requirement; the actual MCU reset remains application-owned.

## Failed keys and lockout

Only an application callback result of `UDS_RESULT_INVALID_KEY` counts as a failed attempt. Other denials, unsupported levels, malformed messages, and sequence errors do not increment the counter. An invalid key returns NRC `0x35` and increments `security_failed_attempts`.

When the counter reaches the configurable maximum, default `3`, the server returns NRC `0x36`, invalidates the seed, enters `UDS_SECURITY_STATE_DELAY`, and starts the configurable lockout, default `10000 ms`. RequestSeed during the lockout returns NRC `0x37` and does not invoke the seed callback.

At the exact lockout-expiration boundary, `uds_server_tick()` clears the delay and decrements the failed-attempt counter by one. It does not reset the counter to zero. Thus three failed attempts become two after the first lockout, matching the Issue #8 diagram. A subsequent invalid key can bring the count back to three and start another lockout. A successful unlock resets the failed-attempt counter to zero.

## Session and reset interaction

Security Access is permitted only in Extended (`0x03`) and Programming (`0x02`) sessions. Default and Safety sessions return NRC `0x7F` (`ServiceNotSupportedInActiveSession`) for Security Access requests. The service metadata and address/session/security bitmask conventions are described in [service_attributes.md](service_attributes.md); the SecurityAccess state machine remains centralized in the UDS core.

Every accepted session change invalidates the current seed and security level and returns security to Locked, while an active failed-attempt lockout remains active. A session transition does not silently preserve an unlocked state. A normal ECU reset clears the failed-attempt counter, invalidates the seed, returns to Default Session, and starts the 10-second initial delay. A programming reset clears the same security state but skips that initial delay. S3 expiration returns to Default and locks security without manufacturing a new power-on delay.

## Timing and wrap-around

All security timers use monotonic `uint32_t` timestamps and wrap-safe comparisons of the form:

```c
(int32_t)(now_ms - deadline_ms) >= 0
```

The seed timeout is configurable through `uds_server_set_timing()` and defaults to `10000 ms`. It invalidates a pending seed at the exact expiry boundary. The initial delay and lockout are also tested at `timeout - 1 ms`, `timeout`, and `timeout + 1 ms` using a deterministic clock.

## Public inspection/configuration API

The generic service-policy inspection functions are:

```c
bool uds_security_subfunction_level(uint8_t subfunction, uint8_t *level, bool *is_seed);
const UdsServiceAttribute *uds_service_attribute(uint8_t sid, uint8_t subservice);
bool uds_service_attribute_allows(const UdsServiceAttribute *attribute, uint8_t session,
                                  uint8_t security_level, UdsAddressMode address_mode);
```


```c
void uds_server_apply_reset(UdsServer *server, UdsResetReason reason, uint32_t now_ms);
void uds_server_set_timing(UdsServer *server, uint32_t s3_timeout_ms,
                           uint32_t security_initial_delay_ms, uint32_t security_lockout_ms,
                           uint32_t security_seed_timeout_ms, uint8_t security_max_attempts);
UdsSecurityState uds_server_security_state(const UdsServer *server);
uint8_t uds_server_security_level(const UdsServer *server);
uint8_t uds_server_security_failed_attempts(const UdsServer *server);
bool uds_server_security_seed_valid(const UdsServer *server);
```

## Reference/test implementation

The deterministic reference/test algorithm is deliberately isolated under `tests/security/uds_security_reference.c` and `.h`. It uses a four-byte opaque seed and the repository-generated bytewise rule `key[i] = seed[i] XOR {A5 5A C3 3C}[i]` for both supported levels. The level-specific wire pairs remain Level 1 (`0x01/0x02`) and Level 5 (`0x09/0x0A`); no reporter-specific formula is implied. The generic server invokes only the application callback and continues to own sessions, seed lifetime, initial delay, attempt counting, lockout, and NRC mapping.

This implementation is **TEST/REFERENCE ONLY** and must not be used for production ECU security. The manual calculator is built as `uds_iso_tp_security_test_key` from `tools/security_test_key.c`; for example, `./build/uds_iso_tp_security_test_key 1 00112233` returns `A5 4B E1 0F`. The full algorithm, vectors, and replacement guidance are in [security_reference.md](security_reference.md).

## Test coverage

`uds_iso_tp_session_security_contract` covers the initial delay, RequestSeed at the delay boundary, SendKey without seed, invalid keys one through three, NRC `0x35`, NRC `0x36`, NRC `0x37`, lockout and expiration, decrement-after-timeout, successful unlock, failed-counter reset, seed invalidation and expiry, stale-key rejection, both documented security levels, malformed requests, callback denials that do not count, session denial in Default, session-transition invalidation, normal reset, and programming reset.

No test claims production cryptographic strength. The callback boundary is deliberately preserved for an application-approved security provider.

## References

[1]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/8 "Issue #8 — Security Access 0x27"
[2]: https://www.iso.org/standard/72439.html "ISO 14229-1:2020 metadata"
