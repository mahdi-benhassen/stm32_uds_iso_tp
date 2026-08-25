# Architecture

> This repository implements ISO 15765-2 ISO-TP and ISO 14229 UDS independently of CANopen. CANopen is neither required nor included.

The authoritative dependency graph is:

```text
UDS ISO 14229
      |
      v
ISO-TP ISO 15765-2
      |
      v
CAN/CAN-FD callback abstraction
      |
      +-- STM32F767 bxCAN / Classical CAN
      +-- FDCAN-capable STM32 / CAN FD
```

See the [CANopen removal audit](canopen_removal_audit.md) for the pre-removal inventory, file actions, submodule review, and residual-reference policy. See [standalone architecture](../standalone/architecture.md) for module ownership and [validation gates](../standalone/validation.md) for reproducible evidence.
