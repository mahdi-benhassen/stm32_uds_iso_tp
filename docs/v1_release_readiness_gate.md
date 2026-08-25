# v1.0 Release Readiness Gate

## Release sequence

The current immutable lineage is:

```text
v0.9.0       -> historical candidate
v0.9.0-rc1   -> reviewed software baseline at 509b49c
v0.9.0-rc2   -> only after physical/HIL remediation evidence
v1.0.0       -> only after all applicable external gates and approvals
```

Existing tags must not be moved silently. Every candidate must record its exact source SHA, build manifest, firmware image hash, OD/EDS/XDD hashes, and evidence-package revision.

## Gate order

| Order | Gate | Current state |
|---:|---|---|
| 1 | Software validation and reproducible build | Repository-controlled; rerun for each candidate |
| 2 | CiA 401 product and OD/PDO freeze | Documented; owner sign-off and board values remain required |
| 3 | HIL and physical CAN campaign | Pending hardware |
| 4 | CiA 302 peer supervision and bus-off | Pending hardware |
| 5 | Flash, watchdog, stress, soak, and resource characterization | Pending hardware/laboratory |
| 6 | Security and manufacturing approval | Pending owners and production process |
| 7 | EMC/environmental laboratory qualification | Pending laboratory |
| 8 | Applicable CANopen conformance | Pending independent test evidence |
| 9 | Release approval and final tag | Blocked until applicable gates close |

## Evidence integrity

The controlled package initialized by `scripts/init_external_evidence_package.sh` must contain machine-readable `status: PASS`, exact `release_commit`, `evidence_id`, and named reviewer fields for every applicable record. Templates, host-only reports, missing board identity, missing raw traces, or unresolved `PENDING` fields cannot satisfy the production gate.

## Claim policy

Until the external package is complete and approved, the repository may claim a **software-validated CANopen reference integration**. It must not claim **hardware-validated**, **production-ready**, **functionally safe**, or **formally CANopen-conformant**. CiA 418, embedded UDS/ISO-TP, NMEA 2000, complete CiA 302 configuration management, and secure field update remain outside the frozen CiA 401 v1 claim.
