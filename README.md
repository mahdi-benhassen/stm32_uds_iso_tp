# STM32 UDS / ISO-TP

> This repository implements ISO 15765-2 ISO-TP and ISO 14229 UDS independently of CANopen. CANopen is neither required nor included.

The repository provides a bounded, heap-free UDS and ISO-TP implementation with a generic CAN/CAN-FD frame boundary, STM32 adapter contracts, a minimal STM32F767 bxCAN application, host tests, HIL inventory tooling, and reproducible build and analysis workflows. The first historical commit is the exact CubeMX snapshot requested from the related reference project; the current tree is the audited standalone result. See the [CANopen removal audit](docs/architecture/canopen_removal_audit.md) for provenance and migration evidence.

## Architecture

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

The authoritative implementation is in [`library/include/uds_iso_tp/`](library/include/uds_iso_tp/) and [`library/src/`](library/src/). It does not include CANopen headers, link CANopen sources, or use CANopen object-dictionary concepts. CAN identifiers such as `0x7E0` and `0x7E8` are explicit diagnostic configuration parameters.

## Transport profiles

| Profile | Frame support | Intended target | Status |
|---|---|---|---|
| Classical CAN | 8-byte CAN data, SF/FF/CF/FC, bounded payloads | STM32F767 bxCAN | Implemented and host-tested |
| CAN FD | 8/12/16/20/24/32/48/64-byte DLCs, 64-byte SF, extended FF length above 4,095 bytes, optional BRS | FDCAN-capable STM32 or external CAN-FD controller | Implemented as core and adapter contract; board HIL remains required |

The default library payload bound is 16,384 bytes and all buffers are static. The transport can carry a 5,000-byte extended First Frame. The UDS dispatcher remains separately bounded by its documented `uint16_t` callback API.

## Build and test the library

```sh
cmake -S library -B build-standalone -G Ninja \
  -DUDS_ISO_TP_BUILD_TESTS=ON \
  -DUDS_ISO_TP_BUILD_EXAMPLES=ON \
  -DUDS_ISO_TP_MAX_PAYLOAD=16384
cmake --build build-standalone --parallel
ctest --test-dir build-standalone --output-on-failure
```

The suite covers Classical CAN and CAN-FD framing, Flow Control CTS/WAIT/OVERFLOW, BS and STmin validation, reserved values, wrong IDs, invalid DLC/PCI, timeout, sequence errors, UDS service contracts, and both STM32 adapter contracts. Sanitizer, coverage, formatting, clang-tidy, cppcheck, and HIL dry-run commands are documented in [`docs/standalone/validation.md`](docs/standalone/validation.md).

## STM32F767 application

The generated STM32F767 project at the repository root is now a minimal UDS-only application. It initializes bxCAN, uses request ID `0x7E0`, functional request ID `0x7DF`, and response ID `0x7E8`, defers received frames from the HAL callback to the main loop, tracks mailbox completion, and composes the authoritative endpoint. A real cross-build requires the ARM GCC toolchain and the generated STM32 HAL sources included in the project. Tag-triggered firmware packaging and the protected optional hardware-flash job are documented in [`docs/release/stm32f767_tag_release.md`](docs/release/stm32f767_tag_release.md).

The reusable bindings are documented in [`examples/stm32f767_bxcan/`](examples/stm32f767_bxcan/) and [`examples/stm32_fdcan/`](examples/stm32_fdcan/). The FDCAN example deliberately remains a contract rather than an invented vendor-generated board project.

## Documentation

| Topic | Document |
|---|---|
| CANopen removal audit | [`docs/architecture/canopen_removal_audit.md`](docs/architecture/canopen_removal_audit.md) |
| Library architecture | [`docs/standalone/architecture.md`](docs/standalone/architecture.md) |
| Classical CAN and CAN-FD ISO-TP | [`docs/standalone/isotp.md`](docs/standalone/isotp.md) |
| STM32 examples | [`docs/standalone/stm32_examples.md`](docs/standalone/stm32_examples.md) |
| HIL and CAN-FD evidence | [`docs/standalone/hil.md`](docs/standalone/hil.md) |
| Validation and release gates | [`docs/standalone/validation.md`](docs/standalone/validation.md) |
| Release-readiness audit | [`docs/standalone/release_audit.md`](docs/standalone/release_audit.md) |
| STM32F767 tagged firmware releases and optional flash | [`docs/release/stm32f767_tag_release.md`](docs/release/stm32f767_tag_release.md) |

The project-owned material remains under its source-available research/education and commercial licensing terms. Review [`LICENSE`](LICENSE), [`COMMERCIAL-LICENSE.md`](COMMERCIAL-LICENSE.md), and [`THIRD_PARTY.md`](THIRD_PARTY.md) before redistribution. No production security, authenticated firmware activation, or physical CAN-FD HIL claim is made by the host validation alone.
