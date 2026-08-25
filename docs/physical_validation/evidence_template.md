# Physical HIL evidence record

Copy this template for each completed hardware campaign. A blank or `TBD` field is not a pass.

## Campaign identity

| Field | Value |
|---|---|
| Campaign ID | `TBD` |
| Operator | `TBD` |
| UTC start/end | `TBD` |
| Repository commit | `TBD` |
| Firmware image SHA-256 | `TBD` |
| Host/toolchain version | `TBD` |
| Analyzer/interface and firmware | `TBD` |
| Raw trace file and SHA-256 | `TBD` |
| Board profile | `board_profile.yaml` revision `TBD` |

## Physical configuration

| Field | Classical CAN / F767 | CAN FD / FDCAN target |
|---|---|---|
| Board and revision | `TBD` | `TBD` |
| MCU/peripheral | `STM32F767 / bxCAN` | `TBD` |
| Transceiver and mode | `TBD` | `TBD` |
| CAN pins | `TBD` | `TBD` |
| Nominal bitrate | `TBD` | `TBD` |
| Data bitrate | `N/A` | `TBD` |
| BRS | `false` | `TBD` |
| CAN IDs | request `0x7E0`, response `0x7E8` | request `0x7E0`, response `0x7E8` |
| Addressing | `11-bit normal` | `11-bit normal` unless approved otherwise |
| Termination | `TBD; two 120-ohm end terminations` | `TBD; two 120-ohm end terminations` |
| Filters/message RAM | `TBD` | `TBD` |

## Per-case result

| Case ID | Stimulus | Expected result | Measured result | Trace/time range | Verdict |
|---|---|---|---|---|---|
| TP-003 | Classical/CAN-FD Single Frame boundary | Correct PCI, DLC, payload, and response | `TBD` | `TBD` | `PASS/FAIL` |
| TP-004 | FF followed by ordered CFs | Complete reassembly with expected length | `TBD` | `TBD` | `PASS/FAIL` |
| TP-006 | CTS flow control | CF transmission starts | `TBD` | `TBD` | `PASS/FAIL` |
| TP-007 | Bounded WAIT sequence | WAIT accepted up to bound, then abort | `TBD` | `TBD` | `PASS/FAIL` |
| TP-008 | OVERFLOW flow control | Immediate transfer abort | `TBD` | `TBD` | `PASS/FAIL` |
| TP-009 | BS 0 and nonzero | Correct block transition | `TBD` | `TBD` | `PASS/FAIL` |
| TP-010 | STmin valid and reserved values | Correct spacing or rejection | `TBD` | `TBD` | `PASS/FAIL` |
| TP-012 | Malformed frame/ID/DLC/PCI | Rejected without unsafe state | `TBD` | `TBD` | `PASS/FAIL` |
| TP-013 | Timeout | Transfer abort at configured deadline | `TBD` | `TBD` | `PASS/FAIL` |
| UDS-001 | Session Control | Expected positive or negative response | `TBD` | `TBD` | `PASS/FAIL` |
| UDS-004 | ReadDataByIdentifier | Product DID value and response format | `TBD` | `TBD` | `PASS/FAIL` |
| UDS-005 | SecurityAccess test policy | Approved test seed/key behavior | `TBD` | `TBD` | `PASS/FAIL` |
| UDS-014 | End-to-end endpoint transfer | Correct transport/application composition | `TBD` | `TBD` | `PASS/FAIL` |

## Timing and safety

Record observed request-to-response latency, inter-CF spacing, P2/P2* behavior where applicable, bus load, analyzer clock source, and measurement uncertainty. The implementation’s millisecond host clock cannot prove sub-millisecond STmin precision; values encoded in the 0xF1–0xF9 range require a target timer/analyzer measurement and an explicit product decision.

Record whether any reset, download, Flash, or activation action was attempted. If yes, attach the operator approval, recovery result, sacrificial-image identifier, and post-test integrity check. A test with missing recovery evidence is not a pass.

## Verdict

**Campaign verdict:** `NOT RUN` until every required field and case is complete.

**Reviewer:** `TBD`

**Review date:** `TBD`
