# CiA 401 Product Definition and Freeze Gate

## Status

**Selected v1 reference personality:** CiA 401 I/O device.

**Current status:** Product-definition freeze candidate. The software personality is selected, but the board-specific electrical values and acceptance sign-offs below remain required before this document can be treated as a production freeze. Unknown hardware values are deliberately marked `TBD`; they must not be inferred from the STM32 reference implementation.

This document is the product boundary for the current v1 path. CiA 402, CiA 418, UDS/ISO-TP, NMEA 2000, CAN-FD, and a complete CiA 302 Network List/Configuration Manager remain outside the v1 production claim unless a separate product decision reopens them.

## Hardware definition

| Item | Current reference fact | Production freeze input |
|---|---|---|
| MCU family | STM32F767 | Exact ordering code and package: **TBD by hardware owner** |
| Flash | Linker profile assumes 2 MiB device density and reserves the final 512 KiB for persistence | Confirm exact MCU density, sector map, option bytes, and bootloader reservation |
| SRAM | Linker profile assumes 512 KiB SRAM | Confirm exact part/package memory map |
| CAN peripheral | CAN1 through the STM32 bxCAN binding | Confirm transceiver part, standby/enable wiring, protection, and connector |
| CAN pins | PA11/PA12, AF9 in the reference board contract | Confirm alternate-function routing, electrical constraints, and board revision |
| oscillator | 25 MHz HSE is the documented reference clock input | Confirm oscillator part, tolerance, load, startup, and clock safety margin |
| supply | Not defined by the generic reference repository | Define nominal, minimum, maximum, brownout threshold, and sequencing |
| termination | Not defined by the generic reference repository | Define switchable/fixed termination, split termination, and grounding policy |
| connector | Not defined by the generic reference repository | Define connector, pinout, shielding, and service access |
| environment | Not defined by the generic reference repository | Define operating/storage temperature, humidity, vibration, and ingress requirements |

## I/O definition

The current CiA 401 adapter exposes the following application seams and generated OD objects. The weak board hooks are intentionally conservative and do not define product electrical behavior.

| Function | CANopen object | Current firmware seam | Product decision required |
|---|---:|---|---|
| Digital input bank | `0x6000:01` | `CANopenReferenceHw_ReadDigitalInputs()` | Number of channels, voltage thresholds, polarity, debounce, diagnostics, and fault value |
| Digital output bank | `0x6200:01` | `CANopenReferenceHw_WriteDigitalOutputs()` | Number of channels, polarity, output type, current limit, short-circuit response, and safe state |
| Analog input 1 | `0x6401:01` | `CANopenReferenceHw_ReadAnalogInput(1)` | Sensor range, units, ADC scaling, calibration, filtering, saturation, and invalid-value behavior |
| Analog input 2 | `0x6411:01` | `CANopenReferenceHw_ReadAnalogInput(2)` | Sensor range, units, ADC scaling, calibration, filtering, saturation, and invalid-value behavior |
| Analog output 1 | `0x6422:01` | `CANopenReferenceHw_WriteAnalogOutput(1, value)` | Whether required, range, DAC/PWM implementation, scaling, calibration, and safe state |

The software default is to move commanded outputs to a de-energized safe state during initialization and fault handling. The board-level safety design must be independent of the software hook and may impose a stricter state.

## CANopen product policy

| Policy | Current reference baseline | Freeze decision |
|---|---|---|
| Product profile | CiA 401 selected for v1 | **Selected** |
| Node-ID | Configured through the project commissioning policy and LSS hooks | Define allowed range, default, persistence, and production programming method |
| Default bitrate | 500 kbit/s reference configuration | Confirm default and allowed alternatives for the product network |
| Heartbeat | CANopenNode producer/consumer services are integrated | Define producer period, consumer entries, timeout reaction, and acceptance limits |
| EMCY | Standard CANopen emergency object is integrated | Define product error codes, repetition policy, clear policy, and service procedure |
| SDO server | CANopenNode SDO server is integrated | Freeze timeout, abort behavior, access policy, and service-tool expectations |
| SDO client | Optional configured client path exists in the reference | Decide whether it is part of the v1 product claim and test contract |
| PDOs | Four RPDO and four TPDO communication/mapping records exist in the OD | The authoritative defaults and mapping policy are recorded in `product/cia401_od.yaml`; product-specific non-empty mappings require a manifest revision and regenerated artifacts |
| TPDO transmission | Determined by active OD communication parameters | Freeze event, inhibit, synchronous, and timer values |
| RPDO behavior | RPDO writes reach the generated application objects | Freeze timeout, invalid-data, disable, and safe-output reactions |
| SYNC | CANopenNode SYNC service is integrated | Define producer/consumer role, period, jitter, and PDO synchronization policy |
| LSS | Stack integration and project policy hooks exist | Define whether LSS is enabled in production and how node-ID/bitrate store is authorized |
| NMT | CANopenNode NMT behavior is integrated | Freeze startup state, reset behavior, invalid-command behavior, and safe-state policy |
| Persistence | OD 1010h/1011h storage path is integrated | Define which parameters are persistent and validate power-loss behavior on hardware |

## Freeze acceptance gates

The product owner and hardware owner must approve the following before a production release can use this document as a frozen definition:

1. The exact STM32F767 ordering code, package, memory density, board revision, transceiver, oscillator, supply, termination, and connector are recorded.
2. The number, electrical levels, polarity, filtering, calibration, and fault behavior of every I/O channel are recorded.
3. The node-ID, bitrate, heartbeat, EMCY, NMT, LSS, SDO, SYNC, and persistence policies are approved.
4. The authoritative OD manifest, generated OD, EDS, firmware configuration, and acceptance tests identify the same product revision; an XDD export remains a separately tracked release gate because no XDD is currently checked in.
5. Safe-state behavior is verified at the board level, including reset, watchdog, bus-off, brownout, and loss-of-communication cases.
6. Hardware/HIL evidence is attached to the exact firmware SHA and board serial; host tests alone cannot close these gates.

## Explicit non-claims

This CiA 401 product freeze does not claim a complete CiA 402 drive, live CiA 418 battery profile, embedded UDS/ISO-TP server, embedded NMEA 2000/J1939 stack, CAN-FD support, secure boot, field-update security, or formal CANopen/CiA 401 conformance. Those are separate product decisions and validation programs.

## Traceability

The selected software baseline is recorded in [`docs/release_v0.9.0_rc1_baseline.md`](docs/release_v0.9.0_rc1_baseline.md). The current generated dictionary and reference EDS are:

- `Generated/OD.c` and `Generated/OD.h`
- `ObjectDictionary/stm32f767_canopen_reference.eds`

The authoritative manifest is `product/cia401_od.yaml`; `scripts/validate_cia401_product.py` checks it against the generated OD, EDS, and selected firmware personality. Hardware-specific values, a product-approved non-empty PDO map, and an XDD export remain explicit product-freeze gates.
