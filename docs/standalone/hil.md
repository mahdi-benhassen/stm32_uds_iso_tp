# HIL Acceptance

HIL must use an independent CAN interface connected to the selected target board. The host runner must record a monotonic timestamp, interface name, CAN identifier, frame format, DLC, data bytes, response latency, NRC or positive response, and verdict for every transaction. A dry run validates inventory and report generation only; it is not hardware evidence.

| Campaign | Classical CAN target | CAN-FD target |
|---|---|---|
| Session and TesterPresent | STM32F767 bxCAN | FDCAN-capable STM32 |
| SF boundary | 7-byte payload | 7-byte and escaped 62-byte payload at DLC 64 |
| Multi-frame | 4,095-byte FF boundary | 4,095-byte and >4,095-byte extended FF |
| Flow control | CTS, WAIT limit, OVERFLOW, BS, STmin | Same plus DLC 64 and BRS |
| Malformed traffic | Sequence, timeout, invalid PCI, invalid DLC | Same plus invalid CAN-FD DLC and format mismatch |
| Coexistence | NMT, heartbeat, SDO, PDO, EMCY | CANopen traffic only where the selected FDCAN system supports it |
| Recovery | CAN error, bus-off, reset, watchdog | CAN-FD error states, bus-off, reset, watchdog |

Reset, download, Flash erase/program, and activation tests are disabled by default. They require explicit operator flags, a sacrificial image, power-control capability, recovery access, and a signed test plan. A production update campaign additionally requires a bootloader, authenticated image verification, rollback behavior, and power-loss testing.

The current repository contains the reusable runner/report contract and adapter examples. No physical CAN-FD HIL result is claimed until a concrete FDCAN board and independent analyzer/interface are selected.
