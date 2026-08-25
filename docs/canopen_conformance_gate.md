# CANopen Conformance and Release Gate

## Claim boundary

Integrating CANopenNode and passing repository vectors does not by itself establish formal CANopen conformance. A formal claim requires the applicable current CiA test plan, an independent test environment or recognized test tool, the complete released OD/EDS/XDD, exact firmware hash, and archived official results.

## Applicable test sets

| Area | Applicability | Required evidence |
|---|---|---|
| CiA 301 | Core communication profile used by the product | Independent test results, traces, deviations |
| CiA 401 | Selected I/O personality | Object/access/type/range/PDO tests and board results |
| LSS/CiA 305 | If commissioning is claimed | Node-ID, bitrate, store/inquire, timeout, invalid-sequence results |
| CiA 302 | Only configured peer supervision in v1 | Peer-heartbeat evidence; no claim for absent `0x1F80–0x1F89` manager objects |
| CiA 402 | Not part of the frozen CiA 401 v1 production claim | Separate product branch and applicable drive tests if later enabled |

## Closeout procedure

Run the official or recognized applicable test set against the release candidate. Archive the tool version, test plan revision, configuration, OD/EDS/XDD hashes, firmware image hash, board identity, raw traces, result report, deviations, waivers, corrective commits, and regression rerun. A failed or unexecuted test remains open; it may not be converted into a pass by host coverage or documentation.

The repository may describe the current baseline as **software-validated reference integration** after its local gates pass. Labels such as **hardware-validated**, **production-ready**, **functionally safe**, or **formally CANopen-conformant** require the external evidence package and signed release approval.
