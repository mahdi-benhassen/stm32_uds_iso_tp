# Validation and Release Gates

Build the independent library with strict C11 diagnostics and run its CTest suite:

```sh
cmake -S library -B build-standalone -G Ninja \
  -DUDS_ISO_TP_BUILD_TESTS=ON \
  -DUDS_ISO_TP_MAX_PAYLOAD=16384
cmake --build build-standalone --parallel
ctest --test-dir build-standalone --output-on-failure
```

The release gate should additionally run `-fsanitize=address,undefined` host tests, clang-format, clang-tidy, cppcheck, and a full build with the intended embedded toolchain. The target profile must record static RAM, stack high-water mark, Flash footprint, transfer timing, and queue behavior under the selected payload bound.

| Evidence | What it proves | What it does not prove |
|---|---|---|
| ISO-TP CTest | Deterministic frame-format and state-machine behavior in the host build | Electrical signaling, vendor HAL behavior, or target interrupt latency |
| Adapter contract build | Correct callback shape and format metadata | A specific generated STM32 FDCAN configuration |
| SocketCAN dry run | CLI inventory and report generation | A connected ECU or physical bus behavior |
| Classical CAN HIL | F767 bxCAN interoperability and coexistence | CAN-FD operation |
| CAN-FD HIL | FDCAN DLC/BRS/extended-length interoperability | Production security or update safety unless separately tested |

No repository release should claim complete ISO 15765-2 or ISO 14229 conformance without a requirements matrix, interoperability tests, negative tests, timing evidence, and review of every enabled optional feature.
