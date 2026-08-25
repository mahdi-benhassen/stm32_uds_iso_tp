# Manufacturing Production Record

## Purpose

Every production board must receive a traceable record tied to its serial number, hardware revision, firmware image, calibration, configuration, and security settings. This document defines the record structure; it is not evidence that any board has passed.

## Per-board test traveler

| Step | Required result or record | Status |
|---|---|---|
| MCU identity | Exact ordering code, lot/trace code, readback identity | PENDING manufacturing |
| Power | Rails, current, reset/brownout behavior | PENDING manufacturing |
| Oscillator | HSE/clock verification | PENDING manufacturing |
| CAN | Transceiver enable, termination, bitrate, loopback and independent-node test | PENDING manufacturing |
| GPIO | Every declared DI/DO polarity and safe state | PENDING manufacturing |
| ADC | Every declared AI range, scaling, and calibration | PENDING manufacturing |
| Flash | Read/write, CRC, persistence, and reserved-region check | PENDING manufacturing |
| Watchdog | Enabled policy, reset, cause capture, safe outputs | PENDING manufacturing |
| Identity | Serial number and hardware revision | PENDING manufacturing |
| Firmware | Version, commit, image hash, OD/EDS hash | PENDING manufacturing |
| Calibration | Calibration constants, instrument IDs, date, operator | PENDING manufacturing |
| Configuration | Node-ID, bitrate, heartbeat, product configuration | PENDING manufacturing |
| Security | Debug/RDP/option-byte settings and programming record | PENDING manufacturing |

## Record format

The released manufacturing system should emit one immutable record per board with UTC timestamps, station identifier, operator, instrument identifiers and calibration dates, measured values, limits, result, deviation/waiver, and approval. A board must not ship with unresolved `PENDING`, failed, or undocumented deviation fields.

The exact GPIO, analog, supply, transceiver, and environmental limits remain product-owner inputs in [`PRODUCT_CIA401.md`](../PRODUCT_CIA401.md). This reference repository does not invent board-specific electrical limits.
