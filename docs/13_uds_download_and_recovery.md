# UDS Download and Recovery Architecture

## Design boundary

The repository contains a bounded download state machine, not a complete production bootloader. `library/compat/legacy_diagnostics/uds/uds_download.*` validates address ranges, alignment, transfer length, block sequence, CRC32, timeout, and activation state. Actual Flash erase, program, verification, image selection, rollback, and reset behavior are supplied through callbacks owned by the product or bootloader.

The UDS application-layer service path follows the general diagnostic-service model of ISO 14229-1 [1], while the multi-frame transport is bounded to classic CAN ISO-TP behavior [2]. Neither the UDS layer nor the ISO-TP layer is permitted to write Flash directly.

## Memory policy

The reference map separates protected regions from a staging image area. The exact addresses are configuration inputs for the selected STM32F767 linker script and board memory map; they are not universal STM32F767 production values.

| Region | Reference policy | Download access |
|---|---|---|
| Bootloader | Protected | Never writable by the application diagnostic server. |
| Active application | Protected while executing | Not directly writable by the reference server. |
| Staging image | Candidate image area | Writable only after session, security, authorization, and callback checks pass. |
| CANopen NVM | Protected from firmware image writes | Preserved for configuration and persistence. |
| Diagnostic storage | Product-defined | Must have separate wear, power-loss, and access policy. |

A production bootloader must add image authenticity and integrity verification, version and anti-rollback policy, key and certificate management, recovery mode, power-loss safety, watchdog behavior, and a formally reviewed activation protocol. CRC32 is an integrity check for the bounded reference state machine; it is not a cryptographic authenticity mechanism.

## State machine

The expected sequence is:

1. Enter the authorized programming session.
2. Complete SecurityAccess using the product provider.
3. Issue RequestDownload for an allowed staging address and length.
4. Erase the permitted staging range through the asynchronous product callback.
5. Send TransferData blocks with the expected sequence counter.
6. Service the watchdog only through the approved callback while erase/program work makes progress.
7. Issue RequestTransferExit with the configured integrity value.
8. Verify the image and metadata through the product callback.
9. Mark activation pending only after all checks pass.
10. Let the bootloader activate the image at a safe reset boundary.

The reference state machine rejects unaligned addresses and lengths, protected regions, transfers before RequestDownload, wrong block sequence counters, writes beyond the declared range, missing callbacks, programming failures, and timeouts. A failed or timed-out transfer must leave the active application intact and clear the pending activation state.

## NRC mapping

| Failure | Expected diagnostic meaning |
|---|---|
| Disallowed memory target or policy | UploadDownloadNotAccepted (`0x70`) or RequestOutOfRange (`0x31`) according to the service-layer mapping. |
| Wrong transfer sequence | WrongBlockSequenceCounter (`0x73`). |
| Erase/program failure | GeneralProgrammingFailure (`0x72`). |
| Transfer timeout or suspension | TransferDataSuspended (`0x71`) or product-defined failure. |
| Integrity verification failure | GeneralProgrammingFailure (`0x72`) or product-specific failure. |
| Activation not available | ConditionsNotCorrect (`0x22`). |

The public NRC reference documents these common meanings and values [3]. The product diagnostic specification remains authoritative for any additional NRCs, timing, or retry rules.

## Recovery and acceptance evidence

A production release must retain evidence for interrupted downloads, power loss during erase and program, repeated TransferData blocks, missing and duplicate RequestTransferExit, invalid CRC, brownout reset, watchdog expiry, rollback, and bootloader refusal of an invalid image. The runner `tests/hardware/run_stm32f767_uds_acceptance.py` keeps download disabled by default and requires `--enable-download` for the policy check. This is intentional: a hardware test that writes Flash must be executed only on a dedicated, recoverable target with an approved test image.

## References

[1]: https://www.iso.org/standard/72439.html "ISO 14229-1:2020 — Road vehicles — Unified diagnostic services (UDS) — Part 1: Application layer"

[2]: https://www.iso.org/standard/66574.html "ISO 15765-2:2016 — Road vehicles — Diagnostic communication over CAN — Part 2: Transport protocol and network layer services"

[3]: https://uds.readthedocs.io/en/v0.1.0/autoapi/uds/messages/nrc/index.html "UDS NRC public reference documentation"
