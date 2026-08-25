# EMC and Environmental Qualification Gate

## Scope

EMC and environmental qualification require an approved laboratory, the final board/enclosure/cabling configuration, and the applicable product standards. They are not GitHub-only code checks and remain **PENDING laboratory**.

## Planned evidence matrix

| Area | Planned test | Required record |
|---|---|---|
| ESD | Contact and air discharge at declared ports and accessible surfaces | Laboratory report, setup, severity, deviations |
| EFT/burst | Supply and signal disturbances | Waveforms, limits, functional result |
| Surge | Applicable supply/CAN surge profile | Fixture, severity, recovery, damage inspection |
| Conducted immunity | Frequency sweep and coupling setup | Calibration, levels, functional result |
| Radiated immunity | Field-strength sweep | Chamber/setup, levels, functional result |
| Conducted emissions | Supply and cable emissions | Detector settings, plots, limits |
| Radiated emissions | Chamber scan | Antenna/setup, plots, limits |
| Supply variation | Nominal, brownout, over/under-voltage cases | Rails, reset cause, safe outputs |
| Temperature | Product operating and storage corners | Dwell times, drift, CAN behavior |
| Humidity | Where required by product category | Profile, condensation handling, result |

The final product owner must identify the applicable standards and severity levels. The lab record must identify the exact board revision, enclosure, harness, transceiver, firmware and OD hashes, calibration certificates, environmental conditions, and any deviations. No conformance or production claim may be inferred from a passing host or HIL campaign alone.
