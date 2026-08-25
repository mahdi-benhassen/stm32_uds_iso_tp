# CubeMX Integration

The `stm32f767_canopen_cubemx` branch keeps CubeMX-generated infrastructure authoritative. `Core/` and `Drivers/` contain generated HAL and startup infrastructure; the UDS implementation is in project-owned `App/` and `library/compat/legacy_diagnostics/` sources. The generated CAN handle is reused; the UDS layer does not call a second CAN initializer.

The branch-specific configuration must retain the generated CAN1 GPIO/NVIC configuration, the 1 ms CANopen timer, and the generated `stm32f7xx_hal_conf.h`. HAL CAN callback registration is enabled so the UDS adapter can attach to FIFO1 without replacing CANopenNode’s FIFO0 callback. After regeneration, confirm that the callback macro, CAN handle, FIFO notifications, filter bank split, and UDS source list remain present.

The CubeMX branch is bare-metal, not FreeRTOS. UDS runs from the bounded main loop. The CAN callback only drains FIFO1, copies frames into the static UDS ring, and updates counters. It does not dispatch UDS services, call Flash, or block.

The branch-specific CMake target adds the diagnostic sources and headers, exposes `CANOPEN_REFERENCE_ENABLE_UDS`, and retains the existing generated `stm32cubemx` library. Use the branch’s own CMake presets and linker inputs for target builds; do not copy the main branch’s generated files blindly.
