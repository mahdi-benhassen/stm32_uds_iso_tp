# UDS Timing

Timing evidence must report the minimum, maximum, p95, p99, worst observed sample, and configured budget for each measured phase. Averages alone are insufficient for the real-time claim.

The optional Cortex-M7 DWT `CYCCNT` instrumentation records CAN IRQ context maxima, TIM7 maxima, complete CANopen processing, UDS RX handoff, ISO-TP processing, UDS service dispatch, UDS TX scheduling, and total UDS mainline processing. Convert cycles using the measured `SystemCoreClock`, not a nominal value.

The required campaigns are:

| Campaign | Required observation |
|---|---|
| CANopen-only high utilization | CANopen deadlines and watchdog progress remain within the approved budget. |
| UDS-only maximum ISO-TP throughput | RX/TX queues, service latency, and timeouts remain bounded. |
| CANopen + UDS | CANopen processing is not starved by the UDS budget. |
| CANopen + UDS + EMCY | EMCY delivery and response latency remain acceptable under diagnostics. |
| CANopen + UDS + bus errors | Error callbacks, recovery, queue accounting, and safety state are deterministic. |
| CANopen + UDS + watchdog | TIM7 and mainline progress continue to satisfy watchdog policy. |

The checked-in code exposes statistics through `CANopenReferenceTiming_GetStats()`. It does not fabricate target measurements. HIL evidence must retain raw snapshots with firmware SHA, board revision, clock, bitrate, sample point, bus load, temperature, supply voltage, and test configuration.
