# Bounded CiA 302 Peer-Supervision Qualification

## Product boundary

The v1 CiA 302 claim is limited to configured peer-heartbeat supervision and the associated CAN acceptance path. The firmware does **not** claim the CiA 302 Network List/Configuration Manager objects `0x1F80–0x1F89`, automatic network commissioning, or a complete CiA 302 master implementation.

## Two-node campaign

| Role | Requirement |
|---|---|
| Node A | STM32F767 DUT with the selected CiA 401 firmware and configured peer node-ID |
| Node B | Independent CANopen node or analyzer/master capable of controlled heartbeat and restart behavior |

The exact firmware commit, OD/EDS hashes, node-IDs, heartbeat times, board identity, transceiver, analyzer, and operator must be recorded before the run.

## Cases

| Case | Procedure | Expected external evidence |
|---|---|---|
| Peer boot-up | Power or reset Node B and capture boot-up | DUT accepts the configured peer path without unrelated-node acceptance |
| Peer heartbeat | Run both nodes in normal operation | Heartbeat frames and supervision state are visible in trace |
| Normal operation | Exercise configured application traffic | No false peer timeout or unsafe output |
| Heartbeat loss | Stop or isolate Node B | Timeout, diagnostic state, and safe reaction match product policy |
| Recovery | Restore Node B heartbeat | Recovery state and output behavior match policy |
| Peer restart | Reset Node B repeatedly | Re-entry is bounded and repeatable |
| Repeated failures | Repeat loss/recovery campaign | No stale state, duplicate ownership, or filter overflow |
| Multi-node behavior | Add unrelated CANopen nodes | Only configured peer supervision changes state |

This campaign validates the concrete peer-heartbeat fix; it does not expand the product claim into a standard CiA 302 configuration manager.
