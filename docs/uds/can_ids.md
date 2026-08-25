# CAN IDs and Addressing

The default UDS addressing is physical classic-CAN communication with standard 11-bit identifiers. The defaults are not universal and must be selected against the product network plan.

| Direction | Symbol | Default |
|---|---|---:|
| Tester request to ECU | `UDS_RX_CAN_ID` | `0x7E0` |
| ECU response to tester | `UDS_TX_CAN_ID` | `0x7E8` |

The STM32 adapter accepts both configured identifiers into its bounded FIFO1 queue. Frames with unrelated IDs are rejected at the adapter boundary. The ISO-TP receiver processes only `UDS_RX_CAN_ID`; the ISO-TP transmitter accepts flow-control frames only from that request-side identifier and emits responses on `UDS_TX_CAN_ID`.

CANopen identifiers remain in FIFO0. UDS request and response identifiers are configured in a separate exact-ID FIFO1 filter bank. The filter list must be reviewed whenever the node ID, PDO COB-IDs, SDO COB-IDs, LSS configuration, or diagnostic IDs are changed. A product must also check for duplicate IDs and validate acceptance-filter capacity before deployment.

This is physical addressing only. Functional addressing, broadcast diagnostics, extended 29-bit identifiers, extended or mixed ISO-TP addressing, CAN FD, and gateway routing are not enabled by the reference adapter. Any of those modes require explicit product policy and additional tests.
