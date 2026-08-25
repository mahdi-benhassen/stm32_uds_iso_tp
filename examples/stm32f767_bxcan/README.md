# STM32F767 bxCAN adapter

This example binds the standalone endpoint to an STM32F767 project using the STM32 HAL `CAN_HandleTypeDef`/bxCAN peripheral. Because bxCAN is Classical CAN, the binding rejects `IsoTpCanFrame` values marked as CAN FD and forwards only an 8-byte data field to the board-owned send callback.

A generated application supplies `send_classic` by translating the standard ID, data bytes, and actual data length into its `HAL_CAN_AddTxMessage()` call. It should supply `now_ms` from the application time base. Receive interrupts should copy a received frame into `IsoTpCanFrame`, set `can_id`, `dlc`, and `is_fd=false`, then defer `uds_isotp_endpoint_receive()` and `uds_isotp_endpoint_process()` to the main loop or a bounded task.

The maintained STM32F767 transport does not contain a duplicate `tx_pending` boolean. ISO-TP owns protocol-level pending/in-flight sequencing in the generic endpoint. The bxCAN adapter records only the bitmask of HAL mailboxes accepted but not yet idle, and `uds_can_transport_tx_complete()` clears that mask when the controller reports no outstanding mailbox. This is hardware completion polling, not an ECUReset flag. `HAL_CAN_AddTxMessage() == HAL_OK` means controller acceptance only; it does not by itself prove physical bus transmission.

For ECUReset (`11 01`), the endpoint produces `51 01`, submits the frame through this thin adapter, and applies the configured completion contract. The F767 application may use the mailbox polling callback for deferred completion; the generic endpoint, not bxCAN `tx_pending`, owns the exactly-once reset transition.

This directory intentionally contains no vendor-generated clocks, filters, GPIO, NVIC, linker script, or transceiver configuration. The F767 path is a real Classical-CAN example, not a CAN-FD claim.
