# UDS / ISO-TP Documentation

This directory is the authoritative documentation set for the bounded STM32F767 UDS reference profile, enabled by default in the current reference build and disableable at compile time. It describes the separation between CANopenNode, the protocol-independent ISO-TP and UDS cores, the STM32 FIFO1 adapter, generated CubeMX infrastructure, and product-owned callbacks.

| Document | Scope |
|---|---|
| [architecture.md](architecture.md) | Module boundaries, ownership, ISR/mainline safety, and CANopen coexistence. |
| [isotp.md](isotp.md) | Classic-CAN ISO-TP frames, state machines, timers, and bounds. |
| [services.md](services.md) | Supported, gated, and unsupported UDS services and NRCs. |
| [configuration.md](configuration.md) | Compile-time switches, identifiers, queues, and timing defaults. |
| [can_ids.md](can_ids.md) | Physical addressing, exact filters, and collision review. |
| [timing.md](timing.md) | DWT instrumentation, budgets, campaigns, and pending measurements. |
| [security.md](security.md) | Replaceable SecurityAccess boundary and non-production provider warning. |
| [flash_programming.md](flash_programming.md) | Download state machine, protected regions, recovery, and activation boundary. |
| [stm32f767.md](stm32f767.md) | MCU, bxCAN, TIM7, memory, and board obligations. |
| [cubemx_integration.md](cubemx_integration.md) | CubeMX-generated ownership and callback integration. |
| [hil_testing.md](hil_testing.md) | Independent SocketCAN runner and physical evidence requirements. |
| [troubleshooting.md](troubleshooting.md) | Diagnostic failure and regeneration checks. |
| [ISSUE16_PRODUCTION_AUDIT.md](ISSUE16_PRODUCTION_AUDIT.md) | Independent requirement-by-requirement audit. |

This documentation does not claim complete ISO 14229, ISO 15765-2, or ISO 11898 conformance. Host tests, fake-HAL tests, and dry-run reports are software evidence; target timing, electrical CAN behavior, bus-off, watchdog, Flash power-loss, security, and HIL evidence remain separate gates.
