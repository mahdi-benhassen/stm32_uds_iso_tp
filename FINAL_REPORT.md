# Standalone UDS / ISO-TP completion report

## Outcome

The repository is now structured as an independent ISO 15765-2 ISO-TP and ISO 14229 UDS project. The first historical commit remains the exact requested CubeMX snapshot, while the current tree removes the unrelated protocol stack, application layer, object-dictionary artifacts, submodule, and build/test dependencies. The complete pre-removal inventory is recorded in [`docs/architecture/canopen_removal_audit.md`](docs/architecture/canopen_removal_audit.md).

> This repository implements ISO 15765-2 ISO-TP and ISO 14229 UDS independently of CANopen. CANopen is neither required nor included.

## Final architecture

```text
UDS ISO 14229
      |
      v
ISO-TP ISO 15765-2
      |
      v
CAN/CAN-FD callback abstraction
      |
      +-- STM32F767 bxCAN / Classical CAN
      +-- FDCAN-capable STM32 / CAN FD
```

The authoritative implementation is limited to `library/include/uds_iso_tp/` and `library/src/`. The root STM32F767 application is a minimal UDS-only target using explicit diagnostic identifiers `0x7E0` and `0x7E8`. No CANopen submodule, protocol source, object dictionary, profile artifact, or optional protocol switch remains in the build graph.

## Removed and migrated material

| Category | Action |
|---|---|
| Submodule metadata and gitlink | Removed `.gitmodules` and `third_party/CanOpenSTM32`. |
| Protocol middleware | Removed the inherited CANopen core, ports, examples, gateway, and legacy diagnostic implementation. |
| Application layer | Removed CANopen lifecycle, profile, object-dictionary, storage, gateway, LSS, watchdog, and recovery files; added `App/Src/uds_app.c`, `can_transport.c`, and `uds_platform.c`. |
| Generated protocol artifacts | Removed object-dictionary sources, EDS files, profile data, and protocol-specific linker reservation. |
| Tests and tooling | Removed protocol-specific host, fuzz, gateway, profile, and acceptance suites; retained standalone CTest, architecture, adapter, and HIL tests. |
| Documentation and CI | Rewrote build, scope, porting, contributor, changelog, third-party, README, and pull-request guidance; removed the old protocol workflow and expanded standalone CI. |
| Generic functionality | Retained generated STM32 HAL/CMSIS target infrastructure and expressed CAN transport and timing through neutral application-owned callbacks. |

## Validation evidence

The standalone host build passes all four CTest contracts. The sanitizer build passes the same contracts. The ISO-TP matrix covers Classical CAN and CAN FD, explicit TX states, CTS, bounded WAIT, immediate OVERFLOW, BS and STmin validation, reserved STmin rejection, wrong CAN ID, invalid DLC/PCI, timeout, and sequence errors. The architecture check passes for all tracked project paths, and the HIL runner’s Classical CAN and CAN-FD dry-run reports complete successfully.

The cleaned STM32F767 root target was cross-built successfully with `arm-none-eabi-gcc`. The resulting Debug image used approximately 22.7 KiB of Flash and 38.2 KiB of RAM in the local generated target build. Hosted workflow run [32802035769](https://github.com/mahdi-benhassen/stm32_uds_iso_tp/actions/runs/32802035769) for cleanup commit `b53cce6` completed successfully, including the architecture check, root cross-build, coverage instrumentation, sanitizer, static-analysis, and HIL dry-run sequence.

## Remaining limitations

A host or cross-build does not prove electrical signaling, transceiver behavior, target interrupt latency, message-RAM configuration, bus-off recovery, EMC performance, production cryptography, authenticated firmware activation, Flash power-loss behavior, or formal ISO conformance. The FDCAN example remains an adapter contract and requires a concrete FDCAN-capable STM32 board project for physical CAN-FD HIL.

## Issue status

Issue #2 is the governing cleanup request. The cleanup commit is published and hosted CI verifies the final clean checkout; the issue remains open for maintainer review rather than being closed automatically. Issue #16 in the original `stm32_canopen_reference` repository remains independent and unchanged; its freeze and production-evidence status are not altered by this standalone cleanup.
