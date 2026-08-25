# STM32 UDS / ISO-TP

This repository was initialized from the frozen `stm32f767_canopen_cubemx` branch of [`stm32_canopen_reference`](https://github.com/mahdi-benhassen/stm32_canopen_reference). The initial commit preserves that CubeMX firmware snapshot. Subsequent commits add an independent, reusable UDS/ISO-TP library without a CANopenNode dependency.

The project has two deliberate boundaries. The **library** in [`library/`](library/) provides protocol-neutral ISO-TP and UDS code with fixed storage and injected frame/clock callbacks. The **examples** in [`examples/`](examples/) show how a board project binds those callbacks to STM32F767 bxCAN for Classical CAN or to an FDCAN-capable STM32 HAL project for CAN FD. The copied CubeMX firmware remains available for integration work and is not silently replaced.

## Transport profiles

| Profile | Frame support | Intended target | Status |
|---|---|---|---|
| Classical CAN | 8-byte CAN data, SF/FF/CF/FC, payloads up to the configured bound | STM32F767 bxCAN | Implemented and host-tested. |
| CAN FD | 8/12/16/20/24/32/48/64-byte DLCs, CAN-FD SF escape, extended FF length above 4,095 bytes, optional BRS | FDCAN-capable STM32 or external CAN-FD controller | Implemented as a transport core and adapter contract; physical HIL requires the selected CAN-FD board. |

The default standalone library bound is 16,384 bytes and all buffers are static. A product may lower this bound at configuration time after checking its memory budget. The STM32F767 example remains Classical CAN because the F767 project uses bxCAN rather than an integrated FDCAN peripheral.

## Build the independent library

```sh
cmake -S library -B build-standalone -G Ninja \
  -DUDS_ISO_TP_BUILD_TESTS=ON \
  -DUDS_ISO_TP_MAX_PAYLOAD=16384
cmake --build build-standalone --parallel
ctest --test-dir build-standalone --output-on-failure
```

The tests exercise Classical CAN framing, CAN-FD 64-byte Single Frames, extended First-Frame lengths above 4,095 bytes, reassembly, UDS service contracts, invalid FlowStatus, and bounded error handling. Sanitizer and static-analysis commands are described in [`docs/standalone/validation.md`](docs/standalone/validation.md).

## Examples

[`examples/stm32f767_bxcan/`](examples/stm32f767_bxcan/) contains the board binding contract for generated STM32F767 `HAL_CAN` projects. [`examples/stm32_fdcan/`](examples/stm32_fdcan/) contains the binding contract for a generated STM32 FDCAN project; it carries the CAN-FD data length and BRS metadata to the vendor HAL. These examples compile against the standalone library but intentionally do not invent vendor-generated clock, GPIO, NVIC, message-RAM, or linker files.

## Safety and production boundaries

The library has no heap allocation, blocking interrupt work, `printf` dependency, or CANopenNode dependency. UDS service callbacks, security, download memory policy, reset policy, and physical adapter behavior remain product-owned. The checked-in deterministic SecurityAccess provider is for tests, not production. No bootloader or authenticated Flash activation is claimed.

## Documentation

| Topic | Document |
|---|---|
| Library architecture | [`docs/standalone/architecture.md`](docs/standalone/architecture.md) |
| Classical CAN and CAN-FD ISO-TP | [`docs/standalone/isotp.md`](docs/standalone/isotp.md) |
| STM32 examples | [`docs/standalone/stm32_examples.md`](docs/standalone/stm32_examples.md) |
| HIL and CAN-FD evidence | [`docs/standalone/hil.md`](docs/standalone/hil.md) |
| Validation and release gates | [`docs/standalone/validation.md`](docs/standalone/validation.md) |
| Release-readiness audit | [`docs/standalone/release_audit.md`](docs/standalone/release_audit.md) |
| Original CubeMX snapshot | [`docs/uds/`](docs/uds/) and [`BUILD.md`](BUILD.md) |
| Freeze baseline | [`issue16-classic-can-uds-cubemx-v1.0.0`](https://github.com/mahdi-benhassen/stm32_canopen_reference/releases/tag/issue16-classic-can-uds-cubemx-v1.0.0) |

Project-owned source remains under the repository’s source-available license. Third-party and copied snapshot components retain their applicable upstream licensing and notices. Review [`LICENSE`](LICENSE) and [`THIRD_PARTY.md`](THIRD_PARTY.md) before redistribution.
