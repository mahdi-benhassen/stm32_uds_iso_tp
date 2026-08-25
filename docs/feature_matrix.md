# STM32 CANopen Reference Platform — Feature Matrix

This repository is an **STM32 CANopen reference platform**. CANopenNode is the protocol stack; project-owned code supplies STM32 integration, the Object Dictionary, profile adapters, safety boundaries, persistence, diagnostics, and validation tools. The matrix deliberately separates software implementation from physical and formal evidence.

| Capability | Implementation | Host/software evidence | Physical HIL | Formal conformance |
|---|---|---|---|---|
| NMT slave | Integrated through CANopenNode | Wire contract and profile tests | Required | Not claimed |
| Heartbeat | Integrated through CANopenNode | Wire and CiA 302 supervision tests | Required | Not claimed |
| EMCY | Integrated through CANopenNode | Core wire contract and diagnostics contracts | Required | Not claimed |
| SDO expedited/segmented | Integrated through CANopenNode | SDO wire contracts | Required | Not claimed |
| PDO mapping and transmission | Integrated through CANopenNode and generated OD | PDO/mapping contracts and profile tests | Required | Not claimed |
| SYNC | Integrated through CANopenNode | State-gated wire contract | Required | Not claimed |
| LSS policy | Project policy plus CANopenNode LSS | LSS policy and wire contracts | Required | Not claimed |
| LSS Fastscan | Not implemented as a complete product commissioning feature | Limited host contract only | Required | Not claimed |
| OD 1010h/1011h persistence | Implemented with CRC-validated two-slot STM32F7 Flash backend | Source contracts and ARM build | Power-loss/endurance testing required | N/A |
| CAN bus-off recovery | Implemented with bounded mainline state machine | Recovery transition tests | Required | Not claimed |
| Watchdog supervision | Implemented as opt-in dual-rate IWDG | Configuration/source contracts and enabled ARM build | Timing/reset test required | N/A |
| CiA 401 I/O | Default reference personality and board hooks | Profile tests and source contracts | Board I/O required | Not claimed |
| CiA 402 state/control | Reference state machine, controlword, statusword, fault reset | Profile tests | Board power stage/feedback required | Not claimed |
| CiA 302 NMT-master supervision | Partial bounded configured-peer supervision reference; standard 0x1F80–0x1F89 Network List/Configuration Manager objects are absent | Host NMT-master tests | Multi-node test required | Not claimed |
| CiA 309 gateway | Optional bounded foundation | Gateway deny-by-default test | Security and transport HIL required | Not claimed |
| UDS | Host-side contract model only | UDS contract runner | Embedded implementation required | Not claimed |
| ISO-TP | Host-side contract model only | ISO-TP contract runner | Embedded implementation required | Not claimed |
| CiA 418 | Explicit opt-in dedicated CANopenNode live Object Dictionary personality using the checked-in generic reference catalog; SDO/PDO reachable in that personality | Live-OD artifact validator, host SDO-style access test, source contracts | Dedicated battery-personality HIL required | Not claimed |
| NMEA 2000 | Host gateway contract only; no embedded C implementation | Host contract runner | Embedded stack and address-claim HIL required | Not claimed |
| CAN-FD/FDCAN | Not implemented; target uses bxCAN | None | Required if selected | Not claimed |
| Bootloader/firmware update | Not implemented | None | Required | Not claimed |

## CiA 401 capability status

| Area | Status | Product work still required |
|---|---|---|
| Digital input/output application seam | Reference | Board GPIO ownership, polarity, diagnostics, and electrical limits |
| PDO mapping examples | Reference | Approve product PDO map and EDS/XDD |
| Input/output timing | Not fixed universally | Measure sampling, update, and worst-case latency on the board |
| Debounce | Board/application-specific | Define per-channel debounce and fault policy |
| Analog scaling | Board/application-specific | Define ADC range, calibration, units, and saturation behavior |
| Error behavior | Safe reference hooks | Define channel diagnostics and fault reaction |
| CiA 401 conformance | Not evidenced | Execute applicable recognized test suite |

## CiA 402 capability status

| Capability | Status |
|---|---|
| CiA 402 state machine | Implemented reference |
| Controlword | Implemented reference |
| Statusword | Implemented reference |
| Fault reset | Implemented reference |
| Profile Position | Reference adapter |
| Profile Velocity | Reference adapter |
| Profile Torque | Not implemented |
| Homing | Not implemented |
| CSP | Not implemented |
| CSV | Not implemented |
| CST | Not implemented |
| Motor feedback | Board-specific |
| Power stage | Board-specific |
| Safety reaction and limits | Product-specific |

## CiA 302 and LSS boundaries

The CiA 302 personality is a **NMT-master supervision reference**, not a complete network configuration manager. The current scope covers configured peer boot-up/heartbeat supervision and bounded NMT behavior. The standard Network List/Configuration Manager objects at 0x1F80–0x1F89 are not present in this reference, and a complete Network List, Configuration Manager, and production commissioning workflow remain future work.

LSS is integrated through the stack and project policy hooks, including node-ID/bitrate persistence seams. Complete Fastscan commissioning behavior, product provisioning, and conformance evidence remain future work.

The CiA 418 personality is initialized only when its explicit build option is selected and builds exactly one dedicated CANopenNode Object Dictionary from `scripts/cia418_catalog.py`. Its application indexes are isolated from the default CiA 401/402 OD by profile-specific source selection; the adapter reads and writes the live OD through CANopenNode’s OD interface, and its configured TPDO mappings are available in that personality. The catalog remains a generic reference catalog rather than a customer-specific Inventus Power product dictionary, and dedicated battery-personality HIL and product approval remain external gates.

UDS/ISO-TP and NMEA 2000 are host-side contract models. No embedded UDS server, ISO-TP transport, NMEA 2000 stack, or address-claim state machine is present in the STM32 firmware.

## Evidence rule

A host test or source-contract test is software evidence only. It must not be reported as physical HIL, electrical validation, or formal CANopen conformance. Release evidence must include the firmware hash, build manifest, board revision, transceiver details, captured CAN traces, test logs, and applicable recognized conformance results.
