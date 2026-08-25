# STM32F767 bxCAN adapter

This example binds the standalone endpoint to an STM32F767 project using the STM32 HAL `CAN_HandleTypeDef`/bxCAN peripheral. Because bxCAN is Classical CAN, the binding rejects `IsoTpCanFrame` values marked as CAN FD and forwards only an 8-byte data field to the board-owned send callback.

A generated application supplies `send_classic` by translating the standard ID, data bytes, and actual data length into its `HAL_CAN_AddTxMessage()` call. It should supply `now_ms` from the application time base. Receive interrupts should copy a received frame into `IsoTpCanFrame`, set `can_id`, `dlc`, and `is_fd=false`, then defer `uds_isotp_endpoint_receive()` and `uds_isotp_endpoint_process()` to the main loop or a bounded task.

This directory intentionally contains no vendor-generated clocks, filters, GPIO, NVIC, linker script, or transceiver configuration. The F767 path is a real Classical-CAN example, not a CAN-FD claim.
