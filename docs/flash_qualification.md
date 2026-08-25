# STM32F767 Flash Persistence Qualification

## Scope and current map

The firmware uses a project-owned STM32F7 Flash backend with dual-slot validation, CRC, and sequence metadata. The current linker configuration reserves the final 512 KiB region beginning at `0x08180000` for nonvolatile storage while leaving 1536 KiB for application Flash. This map is a checked-in software assumption, not a completed hardware qualification: the exact STM32F767 density, package, sector boundaries, bootloader reservation, option bytes, and production memory map must be signed off before release.

## Power-interruption matrix

| Injection point | Required result |
|---|---|
| During sector erase | Boot selects the last valid slot or safe defaults; interrupted slot is rejected |
| During payload write | Partial payload is rejected by CRC/metadata validation |
| During CRC handling | No partially committed image becomes active |
| During metadata write | Older valid slot remains usable |
| During commit marker write | Commit state is fail-closed and rollback is deterministic |
| Immediately after commit | Newest valid slot is selected after reboot |
| Both slots invalid | Factory/default configuration is loaded in a documented safe state |

Each interruption must be repeated across representative configurations and selected voltage/temperature corners. Record the power-switch timing, reset cause, slot headers, CRC result, selected sequence, restored values, and raw serial/debug log.

## Endurance budget

The product owner must define the expected configuration writes per year and service life. The required endurance is:

```text
expected writes/year × expected service life = required erase/write endurance
```

The evidence package must include the MCU documentation revision, sector erase rating, selected store-rate policy, minimum store interval, commissioning behavior, and the behavior of `0x1010`/`0x1011` when storage is unavailable. Automatic unrestricted stores are not acceptable as a production policy.

## Release gate

Host storage tests and a successful ARM link do not prove power-loss atomicity, Flash endurance, voltage corner behavior, or the physical correctness of the linker reservation. The campaign remains **PENDING hardware** until the exact production MCU and board execute the matrix and the linker map is independently reviewed.
