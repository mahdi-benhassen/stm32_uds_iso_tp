# Production Validation and Release Evidence Plan

**Status:** Required product-board and independent-test activity; not satisfied by host CI alone.

This plan converts the reference firmware's remaining release gates into repeatable evidence activities. Each activity must record the exact firmware commit, build-manifest JSON, Object Dictionary/EDS hash, board revision, transceiver part number, instrument identifiers, operator, date, ambient conditions, and pass/fail result.

> Host contract tests prove software assumptions. They do not prove electrical timing, oscillator tolerance, transceiver behavior, Flash endurance, watchdog timing, EMC, functional safety, or formal CANopen conformance.

## 1. Software release gate

Run the repository validator on a clean checkout with initialized submodules and the pinned STM32CubeF7 revision. Archive its output and all generated manifests.

```sh
./scripts/validate_reference.sh
python3 tests/test_firmware_configuration.py
python3 tests/conformance/run_core_vectors.py
make -C tests/host test-sanitize test-coverage
```

The release candidate must run the tag-triggered SocketCAN job. The workflow executes native SocketCAN tests when the runner can create `vcan0`; when the hosted kernel lacks `CONFIG_CAN_VCAN`, it runs the deterministic release regression fallback and uploads an explicit `status=unavailable` artifact. This prevents runner infrastructure failures from masking software regressions, but the unavailable status remains a release-evidence blocker: a production label still requires native SocketCAN or hardware/HIL evidence.

## 2. Physical CAN and transceiver validation

Detailed procedure: [`can_physical_layer_qualification.md`](can_physical_layer_qualification.md). The full rig and case matrix is in [`cia401_hil_validation.md`](cia401_hil_validation.md).

| Activity | Procedure | Pass evidence |
|---|---|---|
| Electrical inspection | Review CANH/CANL routing, 120-ohm termination, common-mode range, connector protection, isolation, supply rails, and EN/STB polarity against the released schematic. | Signed schematic review and measured rail values. |
| Safe startup | Capture reset, transceiver enable, drive-enable, and application-output signals from power-on through CANopen startup. | No output or power-stage enable before board safety checks and CANopen readiness. |
| Timing | Capture differential CAN traffic with a CAN analyzer or differential probe at minimum, nominal, and maximum specified oscillator/temperature conditions. | 500 kbit/s nominal rate, reviewed sample point, no unexplained error frames, and recorded oscillator tolerance. |
| Interoperability | Connect an independent CANopen master and passive analyzer. Exercise boot-up, heartbeat, SDO `0x1018`, PDOs, NMT reset, SYNC, EMCY, and LSS policy. | Decoded trace and expected state transitions. |
| Load/error | Test representative bus utilization, receive FIFO pressure, transmit retries, malformed frames, and unplug/replug behavior. | No unsafe output; diagnostic counters and recovery state match the test record. |

## 3. Bus-off recovery campaign

Detailed procedure: [`bus_off_qualification.md`](bus_off_qualification.md).

Use a controlled fault-injection fixture or analyzer capable of inducing bus errors without damaging the transceiver. Execute at least 30 trials: 10 from normal operation, 10 during active PDO traffic, and 10 during SDO traffic. Include a power-cycle between selected trials.

For each trial record the first bus-off timestamp, error classification, CANopen lifecycle state, recovery attempt count, time to reinitialize, heartbeat behavior, and final board safety state. Pass criteria are: recovery is mainline-only, retries remain within the configured limit, no duplicate CAN peripheral ownership occurs, outputs remain safe, and exhausted retries latch the documented safe-fault state.

## 4. Flash persistence and power-loss campaign

Detailed procedure: [`flash_qualification.md`](flash_qualification.md).

Validate both storage slots on the exact MCU density and linker map used by the product. Exercise valid A/B selection, newest-sequence selection, CRC failure of A, CRC failure of B, both slots invalid, interrupted erase, interrupted payload programming, interrupted commit marker programming, and repeated reboot after each interruption point.

For endurance budgeting, record the selected store-rate policy, expected configuration-change frequency, sector erase-cycle rating from the MCU documentation, and calculated service-life margin. Do not enable unrestricted automatic stores. The product must define who may issue `0x1010`/`0x1011`, the minimum store interval, commissioning behavior, and what happens when both slots are invalid.

Pass criteria are that an interrupted write never produces a partially accepted image, a valid older slot remains usable when the newest write is interrupted, CRC failures are observable, and defaults are safe when no valid slot exists.

## 5. Watchdog timing and reset campaign

Detailed procedure: [`watchdog_qualification.md`](watchdog_qualification.md).

With the IWDG personality enabled, measure the LSI frequency and IWDG timeout at minimum, nominal, and maximum voltage/temperature conditions. Record startup grace duration, timer-progress deadline, refresh margin, and observed reset latency.

