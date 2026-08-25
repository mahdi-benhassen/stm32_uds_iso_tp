# STM32 FDCAN adapter

This example binds the standalone endpoint to a generated STM32 project with an FDCAN peripheral, such as an STM32G4, STM32H5, or STM32H7 device selected by the integrator. The binding forwards the standard ID, data pointer, actual CAN-FD data length, and `bit_rate_switch` metadata through `Stm32FdCanBinding::send_fd`.

The board project must translate the data length to its vendor HAL message-length enum, configure nominal and data bit rates, allocate FDCAN message RAM, configure filters and interrupt routing, control the CAN-FD transceiver, and copy received frames back into `IsoTpCanFrame` with `is_fd=true`. The endpoint configuration defaults to 64-byte transmit/receive lengths and BRS enabled; the generated project remains responsible for matching those settings to its peer.

No fake vendor HAL build is included. A physical CAN-FD HIL claim requires a selected concrete MCU/board, transceiver, wiring, nominal/data bit rates, peer ECU or analyzer, and captured evidence. The adapter contract itself is host compile-checked by the standalone CMake build.
