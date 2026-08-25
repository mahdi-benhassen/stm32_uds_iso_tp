# Product Scope

## Purpose

This repository is a **reference firmware platform** for STM32F767 CANopen development. It is not, by itself, a finished device, drive, gateway, safety product, or field-update system. The current v1 product path selects CiA 401 as its production-personality candidate; the definition and unresolved owner sign-offs are recorded in [`PRODUCT_CIA401.md`](PRODUCT_CIA401.md). A production product must select one declared personality, freeze its Object Dictionary and hardware design, and produce the required hardware, reliability, security, manufacturing, electrical, and conformance evidence.

## Declared reference personalities

| Personality | Default status | Supported reference behavior | Not supported as a production claim |
|---|---|---|---|
| CiA 401 I/O device | Default | CANopenNode communication services, generated OD integration, board I/O seams, safe startup hooks, and profile-oriented host tests | Product-specific electrical limits, channel calibration, debounce requirements, board diagnostics, HIL evidence, and formal CiA 401 conformance |
| CiA 402 drive interface | Optional | Reference state/controlword/statusword/fault-reset behavior and Profile Position/Velocity adapter seams | Complete drive product, torque/homing/CSP/CSV/CST behavior, motor feedback, power-stage safety, limits, following-error behavior, HIL, and formal conformance |
| CiA 302 NMT-master supervision | Optional | Bounded configured-peer boot-up and heartbeat supervision with deterministic host tests; standard Network List/Configuration Manager objects 0x1F80–0x1F89 are not implemented | Complete network list, configuration manager, commissioning workflow, startup configuration manager, multi-node production evidence, and formal conformance |
| CiA 309 gateway foundation | Optional | Bounded gateway foundation with deny-by-default policy tests | Authenticated production transport, authorization model, audit trail, security approval, and gateway conformance |
| Inventus battery test OD | Explicit opt-in test-only profile | Workbook-derived manufacturer-specific `0x4800–0x4921` application entries, Issue #12 battery application objects `0x6000–0x6081`, structured `0xD000`, bounded `0xD001`, generated OD/EDS artifacts, profile-local data seam, and host validation | Commercial use, production default selection, approved safety semantics, hardware conformance, and formal CiA 418/product conformance |

## Protocol and feature boundaries

| Feature | Repository status | Production interpretation |
|---|---|---|
| NMT, heartbeat, EMCY, SDO, PDO, SYNC | Integrated through CANopenNode and project configuration | Requires product OD approval, board testing, stress testing, and applicable conformance evidence |
| LSS | Stack integration and project policy hooks | Complete Fastscan commissioning and multi-node provisioning are not claimed |
| UDS / ISO-TP | Host-side contract models | No embedded UDS server or embedded ISO-TP implementation is claimed |
| CiA 418 | Explicit opt-in adapter plus synchronized generated model artifact; an isolated Inventus battery OD/EDS is available only for non-commercial testing | No CiA 418 or Inventus battery production device profile is claimed; hardware-owner approval, safety semantics, physical evidence, and formal conformance remain absent |
| NMEA 2000 | Host gateway contract only; no embedded C implementation | No embedded NMEA 2000 stack or field interoperability is claimed; address-claim state machine is also absent |
| CAN-FD/FDCAN | Not implemented; target uses bxCAN | No CAN-FD capability is claimed |
| Bootloader and firmware update | Not implemented | No secure update, signature verification, rollback, or anti-rollback capability is claimed |
| Secure boot and product security | Repository policy and release boundaries only | No secure boot, key provisioning, firmware authentication, or production debug-lock evidence is claimed |

## Evidence rule

A host test, source-contract test, sanitizer run, fuzz run, or cross-build is **software evidence only**. It does not establish physical CAN behavior, electrical compliance, EMC performance, watchdog timing on silicon, Flash power-loss tolerance, manufacturing repeatability, security approval, or formal CANopen conformance.

The minimum product evidence for a selected personality includes the exact firmware SHA, OD/EDS/XDD hashes, build manifest, board revision, transceiver and termination details, captured CAN traces, environmental conditions, test results, and independent review or conformance records where applicable.

## Release levels

| Release level | Meaning |
|---|---|
| `v0.9.0` Historical hardware-validation candidate | Historical candidate tag retained at `9c04ef2`; it records an earlier software baseline. |
| `v0.9.0-rc1` Hardware Validation Candidate | Immutable candidate tag at `509b49c`; selected CiA 401 product path and current software remediation baseline. External evidence is explicitly pending. |
| `v1.0.0` Production release | All mandatory product requirements pass implementation, automated verification where applicable, physical/HIL verification where applicable, documented acceptance criteria, recorded evidence, and approved release review. |

## Scope-change rule

Adding a protocol or profile to a build flag, test model, generated artifact, or example does not make it a supported production feature. A scope change requires an update to this document, the feature matrix, the Object Dictionary/evidence plan, the release checklist, and the corresponding implementation and verification records.