Independently halt or starve: (a) the main loop, (b) TIM7 progress, (c) CAN interrupt processing, and (d) board safety processing. Confirm that only the intended failures cause reset, that the reset-cause flags are captured and cleared according to the product policy, and that the board returns to a safe state. Repeat each fault at least 10 times.

## 6. CiA 401 acceptance

Bind the reference hooks to the released board and record channel count, polarity, debounce, ADC range, scaling, calibration, sampling period, update period, worst-case latency, diagnostic reaction, and safe-state behavior. Test every channel at normal, boundary, disconnected, shorted, and over-range conditions where applicable. The reference profile adapter is not a universal CiA 401 implementation until these board-specific values and applicable conformance results are approved.

## 7. CiA 402 acceptance boundary

The current reference provides a bounded state/control adapter, not a complete drive. A product claiming CiA 402 support must separately specify and validate the enabled modes, power-stage control, position/velocity/torque limits, feedback plausibility, homing, quick-stop, fault reaction, and safety architecture. Do not enable unsupported modes such as torque, homing, CSP, CSV, or CST merely because their object names are present.

For each enabled mode, use a second CANopen node or recognized test tool and archive controlword/statusword traces, limit tests, feedback-loss behavior, fault reset, quick-stop behavior, and power-stage measurements.

## 8. CiA 302 and LSS commissioning

The bounded peer-supervision procedure is in [`cia302_peer_supervision_qualification.md`](cia302_peer_supervision_qualification.md).

Run the existing UDS/CiA 302 hardware acceptance procedure with at least one independent peer. Add node-ID/bitrate persistence, heartbeat loss, peer reboot, NMT reset, and recovery tests. A complete Network List, Configuration Manager, and LSS Fastscan commissioning claim requires a separate design, implementation, product provisioning procedure, and applicable conformance evidence.

## 9. Security and secure update

The v1 threat-model gate is in [`security_v1_release_gate.md`](security_v1_release_gate.md).

Before enabling any gateway or field update path, approve a threat model and secure-update design covering immutable trust anchor, signed image format, key custody and rotation, anti-rollback versioning, recovery image, debug lifecycle/option bytes, manufacturing provisioning, failed-update recovery, and audit logging. The reference firmware does not implement these mechanisms and must remain a development baseline until the product security owner signs the release checklist in `docs/10_product_security_release_checklist.md`.

## 10. Formal conformance and release record

Formal conformance gate: [`canopen_conformance_gate.md`](canopen_conformance_gate.md). Release sequence: [`v1_release_readiness_gate.md`](v1_release_readiness_gate.md). Manufacturing and laboratory records are defined in [`manufacturing_production_record.md`](manufacturing_production_record.md) and [`emc_environmental_qualification.md`](emc_environmental_qualification.md). Stress/resource procedure: [`stress_soak_resource_qualification.md`](stress_soak_resource_qualification.md).

Host vectors and recognized conformance testing are different evidence classes. A formal claim requires the applicable current CiA test plan, test-tool version, complete Object Dictionary/EDS release, exact firmware hash, and archived pass results. Record deviations and waivers explicitly.

A release evidence package must contain:

1. Git commit and CI JSON build manifest.
2. Compiler, linker, CANopenNode/CanOpenSTM32, and STM32CubeF7 revisions.
3. Generated OD and EDS/XDD hashes.
4. Board revision, BOM/transceiver details, schematic review, and calibration data.
5. Physical CAN traces, bus-off/recovery logs, Flash power-loss/endurance results, and watchdog timing results.
6. CiA 401/402/302/LSS acceptance results and recognized conformance reports where claimed.
7. Security approval, debug-lock record, secure-update design, and signed release approval.

A product may label this reference **software-validated** only after the repository gates pass. Labels such as **hardware-validated**, **production-ready**, **functionally safe**, or **formally CANopen-conformant** require the external evidence above.

## References

[1]: https://www.st.com/resource/en/reference_manual/rm0410-stm32f76xxx-and-stm32f77xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf "STM32F76xxx/F77xxx reference manual"

[2]: https://www.can-cia.org/can-knowledge/cia-301-series-canopen-application-layer-and-communication-profile "CiA 301 overview"

[3]: https://csrc.nist.gov/pubs/sp/800/193/final "NIST SP 800-193 Platform Firmware Resiliency Guidelines"

[4]: https://pages.nist.gov/ssdf/ "NIST Secure Software Development Framework"

[5]: https://github.com/CANopenNode/CANopenNode "Pinned CANopenNode implementation"

## Evidence record template

| Field | Value |
|---|---|
| Firmware commit |  |
| Build-manifest SHA-256 |  |
| OD/EDS hash |  |
| Board revision |  |
| Transceiver/BOM revision |  |
| Instrument IDs |  |
| Operator/date/ambient |  |
| Test case set and result |  |
| Deviations/waivers |  |
| Approvals |  |

The empty template is intentional: it prevents unexecuted hardware evidence from being represented as completed merely because a procedure exists.

---

© 2026 project contributors. Licensed under the STM32 CANopen Reference Research and Education License.
