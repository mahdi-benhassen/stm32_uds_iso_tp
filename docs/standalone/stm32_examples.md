# STM32 Examples

## STM32F767 bxCAN / Classical CAN

The `stm32f767_bxcan` example binds the endpoint to a generated STM32F7 `HAL_CAN` project through two callbacks: one for a non-blocking eight-byte CAN submission and one for a millisecond clock. It configures `isotp_config_classic_can()` and uses the reference request/response identifiers `0x7E0` and `0x7E8`.

The F767 project in the initial snapshot remains the authoritative source for `CAN_HandleTypeDef`, `MX_CAN1_Init()`, GPIO alternate functions, NVIC priorities, TIM7, linker placement, and transceiver controls. The standalone library does not copy or replace those generated files. The F767 integrated bxCAN controller is therefore a Classical CAN example, not a native CAN-FD example.

## FDCAN-capable STM32

The `stm32_fdcan` example binds the same endpoint contract to a generated FDCAN HAL project. It selects `tx_dl=64`, `rx_dl=64`, and BRS in the transport configuration, then forwards `is_fd`, DLC, and BRS metadata to the vendor send callback. A product integration must provide the actual FDCAN handle, message RAM configuration, nominal/data bit timing, filters, interrupt setup, transceiver mode, and board wiring.

The example is compile-checked as a HAL-independent adapter contract. It is not evidence that the current STM32F767 board can transmit CAN-FD. Real CAN-FD HIL requires an FDCAN-capable MCU or an external CAN-FD controller and a suitable independent CAN-FD interface.

| Example | Native hardware claim | Required HIL evidence |
|---|---|---|
| STM32F767 bxCAN | Classical CAN only | CAN transceiver, filters, timing, bus load, error recovery, and UDS/CANopen coexistence. |
| FDCAN-capable STM32 | CAN-FD possible with selected generated project | Nominal/data bit rates, BRS, DLC 64, extended FF, interoperability, errors, and power/reset recovery. |
