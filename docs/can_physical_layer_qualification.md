# CAN Physical-Layer Qualification

## Scope

This campaign verifies the released STM32F767 board and transceiver rather than the host CAN model. It remains **PENDING hardware/laboratory** until a calibrated analyzer and, where required, a differential probe produce evidence linked to the exact board and firmware image.

## Measurement matrix

| Area | Measurement | Required evidence |
|---|---|---|
| Bitrate | Nominal 500 kbit/s and every product-approved alternative | Analyzer decode, measured bit period, configuration record |
| Sample point | Configured and measured sample point | Bit-timing calculation and captured waveform |
| Oscillator | HSE frequency, tolerance, startup, and corner drift | Frequency measurement at approved voltage/temperature corners |
| CANH/CANL | Dominant/recessive levels and common-mode behavior | Differential waveform captures and measured limits |
| Differential voltage | Dominant and recessive differential amplitude | Oscilloscope/analyzer trace with probe calibration |
| Termination | End-to-end resistance, split termination, grounding, connector | Resistance measurement and schematic review |
| ACK and errors | ACK behavior, error frames, TEC/REC and bus-load counters | Independent-node trace and controller diagnostics |
| Load | 25%, 50%, 75%, 90%, and maximum intended utilization | Timestamped load calculation and no-loss/error result |
| Transceiver control | Standby/enable, reset, power sequencing, unplug/replug | Digital-control trace and safe-state observation |

## Procedure

Use the USB-CAN interface and independent CANopen node from the HIL rig. Capture a passive analyzer trace at startup, NMT transitions, heartbeat, SDO, PDO, SYNC, EMCY, LSS, and bus-off recovery. Measure the physical waveform independently of the protocol decoder. Repeat at the product’s approved voltage and environmental corners.

The result record must include board serial and revision, transceiver part and BOM revision, analyzer and probe identifiers, calibration dates, firmware and OD hashes, termination configuration, ambient conditions, and trace paths. A host `vcan` pass is software evidence only and cannot close this campaign.
