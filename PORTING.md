# Porting the standalone UDS / ISO-TP stack

The repository separates protocol code from the target platform. The authoritative implementation in `library/` depends only on fixed-width types, configured storage, an injected clock, and injected CAN/CAN-FD frame callbacks. A target application supplies the hardware binding and application callbacks.

## Layer boundaries

| Layer | Location | Responsibility |
|---|---|---|
| UDS and ISO-TP | `library/include/uds_iso_tp`, `library/src` | Protocol state machines, bounded buffers, service dispatch, and transport framing |
| Target bindings | `examples/stm32f767_bxcan`, `examples/stm32_fdcan` | Frame metadata conversion between the generic API and vendor HAL types |
| STM32F767 application | `App/Inc`, `App/Src`, `Core/` | HAL initialization, deferred RX handoff, main-loop scheduling, and application-owned UDS callbacks |
| Host validation | `library/tests`, `tests/architecture`, `tests/standalone` | Deterministic protocol contracts, architecture checks, and safety-gated HIL inventory |

## CubeMX integration rules

Keep generated clock, GPIO, CAN, NVIC, startup, and linker files under the generated project areas. Add target-specific logic through application-owned source files and explicit callback boundaries. The RX interrupt must only copy or enqueue a frame; UDS and ISO-TP dispatch belongs in the mainline or another non-interrupt execution context.

For bxCAN, configure the generic frame as Classical CAN with an actual data length of at most eight bytes. For FDCAN, propagate the actual valid CAN-FD data length and bit-rate-switch flag; do not encode CAN-FD behavior through a Classical CAN DLC value. The FDCAN example is intentionally a binding contract and must be completed with a concrete CubeMX-generated board project before physical HIL.

## Addressing

Configure request and response CAN identifiers explicitly in the application or endpoint configuration. The reference F767 application uses request `0x7E0` and response `0x7E8`. These are diagnostic identifiers, not network-management or object-dictionary identifiers. Extended and functional addressing require a separate API design and test matrix before use.

## Application callbacks

UDS service callbacks own application policy. A product must provide its own DID values, SecurityAccess provider, reset policy, download memory map, erase/program callbacks, and any physical safety interlocks. The deterministic security provider in the repository is test-only. Keep all Flash, bootloader, reset, and authentication decisions outside the transport ISR.

## Verification

Run the host CTest suite, architecture check, sanitizer build, formatting, clang-tidy, cppcheck, and both HIL dry-run profiles before flashing. Record the exact MCU, HAL revision, compiler, transceiver, nominal/data bit rates, message-RAM configuration, CAN IDs, wiring, peer equipment, and captured traces for a physical campaign.
