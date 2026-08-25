# UDS Security Access (`0x27`)

## Security boundary

The UDS core now contains an explicit security state machine, but the configured callback remains application-owned. The repository includes a deterministic reference/test callback implementation outside the generic library; it is not a production cryptographic implementation. Production firmware must inject an approved seed-generation and key-verification policy without placing cryptography or hardware dependencies in `library/`. See [SecurityAccess Reference/Test Infrastructure](security_reference.md) for the exact repository-generated vectors and manual procedure.

The states are:

| State | Meaning |
|---|---|
| `UDS_SECURITY_STATE_LOCKED_READY` | No valid seed is pending and a new RequestSeed is immediately permitted when session policy allows it |
| `UDS_SECURITY_STATE_WAITING_FOR_KEY` | A seed has been issued, is bound to one security level, and is waiting for a matching SendKey |
| `UDS_SECURITY_STATE_UNLOCKED` | The key was accepted for the selected level; the seed is invalidated |
| `UDS_SECURITY_STATE_LOCKOUT` | Three genuine invalid keys have been received and the lockout timer is active |

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

A RequestSeed is exactly two bytes: `27 <odd-subfunction>`. The server validates the active diagnostic session, lockout state, subfunction mapping, and message length before invoking the application seed callback. A successful response is:

```text
67 <request-seed-subfunction> <seed bytes...>
```

The returned seed is marked valid, associated with its security level, and given the configurable seed lifetime. A repeated RequestSeed replaces the previous pending seed and restarts its seed lifetime whenever the session is permitted and the server is not in LOCKOUT.

A SendKey request must contain at least one key byte and must follow a valid RequestSeed for the same security level. SendKey without a seed, with a seed for another level, or after seed expiry returns NRC `0x24` (`RequestSequenceError`). A successful response is:

```text
67 <send-key-subfunction>
```

The seed is invalidated immediately after successful unlock and cannot be reused. A stale key therefore cannot unlock the server a second time.

## Initialization and reset

Power-on initialization and both normal and programming ECU resets enter `UDS_SECURITY_STATE_LOCKED_READY`. There is no startup SecurityAccess timer. After the application enters Extended or Programming Session, the first valid RequestSeed is accepted immediately. The legacy `security_initial_delay_ms` argument and compatibility fields remain source-compatible but are inert and are never evaluated. The actual MCU reset remains application-owned.

## Failed keys and lockout

Only an application callback result of `UDS_RESULT_INVALID_KEY` counts as a failed attempt. Other denials, unsupported levels, malformed messages, and sequence errors do not increment the counter. An invalid key returns NRC `0x35` and increments `security_failed_attempts`.

When the counter reaches the configurable maximum, default `3`, the server returns NRC `0x36`, invalidates the seed, enters `UDS_SECURITY_STATE_LOCKOUT`, and starts the lockout timer, default `10000 ms`. RequestSeed and SendKey during the lockout return NRC `0x37` and do not invoke the application security callbacks.

At the exact lockout-expiration boundary, `uds_server_tick()` clears the lockout, resets `failed_attempts` to zero, and enters `UDS_SECURITY_STATE_LOCKED_READY`. A new RequestSeed is therefore immediately possible without another delay. A successful unlock also resets the failed-attempt counter to zero. Only a genuine invalid SendKey increments the counter; RequestSeed, malformed messages, sequence errors, and other callback denials do not count.

## Session and reset interaction

Security Access is permitted only in Extended (`0x03`) and Programming (`0x02`) sessions. Default and Safety sessions return NRC `0x7F` (`ServiceNotSupportedInActiveSession`) for Security Access requests. The service metadata and address/session/security bitmask conventions are described in [service_attributes.md](service_attributes.md); the SecurityAccess state machine remains centralized in the UDS core.

Every accepted session change invalidates the current seed and security level and returns security to LOCKED_READY, while an active failed-attempt lockout remains active. A session transition does not silently preserve an unlocked state. A normal or programming ECU reset clears the failed-attempt counter, invalidates the seed, and returns to Default Session with no startup delay. S3 expiration returns to Default and locks security without manufacturing a new timer.

## Timing and wrap-around

All security timers use monotonic `uint32_t` timestamps and wrap-safe comparisons of the form:

```c
(int32_t)(now_ms - deadline_ms) >= 0
```

The seed timeout and lockout timeout are independently configurable through `uds_server_set_timing()` and default to `10000 ms`; its retained `security_initial_delay_ms` parameter is ignored for compatibility. The seed timer invalidates a pending seed at the exact expiry boundary. The lockout timer is tested at `timeout - 1 ms`, `timeout`, and `timeout + 1 ms` using a deterministic clock; initialization never starts either timer.

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

The deterministic reference/test algorithm is deliberately isolated under `tests/security/uds_security_reference.c` and `.h`. It uses a four-byte opaque seed and the repository-generated bytewise rule `key[i] = seed[i] XOR {A5 5A C3 3C}[i]` for both supported levels. The level-specific wire pairs remain Level 1 (`0x01/0x02`) and Level 5 (`0x09/0x0A`); no reporter-specific formula is implied. The generic server invokes only the application callback and continues to own sessions, seed lifetime, attempt counting, lockout, and NRC mapping. The reference provider follows the same immediate-ready/lockout-only timing contract.

This implementation is **TEST/REFERENCE ONLY** and must not be used for production ECU security. The manual calculator is built as `uds_iso_tp_security_test_key` from `tools/security_test_key.c`; for example, `./build/uds_iso_tp_security_test_key 1 00112233` returns `A5 4B E1 0F`. The full algorithm, vectors, and replacement guidance are in [security_reference.md](security_reference.md).

## Test coverage

`uds_iso_tp_session_security_contract` covers immediate RequestSeed after power-on/session entry, immediate correct unlock, invalid keys one through three, NRC `0x24`, NRC `0x35`, NRC `0x36`, NRC `0x37`, lockout at 9.999/10.000 seconds, immediate retry after expiry, failed-counter reset, seed invalidation and expiry, stale-key rejection, both documented security levels, malformed requests, callback denials that do not count, session denial in Default, session-transition invalidation, normal reset, and programming reset. The deterministic reference provider has matching immediate-ready and three-attempt lockout coverage.

No test claims production cryptographic strength. The callback boundary is deliberately preserved for an application-approved security provider.

## References

[1]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/8 "Issue #8 — Security Access 0x27"
[2]: https://www.iso.org/standard/72439.html "ISO 14229-1:2020 metadata"
