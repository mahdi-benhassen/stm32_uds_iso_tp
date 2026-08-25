# Watchdog Qualification and Production Decision

## Current firmware behavior

The reference firmware implements an opt-in IWDG personality. Refresh requires progress from both TIM7 and the mainline, startup grace is bounded, LSI readiness is checked, and reset-cause information is captured. The default development configuration keeps the watchdog disabled; enable it with `-DCANOPEN_REFERENCE_ENABLE_IWDG=ON` for the qualification build.

The production decision is not closed by the source contracts. The product owner should select **watchdog enabled for the production profile** or document an approved alternative safety monitor. Until that decision and board evidence exist, the repository must not label the watchdog campaign complete.

## Qualification matrix

| Case | Injection or condition | Required observations | Status |
|---|---|---|---|
| Normal operation | Nominal voltage and temperature | No unintended reset; refresh margin | PENDING hardware |
| TIM7 stopped | Halt or starve timer progress | Reset latency, reset cause, safe outputs | PENDING hardware |
| Mainline stopped | Block application progress | Reset latency, reset cause, safe outputs | PENDING hardware |
| CAN processing blocked | Prevent CAN progress while preserving other services | Intended supervision response and safe state | PENDING hardware |
| Refresh starvation | Force refresh precondition to remain false | IWDG reset and captured cause | PENDING hardware |
| Startup grace | Power-on and reset at voltage/temperature corners | Grace duration and no premature reset | PENDING hardware |
| Minimum timeout | Calibrated minimum LSI/timeout condition | Lower-bound reset latency | PENDING hardware |
| Maximum timeout | Calibrated maximum LSI/timeout condition | Upper-bound reset latency | PENDING hardware |
| Reset recovery | Reboot after each injected fault | Safe outputs, boot state, heartbeat | PENDING hardware |
| Temperature/voltage corners | Required product environmental corners | Timing margin and repeatability | PENDING laboratory |

Each injected fault must be repeated at least ten times unless the approved product safety plan specifies a larger sample. Record board serial, firmware hash, reset-cause register, measured LSI frequency, configured timeout, observed latency, voltage, temperature, and trace/debug evidence.

## Release gate

A watchdog claim requires a production-profile build, board-level reset and safe-output evidence, and review of the measured timeout margin. Host tests prove configuration and state-machine assumptions only; they cannot prove LSI frequency, watchdog timing, reset latency, or board safety behavior.
