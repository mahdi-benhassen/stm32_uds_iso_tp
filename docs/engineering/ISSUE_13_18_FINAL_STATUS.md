# Issues #13–#18 Final Review Status

**Repository:** `mahdi-benhassen/stm32_uds_iso_tp`
**Revision reviewed:** `ad7bbb3`
**Target:** STM32C092 + FDCAN + external transceiver + Keil MDK / Arm Compiler 6

The latest review does **not** mark all issues fixed. The implementation now has stronger host-side contracts and a deterministic DTC fixture, but Keil compilation, flashing, real C092 traffic, and physical HIL evidence are unavailable in this environment.

## Status summary

| Issue | Exact status | Review conclusion |
|---:|---|---|
| #13 | **PROTOCOL ONLY** | C092 TX-event correlation, callback context, polling, error recovery, and endpoint reuse contracts are implemented. Real C092 target validation remains required. |
| #14 | **PROTOCOL ONLY** | The first-request lifecycle failure has a maintained transport fix and host regressions, but the reporter’s acceptance criterion requires 1,000 real C092 requests and mixed/error sequences without reinitialization. |
| #15 | **BACKEND REQUIRED** | AES-CMAC-128 and SecurityAccess state behavior are tested; application key ownership, freshness, provisioning, and complete product security policy remain external. |
| #16 | **PROTOCOL ONLY** | Service-family routing, common policy gate, bounded callback contracts, and memory/periodic preflight exist. The listed services are not all fully parsed or supplied with production reference backends. |
| #17 | **BACKEND REQUIRED** | A deterministic three-record reference fixture now exercises status filtering, snapshot/extended data, unknown records, clear/read interaction, and bounded responses. Device DTC storage and complete ISO 14229 revision-specific coverage remain backend-dependent. |
| #18 | **PROTOCOL ONLY** | All five reset types are recognized and tested, with deferred response completion. C092 explicitly supports hard and soft reset only; the remaining three return NRC `0x12` under the platform policy. |

No issue receives **IMPLEMENTED + HIL VERIFIED**, because no physical HIL session was executed.

## Validation evidence

The standalone suite contains ten contract targets. The normal and ASan/UBSan host builds pass all ten after the review changes. The repeated Classic CAN regression performs 1,000 consecutive TesterPresent transactions without endpoint or peripheral reinitialization. Additional tests cover direct interrupt/polling C092 event paths, stale markers, aborted and FIFO-full events, enqueue failure, endpoint TX-error recovery for single- and multi-frame responses, AES-CMAC RFC 4493 vectors, bounded arbitrary seed lengths, and deterministic DTC records.

The STM32F767 root firmware builds with ARM GCC 13.2.1. The strict freestanding ARM GCC check passes with `ISOTP_MAX_PAYLOAD=4095`, and the supplied Issue #14 project passes the C092 adapter syntax check against its HAL headers. GitHub Actions validates the host/ARM/static-analysis pipeline and generates HIL dry-run reports; those reports are procedure artifacts, not physical evidence.

The available environment does not include Keil MDK, Arm Compiler 6, `armlink`, `fromelf`, an STM32C092 board, a CAN analyzer, or a debugger. Therefore the following remain **not executed**: Keil build/link/map verification, C092 flash, physical CAN communication, TX Event FIFO measurements, reset-cause capture, and HIL stress sequences.

## Required next acceptance step

A hardware owner must select and document the exact C092 board, transceiver, bit rates, message-RAM/TX Event FIFO element count, Keil project, and analyzer. The live procedure should then execute power-on, 1,000 consecutive requests, mixed services, multi-frame traffic, malformed requests, timeout/error recovery, ECUReset response-before-reset ordering, restart, and trace capture. The software changes must not be called production-ready until that evidence is reviewed.

## References

[1]: https://uds.readthedocs.io/en/latest/pages/user_guide/message_translation.html#readdtcinformation "py-uds ReadDTCInformation service translation documentation"
