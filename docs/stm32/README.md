# STM32 integration

The repository includes a minimal STM32F767 bxCAN application and reusable adapter contracts for Classical CAN and CAN FD. The F767 target is Classical CAN only because its bxCAN peripheral does not implement CAN FD. A CAN-FD target requires a concrete FDCAN-capable STM32 board project with vendor-generated message-RAM, clock, GPIO, NVIC, filter, and transceiver configuration.

See [`examples/stm32f767_bxcan/`](../../examples/stm32f767_bxcan/) for the `HAL_CAN` binding contract, [`examples/stm32_fdcan/`](../../examples/stm32_fdcan/) for the CAN-FD metadata contract, and [`BUILD.md`](../../BUILD.md) for the root cross-build. Physical HIL evidence must record the exact board, transceiver, timing, wiring, peer equipment, and captured traces.
