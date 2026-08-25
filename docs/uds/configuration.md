# UDS Configuration

UDS is enabled by default in the current reference build. A product may explicitly disable it with `CANOPEN_REFERENCE_ENABLE_UDS=OFF` after reviewing its diagnostic access policy. The default identifiers are reference values only and remain configurable.

| Configuration | Default | Meaning |
|---|---:|---|
| `CANOPEN_REFERENCE_ENABLE_UDS` | `ON` | Enables the STM32 UDS runtime integration. |
| `UDS_RX_CAN_ID` | `0x7E0` | Tester-to-ECU request identifier. |
| `UDS_TX_CAN_ID` | `0x7E8` | ECU-to-tester response and flow-control input identifier. |
| `ISOTP_MAX_PAYLOAD` | `4095` | Maximum logical classic-CAN ISO-TP payload. |
| `ISOTP_DEFAULT_RX_TIMEOUT_MS` | `1000` | RX inter-frame timeout. |
| `ISOTP_DEFAULT_TX_TIMEOUT_MS` | `1000` | TX flow-control/progress timeout. |
| `UDS_DEFAULT_P2_SERVER_MS` | `50` | Reference P2 server time. |
| `UDS_DEFAULT_P2_STAR_SERVER_MS` | `5000` | Reference P2* server time. |
| `UDS_DEFAULT_S3_SERVER_MS` | `5000` | Reference session inactivity timeout. |
| `UDS_STM32_RX_QUEUE_CAPACITY` | Build-defined | Static ISR-to-mainline RX ring capacity. |
| `UDS_STM32_TX_QUEUE_CAPACITY` | Build-defined | Static mainline-to-mailbox TX ring capacity. |

Service gates such as `UDS_ENABLE_IO_CONTROL_BY_IDENTIFIER` and `UDS_ENABLE_ROUTINE_CONTROL` are compile-time policy switches. Enabling a service does not create a safe product implementation: the application must provide the callback and its authorization, range, timing, and recovery policy.

The CMake build exposes `CANOPEN_REFERENCE_ENABLE_UDS`, `CANOPEN_REFERENCE_UDS_RX_CAN_ID`, and `CANOPEN_REFERENCE_UDS_TX_CAN_ID`. The CubeMX branch additionally requires HAL CAN callback registration to remain enabled so the dedicated FIFO1 callback can be attached without replacing CANopenNode’s FIFO0 callback.
