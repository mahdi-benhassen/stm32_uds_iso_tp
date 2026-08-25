# Validation and Release Gates

Build the independent library with strict C11 diagnostics and run its CTest suite:

```sh
cmake -S library -B build-standalone -G Ninja \
  -DUDS_ISO_TP_BUILD_TESTS=ON \
  -DUDS_ISO_TP_MAX_PAYLOAD=16384
cmake --build build-standalone --parallel
ctest --test-dir build-standalone --output-on-failure
```

The release gate should additionally run the five-suite CTest set, including `uds_iso_tp_session_security_contract`, an instrumented `gcovr` coverage build, `-fsanitize=address,undefined` host tests, clang-format, clang-tidy, cppcheck, and a full build with the intended embedded toolchain. The library also provides `UDS_ISO_TP_BUILD_PORTABILITY_CHECK`, which compiles every core source with strict C99 freestanding flags, and `tests/portability/check_freestanding_arm_gcc.sh`, which performs the same check for Cortex-M0+ with an explicit target payload bound. These GCC gates reduce portability risk but do not replace an ARM Compiler/Keil build. The ISO-TP matrix covers both Classical CAN and CAN FD, CTS, bounded WAIT, immediate OVERFLOW, BS and STmin validation, reserved STmin rejection, wrong CAN ID, invalid DLC/PCI, timeout, and sequence errors. The target profile must record static RAM, stack high-water mark, Flash footprint, transfer timing, and queue behavior under the selected payload bound.

| Evidence | What it proves | What it does not prove |
|---|---|---|
| ISO-TP CTest | Deterministic frame-format and state-machine behavior in the host build | Electrical signaling, vendor HAL behavior, or target interrupt latency |
| Session/security CTest | Deterministic UDS `0x10`/`0x27` state, NRC, reset, and timing behavior | Production cryptography, vendor HAL behavior, or physical timing |
| Adapter contract build | Correct callback shape and format metadata | A specific generated STM32 FDCAN configuration |
| Strict C99 portability compile | Core sources compile freestanding with warnings treated as errors | Proprietary Keil behavior, vendor HAL integration, link placement, or board operation |
| SocketCAN dry run | CLI inventory and report generation | A connected ECU or physical bus behavior |
| Classical CAN HIL | F767 bxCAN interoperability and coexistence | CAN-FD operation |
| CAN-FD HIL | FDCAN DLC/BRS/extended-length interoperability | Production security or update safety unless separately tested |

No repository release should claim complete ISO 15765-2 or ISO 14229 conformance without a requirements matrix, interoperability tests, negative tests, timing evidence, and review of every enabled optional feature.
