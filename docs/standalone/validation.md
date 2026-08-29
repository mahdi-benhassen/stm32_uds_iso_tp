# Validation and Release Gates

Build the independent library with strict C11 diagnostics and run its CTest suite:

```sh
cmake -S library -B build-standalone -G Ninja \
  -DUDS_ISO_TP_BUILD_TESTS=ON \
  -DUDS_ISO_TP_MAX_PAYLOAD=16384
cmake --build build-standalone --parallel
ctest --test-dir build-standalone --output-on-failure
```

The release gate should run the five-test core CTest set when adapter examples are disabled, including `uds_iso_tp_session_security_contract` and `uds_iso_tp_security_reference_utility`; enabling adapter examples adds the adapter contract test and the C092 transport, diagnostics, and immediate-reset contracts. The GitHub Actions workflow also runs `uds_iso_tp_c092_immediate_reset_rx_contract` and `uds_iso_tp_reset_recovery_contract` in a dedicated visible step. The immediate-reset contract models a request arriving at the smallest practical software interval after FDCAN start, before the higher-level READY mark, and verifies `10 01 → 50 01` plus explicit pre-initialization rejection instrumentation. The gate also includes an instrumented `gcovr` coverage build, `-fsanitize=address,undefined` host tests, clang-format, clang-tidy, cppcheck, and a full build with the intended embedded toolchain. The reference utility test checks the repository-generated Level 1 vector and the session/security contract checks both Level 1 and Level 5 callback flows, while neither test claims production cryptographic strength. The library also provides `UDS_ISO_TP_BUILD_PORTABILITY_CHECK`, which compiles every generic core source with strict C99 freestanding flags, and `tests/portability/check_freestanding_arm_gcc.sh`, which performs the same check for Cortex-M0+ with an explicit target payload bound; the test-only reference layer is intentionally not part of the generic portability object. These GCC gates reduce portability risk but do not replace an ARM Compiler/Keil build. The ISO-TP matrix covers both Classical CAN and CAN FD, configurable SF/FF/FC/CF padding, independent RX/TX contexts, same-N_AI full-duplex restart, bounded control/response queuing, CTS, bounded WAIT, immediate OVERFLOW, BS and STmin validation, reserved STmin rejection, wrong CAN ID, invalid DLC/PCI, timeout, and sequence errors. The UDS matrix covers explicit Level 1–5 SecurityAccess subfunction mapping, session/security/address metadata, functional-ID dispatch, response-before-reset, asynchronous TX completion, invalid reset type, and suppressed ECUReset response. The target profile must record static RAM, stack high-water mark, Flash footprint, transfer timing, and queue behavior under the selected payload bound.

| Evidence | What it proves | What it does not prove |
|---|---|---|
| ISO-TP and endpoint CTest | Deterministic padded frame serialization, independent RX/TX state, full-duplex composition, FC behavior, and deferred reset completion in the host build | Electrical signaling, vendor HAL behavior, or target interrupt latency |
| Session/security CTest and reference utility | Deterministic UDS `0x10`/`0x27` state, NRC, service metadata, addressing, callback integration, and repository-generated key vectors | Production cryptography, vendor HAL behavior, or physical timing |
| Adapter contract build | Correct callback shape and format metadata | A specific generated STM32 FDCAN configuration |
| Strict C99 portability compile | Core sources compile freestanding with warnings treated as errors | Proprietary Keil behavior, vendor HAL integration, link placement, or board operation |
| SocketCAN dry run | CLI inventory and report generation | A connected ECU or physical bus behavior |
| Classical CAN HIL | F767 bxCAN interoperability and coexistence | CAN-FD operation |
| CAN-FD HIL | FDCAN DLC/BRS/extended-length interoperability | Production security or update safety unless separately tested |

No repository release should claim complete ISO 15765-2 or ISO 14229 conformance without a requirements matrix, interoperability tests, negative tests, timing evidence, and review of every enabled optional feature.
