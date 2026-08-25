# Stress, Soak, and Resource Qualification

## Stress and soak matrix

Run combined TPDO/RPDO, SDO, SYNC, heartbeat, and EMCY traffic at 25%, 50%, 75%, 90%, and the maximum intended bus load. The duration gates are 1 hour, 8 hours, 24 hours, 72 hours, and preferably 168 hours for final qualification. These are planned external tests, not completed results.

| Load/duration | Required observations |
|---|---|
| 25–50% | CAN errors, latency, CPU, RAM, stack, queue occupancy |
| 75–90% | Same measurements plus dropped/late PDO and retry behavior |
| Maximum intended load | Safe behavior, error counters, recovery, watchdog, reset count |
| 1–8 hours | Thermal and drift trend |
| 24–72 hours | Long-duration stability and resource leak detection |
| 168 hours | Final soak candidate with signed review |

Record RAM high-water mark, stack high-water mark, queue occupancy, CPU utilization, ISR duration, CAN errors, resets, watchdog events, temperature, supply, and trace references.

## Resource characterization

Establish measured maximum Flash use, RAM use, stack use, CPU utilization, ISR duration, CAN latency, PDO latency, and SYNC jitter on the exact production board. Define engineering margins before release; a 95% Flash, RAM, or CPU baseline is not an acceptable production margin without an approved risk decision.

The existing ARM size and coverage gates remain necessary software checks. They do not measure silicon timing, thermal behavior, interrupt latency, or long-duration stability. Results must be linked to the board serial, firmware hash, tool versions, instrumentation, and environmental conditions.
