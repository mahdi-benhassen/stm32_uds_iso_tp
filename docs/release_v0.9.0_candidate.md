# v0.9.0 Hardware Validation Candidate

## Purpose

`v0.9.0` is a **hardware-validation-candidate** milestone for the STM32F767 CANopen reference implementation. It packages the reproducible software baseline and its release evidence so that board-level validation can begin against one immutable firmware, Object Dictionary, linker, toolchain, dependency, and CI record.

This milestone is not a production approval, a functional-safety claim, or an official CANopen conformance result. The repository is a reference implementation, and the physical evidence described below must be produced on the exact target board, transceiver, oscillator, Flash density, and released configuration.

> A candidate tag identifies the software baseline to validate; it does not convert unexecuted hardware procedures into completed evidence.

## Candidate entry criteria

| Area | Candidate evidence | Status in repository |
|---|---|---|
| Source and dependencies | Clean commit, initialized CANopenNode submodule, pinned STM32CubeF7 revision, CMake ARM build | Automated in CI/local validation |
| Reproducibility | JSON manifest containing personality, toolchain, dependency, linker, Object Dictionary, and image hashes | Automated in the STM32 build job |
| Protocol regression | Deterministic CANopen vector corpus with category and minimum-count validation | 105 software vectors; not official conformance evidence |
| Host robustness | Contract tests, AddressSanitizer/UndefinedBehaviorSanitizer, gcov summary, and optional libFuzzer build target | Automated where toolchains are available |
| Static analysis | clang-format, cppcheck, clang-tidy, and compiler-hardening flag evaluation | Automated in static-analysis CI |
| Resource budget | GNU ld map section-size report for text, data, BSS, Flash load, and RAM | Automated in the STM32 build job |
| Release artifacts | Firmware ELF/HEX/BIN/MAP for all CI personalities, JUnit XML, coverage JSON, sanitizer report, memory report, and manifest | Validated before artifact upload |

The host release gate enforces **at least 90% line, 95% function, and 85% branch coverage** for the exercised project-owned host modules. The expanded deterministic recovery and CiA 302 boundary tests currently measure **97.97% line, 100% function, and 87.32% branch coverage**. The measured report must be retained with the candidate artifact. This remains host software evidence only; it does not replace physical timing, HIL, EMC, or conformance evidence. The STM32 CI personalities use the explicit production compiler profile and tightened map budgets described in [`docs/production_build_profile.md`](production_build_profile.md).

## Required external evidence before production labeling

The following gates are intentionally not marked complete by this document:

| Gate | Required evidence | Blocking status |
|---|---|---|
| Board and electrical review | Released schematic/BOM review, termination, protection, isolation, rails, and transceiver checks | Pending product board |
| Physical CAN timing and interoperability | Analyzer traces at specified oscillator/temperature conditions with an independent CANopen peer | Pending hardware/HIL |
| Bus-off campaign | At least 30 controlled trials, recovery timing, retry limits, heartbeat behavior, and safe-state records | Pending hardware/HIL |
| Flash persistence | Power-loss and interrupted-write campaign on the exact MCU density and linker map | Pending hardware |
| Watchdog campaign | LSI and IWDG timing across voltage/temperature plus independent fault-injection trials | Pending hardware |
| CiA 401 acceptance | Board-specific I/O timing, polarity, debounce, scaling, calibration, and fault behavior | Pending board integration |
| CiA 402 acceptance | Explicitly enabled drive modes, power-stage, feedback, limit, quick-stop, and fault-reaction evidence | Pending drive integration |
| CiA 302/LSS commissioning | Independent peer tests, persistence, heartbeat loss, reset, recovery, and any complete commissioning claims | Pending hardware/HIL |
| Security and update approval | Threat model, signed update design, key custody, anti-rollback, debug lifecycle, and recovery approval | Pending security architecture |
| Formal conformance | Applicable current CiA test plan, tool version, released OD/EDS/XDD, exact image hash, and archived result | Pending independent conformance |

The complete procedures and evidence template are in [`docs/production_validation_plan.md`](production_validation_plan.md). The empty evidence record is deliberate: a blank field is a release blocker, not an implicit pass.

## Candidate versus v1.0

`v0.9.0` is suitable for controlled hardware validation and reproducible engineering review. It may be distributed to the validation team with the complete software evidence bundle and an explicit list of external gates.

`v1.0.0` requires all product-specific gates above to be closed or formally waived by the responsible engineering owners, with archived evidence tied to the exact release commit and image hashes. A v1.0 label must not be used to imply functional safety, production readiness, cybersecurity approval, or CANopen conformance unless the corresponding independent evidence is present.

## Release command sequence

The software portion of the candidate gate is:

```sh
./scripts/validate_reference.sh
python3 scripts/run_validation_junit.py --output build/reports/test-results.xml
make -C tests/host test-coverage-report test-sanitize-report
scripts/check_memory_budget.sh build/ci-firmware/stm32f767_canopen_reference.map build/memory-budget.json
python3 scripts/validate_release_artifacts.py .
```

The production gate is intentionally separate and fail-closed:

```sh
scripts/check_production_release_gate.sh --production --evidence-dir path/to/archived-evidence
```

Do not replace the evidence directory with generated host reports. The script checks that the required external evidence records exist; it does not generate or infer them.

## Traceability

This milestone implements the software and release-engineering actions from the newest engineering review: expanded deterministic vectors, release artifact validation, explicit host coverage thresholds, sanitizer/JUnit evidence, structured fuzzing, clang-tidy enforcement, a target production compiler profile, tightened map budget checks, machine-validated external evidence semantics, and release-gate documentation. The remaining physical, security, manufacturing, EMC, HIL, and conformance activities remain external gates by design.
