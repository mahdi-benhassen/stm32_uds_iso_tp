# UDS Diagnostic Session Control (`0x10`)

## Scope and policy

The UDS core represents the active diagnostic session explicitly in `UdsServer.session`. The implementation supports the four session values requested in Issue #7: Default (`0x01`), Programming (`0x02`), Extended Diagnostic (`0x03`), and Safety System Diagnostic (`0x04`). Startup enters Default Session.

The issue’s detailed transition image contains nodes for Default, Extended Diagnostic, and Programming, while the overview image additionally lists Safety System Diagnostic. Because no pairwise Safety transition graph is shown, the implementation uses a conservative policy: Safety can return to Default, but undocumented transitions into or out of Safety are denied rather than inferred.

## Transition matrix

| Current session | Requested Default `0x01` | Requested Programming `0x02` | Requested Extended `0x03` | Requested Safety `0x04` |
|---|---:|---:|---:|---:|
| Default | Allowed | Denied | Allowed | Denied |
| Programming | Allowed | Allowed as same-session request | Denied | Denied |
| Extended | Allowed | Allowed | Allowed as same-session request | Denied |
| Safety | Allowed | Denied | Denied | Allowed as same-session request |

The policy is centralized in `uds_session_transition_allowed()`. The service handler never assigns the requested session without first checking this table. A denied transition produces a negative response with NRC `0x22` (`ConditionsNotCorrect`). An unsupported session value produces NRC `0x12` (`SubFunctionNotSupported`); an invalid message length produces NRC `0x13`.

The implementation intentionally follows the specific Issue #7 path constraints: Default cannot directly enter Programming; Extended is the gateway to Programming; Programming returns to Default and does not transition directly to Extended; and non-default sessions can return to Default. Same-session requests are accepted and reapply the session reset policy.

## Session request format and response

A request is exactly two bytes: `10` followed by the session subfunction. The high bit (`0x80`) is the suppress-positive-response indication and is removed before session-policy lookup. Values `0x81`, `0x82`, `0x83`, and `0x84` therefore select sessions `0x01`, `0x02`, `0x03`, and `0x04` with positive response suppression; they are not separate session identifiers.

A non-suppressed successful response is six bytes:

```text
50 <session> <P2 high> <P2 low> <P2* high in 10 ms> <P2* low in 10 ms>
```

The default configured values are `P2 = 50 ms` and `P2* = 5000 ms`. They are configuration values already held by `UdsServer`; no timing value is invented from the screenshots. A suppressed successful request returns `UDS_RESULT_NO_RESPONSE` and leaves the response length zero.

## Session-change side effects

Every accepted session request invalidates the active security level and any pending seed, stops an active download context, and resets its transfer block counter. A request for Default from a non-default session marks a normal reset as pending because the Issue #7 screenshot associates that path with complete reset behavior. A request for Programming marks a programming reset as pending; both reset reasons return security to the immediate `LOCKED_READY` state after the application applies the reset.

The transport-independent application must not reset from the request callback. For ECUReset (`0x11`), the callback only authorizes/prepares the request; the positive response is submitted first and the application-owned executor is called after the endpoint’s TX completion boundary. The detailed metadata and reset API are documented in [service_attributes.md](service_attributes.md).

For a reset that is already represented as pending by a session transition, the application may perform its platform reset at its chosen completion point and then call:

```c
uds_server_apply_reset(&server, UDS_RESET_NORMAL, now_ms);
```

for a normal reset, or:

```c
uds_server_apply_reset(&server, UDS_RESET_PROGRAMMING, now_ms);
```

for the programming/reprogramming reset. The API returns the logical state to Default, clears pending reset state, invalidates seeds and security level, resets download state, and establishes `LOCKED_READY` with no startup SecurityAccess timer. The generic library does not call an MCU reset primitive.

## S3 server timer

`uds_server_tick()` is non-blocking and uses the injected monotonic `uint32_t` time value. It compares elapsed time with wrap-safe unsigned subtraction:

```c
(uint32_t)(now_ms - last_activity_ms) >= s3_server_timeout_ms
```

The default S3 timeout is `UDS_DEFAULT_S3_SERVER_MS` (`5000 ms`) and can be changed with `uds_server_set_timing()`. Valid diagnostic requests refresh `last_activity_ms`, including Tester Present and requests that produce a negative response. At expiration the server enters Default Session, invalidates security and seed state, stops an active download, resets the download block counter, refreshes the timer origin, and returns `UDS_RESULT_BUSY` to identify the expiration event. The endpoint remains non-blocking and does not require a sleep or `HAL_Delay()`.

S3 expiration is separate from the lockout timer. S3 expiration locks security and invalidates the seed, but does not start a SecurityAccess delay; a subsequent permitted session request can reach immediate `LOCKED_READY` behavior.

## Public API

The session-related API is:

```c
UdsSessionTransitionResult uds_session_transition_allowed(
    uint8_t current_session, uint8_t requested_session);
UdsCallbackResult uds_server_request_session(
    UdsServer *server, uint8_t requested_session, uint32_t now_ms);
void uds_server_apply_reset(
    UdsServer *server, UdsResetReason reason, uint32_t now_ms);
UdsCallbackResult uds_server_tick(UdsServer *server, uint32_t now_ms);
uint8_t uds_server_session(const UdsServer *server);
```

The library has no STM32 HAL, CMSIS, sleep, delay, heap, or reset-register dependency. The application owns the actual reset mechanism. For ECUReset responses, configure the endpoint’s optional `tx_complete` callback when transport completion is asynchronous and call `uds_isotp_endpoint_tx_complete()` after the final frame; the endpoint then invokes `ecu_reset_execute`. After the platform reset returns or on the next boot, the application can use `uds_server_apply_reset()` to reinitialize logical state.

## Test coverage

`uds_iso_tp_session_security_contract` covers startup Default, all documented allowed and forbidden transitions, repeated same-session behavior, Programming reset-pending behavior, normal/programming reset effects, suppress-positive-response, invalid session values, invalid lengths, S3 timeout at the exact boundary, timer refresh, and session/security invalidation. The tests use a deterministic fake timestamp and no real delays.

## References

[1]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/7 "Issue #7 — Session Handling 0x10"
[2]: https://www.iso.org/standard/72439.html "ISO 14229-1:2020 metadata"
