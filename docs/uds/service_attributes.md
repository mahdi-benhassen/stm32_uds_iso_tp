#UDS service attributes and addressing policy

The generic UDS dispatcher exposes `UdsServiceAttribute` metadata for service and subservice policy. The structure is:

```c
typedef struct {
    uint8_t sid;
    uint8_t subservice;
    uint8_t session_mask;
    uint16_t security_mask;
    uint8_t address_mode;
} UdsServiceAttribute;
```

`session_mask` is a bitmask using `UDS_SESSION_MASK_DEFAULT`, `UDS_SESSION_MASK_PROGRAMMING`, `UDS_SESSION_MASK_EXTENDED`,
    and `UDS_SESSION_MASK_SAFETY`. `security_mask` is an allowed - level bitmask,
    not a minimum - level value; for
    example, `UDS_SECURITY_MASK_LEVEL_1` allows only the unlocked Level 1 state. `UDS_SECURITY_MASK_NONE` means that no unlocked security level is required. `subservice=UDS_SERVICE_ANY_SUBFUNCTION` represents a service-wide default, while the metadata type also permits exact subservice entries.

`address_mode` uses `UDS_ADDRESS_PHYSICAL` and `UDS_ADDRESS_FUNCTIONAL` bits. As requested by Issue #11, `UDS_ADDRESS_MODE_BOTH` is `0`; zero means both physical and functional addressing. The endpoint determines the addressing mode from the received CAN ID before dispatch: `request_id` is physical and the optional non-zero `functional_request_id` is functional. Functional requests are normalized to the endpoint’s network-layer request ID for ISO-TP processing, while the UDS policy receives the original addressing mode.

The current repository-generated policy is intentionally explicit:

| Service | Sessions | Security | Addressing |
|---|---|---|---|
| `0x10`, `0x19`, `0x22`, `0x28`, `0x3E` | All four sessions | None | Both (`0`) |
| `0x11` | All four sessions | None | Physical |
| `0x27` | Extended and Programming | None for RequestSeed; key state remains in the SecurityAccess state machine | Physical |
| `0x2F` | Extended | None | Physical |
| `0x31` | Extended and Programming | None in this generic profile | Physical |
| `0x34`, `0x36`, `0x37` | Programming | None in this generic profile | Physical |
| `0x85` | All four sessions | None | Physical |

These entries are a repository policy baseline, not a claim that every ECU uses the same matrix. Applications requiring a different policy should replace or extend the metadata implementation rather than duplicating checks inside every service handler. Unsupported address/session/security combinations return the existing negative-response path with NRC `0x7F` for service-not-supported-in-active-session or NRC `0x33` for security denial.

## SecurityAccess mapping

The authoritative function `uds_security_subfunction_level()` maps odd RequestSeed and following even SendKey subfunctions explicitly:

| Level | RequestSeed | SendKey |
|---:|---:|---:|
| 1 | `0x01` | `0x02` |
| 2 | `0x03` | `0x04` |
| 3 | `0x05` | `0x06` |
| 4 | `0x07` | `0x08` |
| 5 | `0x09` | `0x0A` |

The generic server still requires application callbacks for seed generation and key verification. The repository’s deterministic helper remains test-only under `tests/security/`.

## ECUReset and completion

For `0x11 0x01`, the existing `ecu_reset` callback authorizes or prepares the reset. The server then creates the positive `0x51 0x01` response and sets `reset_pending`; it does not invoke a hardware reset. The endpoint submits the response through ISO-TP first. If the transport uses asynchronous completion, configure `tx_complete` and call `uds_isotp_endpoint_tx_complete()` after the final CAN frame is complete. The endpoint then calls the application-owned `ecu_reset_execute` callback. If `tx_complete` is absent, a successful `send_frame` return is the documented handoff/completion boundary. No fixed delay is used in the generic library.

The STM32F767 example implements the executor in `App/Src/uds_platform.c`, where `NVIC_SystemReset()` is target-specific and outside `library/`. The same pattern can be used for another platform with its own reset primitive.

## References

[1]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/11 "Issue #11 — UDS service attributes"
[2]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/12 "Issue #12 — ECUReset response order"
[3]: https://www.iso.org/standard/72439.html "ISO 14229-1 metadata"
