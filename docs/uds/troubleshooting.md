# UDS Troubleshooting

| Symptom | Checks |
|---|---|
| No response | Confirm `CANOPEN_REFERENCE_ENABLE_UDS=1`, request/response IDs, transceiver state, exact FIFO1 filters, CAN notifications, and that the main loop calls `CANopenReference_UDS_Process()`. |
| `0x13` negative response | Check UDS SID length and subfunction encoding. |
| `0x31` or `0x22` | Check DID, active session, security level, callback availability, and policy range. |
| `0x35`, `0x36`, or `0x37` | Check the configured SecurityAccess provider, key length, attempt count, and lockout timer. The checked-in provider is non-production. |
| `0x70`, `0x71`, `0x72`, or `0x73` | Check download address/length/alignment, callback state, CRC, timeout, and block sequence. |
| Multi-frame timeout | Check ISO-TP P2/inter-frame timing, STmin, block size, bus load, and whether the tester sends flow control on the configured direction. |
| RX overflow | Inspect adapter statistics and reduce bus load or increase the compile-time queue only after timing and RAM evidence. Never silently drop the diagnostic evidence. |
| CANopen regression | Verify UDS identifiers are not duplicated in CANopen filters, UDS is in FIFO1, CANopen remains in FIFO0, and the bounded UDS budget is not called from the CAN ISR. |
| CubeMX regeneration failure | Re-check the generated CAN handle, callback-registration macro, FIFO notifications, timer, source list, include paths, and USER CODE boundaries. |

Use the host contract tests and dry-run hardware runner before touching a board. Use a dedicated recoverable target for reset and download operations. Record the firmware SHA and all configuration inputs with any failure report.
