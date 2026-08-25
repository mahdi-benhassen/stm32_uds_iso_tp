# UDS Flash Programming

The reference implementation provides a bounded UDS download interface, not a production bootloader. `RequestDownload (0x34)`, `TransferData (0x36)`, and `RequestTransferExit (0x37)` call product-owned callbacks for erase, program, verification, and activation policy.

The download state machine validates address, length, alignment, declared transfer size, block sequence, timeout, and CRC32. It must never overwrite the bootloader, active application, CANopen parameter NVM, or diagnostic persistent storage. The staging image region is a reference boundary only and must be replaced with the exact linker and bootloader memory map.

Erase and program operations execute outside interrupt context. The watchdog callback is invoked only from bounded progress paths. A failed, timed-out, interrupted, or CRC-invalid transfer must clear activation-pending state and preserve the active application. Successful integrity verification may mark an image pending, but the checked-in reference has no activation-capable bootloader and therefore leaves final activation/reboot pending.

CRC32 detects accidental corruption; it does not provide authenticity. A production implementation requires signed image verification, key management, anti-rollback policy, power-loss-safe metadata, boot-attempt counters, rollback, and recovery evidence.
