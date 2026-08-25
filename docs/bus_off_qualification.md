# Bus-Off Recovery Qualification

## Scope

The firmware recovery path is implemented and covered by host state-transition tests, but bus-off qualification remains an external board campaign. The objective is to prove the complete sequence on the STM32F767 and transceiver:

> bus-off → CAN disabled → safe state → re-initialization → recovery → CAN resumes

No software-only test may mark this campaign complete.

## Trial matrix

| Condition | Trials | Required injection |
|---|---:|---|
| Normal operation | 30 | Controlled bus-off while the node is otherwise idle |
| PDO active | 30 | Bus-off during active TPDO/RPDO traffic |
| SDO active | 30 | Bus-off during upload/download activity |
| High bus load | 30 | Bus-off at the maximum intended bus utilization |
| Repeated bus-off | 100 | Repeated controlled faults with recovery between trials |
| Terminal failure | 30 | Sustained fault or disconnected bus according to the safety plan |

The planned campaign contains **250 trials**. The count is a plan, not completed evidence.

## Per-trial record

Record the first bus-off timestamp, controller error counters, error classification, CAN disable timestamp, safe-state timestamp, re-initialization timestamp, recovery timestamp, heartbeat behavior, recovery-attempt count, final NMT/CAN state, output state, board and firmware identity, ambient conditions, and raw CAN trace.

## Acceptance criteria

The recovery path must not create duplicate CAN peripheral ownership or unsafe output transitions. The configured retry limit must be respected. A recovered node must resume the documented CANopen lifecycle and heartbeat behavior. An exhausted retry path must latch the documented safe fault. Terminal-failure handling must be reviewed separately from recoverable bus faults.

The campaign requires a real fault-injection fixture or analyzer, independent CAN observation, and board-level output measurement. The host recovery tests remain necessary regression checks but are not a substitute for this evidence.
