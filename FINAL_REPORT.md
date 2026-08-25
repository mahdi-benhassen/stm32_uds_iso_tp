# STM32 UDS / ISO-TP Standalone Repository — Completion Report

## Outcome

The standalone repository is now published at [`mahdi-benhassen/stm32_uds_iso_tp`][1]. Its first commit, `01a7bd6`, is the exact snapshot of the `stm32f767_canopen_cubemx` baseline requested by the project owner. The independent implementation was then layered on top in commit `57480be`, which is now the remote `main` tip.

The original `stm32_canopen_reference` implementation was not reverted or deleted. Its Issue #16 work remains frozen and tagged in the original repository, and Issue #16 remains open because physical production evidence is incomplete.

## Published contents

| Area | Delivered scope | Evidence |
|---|---|---|
| Independent transport | Protocol-neutral C11 ISO-TP core with fixed buffers, injected frame callbacks, Classical CAN and CAN-FD profiles | `library/include/uds_iso_tp/isotp.h`, `library/src/isotp.c` |
| CAN-FD behavior | Valid 8/12/16/20/24/32/48/64-byte lengths, 64-byte Single-Frame support, SF escape format, BRS metadata, and extended First-Frame lengths above 4,095 bytes | 5,000-byte extended-FF host test and CAN-FD endpoint test |
| UDS composition | Callback-based UDS server and ISO-TP endpoint without a CANopenNode dependency or heap allocation | `library/src/endpoint.c`, `library/src/uds.c` |
| STM32F767 | bxCAN adapter contract that is explicitly Classical CAN only | `examples/stm32f767_bxcan/` |
| FDCAN-capable STM32 | FDCAN adapter contract forwarding actual data length and BRS metadata | `examples/stm32_fdcan/` |
| Testing | Four host CTest contracts, including both adapter contracts, plus AddressSanitizer and UndefinedBehaviorSanitizer builds | `library/tests/` |
| Automation | GitHub Actions workflow for build, tests, sanitizers, formatting, clang-tidy, cppcheck, and HIL dry-run reports | `.github/workflows/standalone-uds.yml` |
| HIL readiness | Non-destructive Classical CAN and CAN-FD inventory runner producing JSON, CSV, and Markdown reports | `tests/standalone/run_uds_iso_tp_hil.py` |
| Documentation | Architecture, ISO-TP profile, STM32 examples, HIL checklist, validation gates, and release audit | `docs/standalone/` |

## Validation result

The local strict build completed successfully with all **four of four CTest contracts passing**. The sanitizer build also completed with all four tests passing. clang-format, clang-tidy, cppcheck, Python bytecode compilation, and both Classical CAN and CAN-FD HIL dry-run profiles completed successfully.

The hosted workflow [`Standalone UDS ISO-TP validation`][2] for commit `57480be` completed successfully. This proves reproducible host-side and adapter-contract validation in GitHub Actions; it does not prove electrical signaling, vendor HAL configuration, target interrupt latency, or physical interoperability.

## Deliberate boundaries

The F767 example remains Classical CAN because the target project uses bxCAN. Native CAN-FD requires a distinct FDCAN-capable STM32 target or an external CAN-FD controller. The FDCAN directory is therefore an honest adapter contract rather than a fabricated vendor-generated board project. A physical CAN-FD campaign still needs a selected board, transceiver, nominal/data bit rates, message-RAM configuration, filters, wiring, peer ECU or analyzer, and captured evidence.

The transport supports payloads above 4,095 bytes, including the tested 5,000-byte extended First-Frame case. The UDS dispatcher is intentionally bounded separately: its callback API uses `uint16_t` lengths, its default request and response limits are 4,095 bytes, and compile-time assertions reject configurations above `UINT16_MAX`. The repository does not claim an unbounded end-to-end UDS payload path.

The repository is an engineering baseline, not a completed production bootloader or a full ISO 15765-2 / ISO 14229 conformance certification. SecurityAccess is policy-injected; the deterministic provider is test-only. Flash activation, authenticated image validation, rollback, anti-rollback, reset handoff, production key management, EMC qualification, and board-specific HIL remain product-owned work.

## Optional integration path

The recommended next step is to integrate the standalone library into `stm32_canopen_reference` as an optional, pinned dependency. The original in-tree diagnostics should remain as a compatibility path initially, with a new CMake option such as `CANOPEN_REFERENCE_USE_STANDALONE_UDS`. Both paths should build and pass their existing tests before the duplicated implementation is retired. This integration has not been applied to the frozen original repository.

## Credential hygiene

The GitHub credentials pasted during this task should be revoked and replaced because they were exposed in conversation. No credential was committed to the repository or stored in project files.

## References

[1]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp "Standalone STM32 UDS / ISO-TP repository"
[2]: https://github.com/mahdi-benhassen/stm32_uds_iso_tp/actions/runs/32798716258 "Hosted standalone validation workflow run"
[3]: https://www.iso.org/standard/66574.html "ISO 15765-2:2016 — Road vehicles — Diagnostic communication over CAN — Part 2"
[4]: https://www.iso.org/standard/43464.html "ISO 14229-1 — Road vehicles — Unified diagnostic services"
