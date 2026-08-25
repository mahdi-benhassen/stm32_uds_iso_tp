# Build and hardware bring-up

This repository builds an independent ISO 15765-2 ISO-TP and ISO 14229 UDS stack. The host library is built with CMake and GCC or Clang. The STM32F767 target uses the generated STM32 HAL and GNU Arm Embedded GCC. No protocol stack other than the standalone UDS/ISO-TP implementation is downloaded or linked.

## Targets

| Target | Transport | Purpose |
|---|---|---|
| `library/` | Host callback abstraction | Strict host build, tests, sanitizers, coverage, and static analysis |
| Repository root | STM32F767 CAN1/bxCAN | Minimal UDS-only Classical CAN application |
| `examples/stm32f767_bxcan/` | STM32F767 bxCAN | Reusable Classical CAN adapter contract |
| `examples/stm32_fdcan/` | STM32 FDCAN | Reusable CAN-FD adapter contract with DLC and BRS metadata |

The STM32F767 does not contain an integrated CAN-FD controller. Its example is therefore Classical CAN only. CAN-FD hardware requires a selected FDCAN-capable STM32 or external CAN-FD controller, a compatible transceiver, and a board-specific generated project.

## Host library build

```sh
cmake -S library -B build-standalone -G Ninja \
  -DUDS_ISO_TP_BUILD_TESTS=ON \
  -DUDS_ISO_TP_BUILD_EXAMPLES=ON \
  -DUDS_ISO_TP_MAX_PAYLOAD=16384
cmake --build build-standalone --parallel
ctest --test-dir build-standalone --output-on-failure
```

Run the architecture check and analysis gates with:

```sh
python3 tests/architecture/check_standalone_architecture.py
clang-format --dry-run --Werror library/src/*.c library/include/uds_iso_tp/*.h \
  library/tests/isotp/*.c library/tests/uds/*.c \
  examples/stm32f767_bxcan/*.c examples/stm32f767_bxcan/*.h \
  examples/stm32_fdcan/*.c examples/stm32_fdcan/*.h
clang-tidy library/src/*.c -- -std=c11 -Ilibrary/include -Ilibrary/include/uds_iso_tp
cppcheck --enable=warning,performance,portability --error-exitcode=1 \
  --std=c11 --inline-suppr --suppress=missingIncludeSystem \
  -Ilibrary/include library/src library/tests examples
```

The sanitizer build is:

```sh
cmake -S library -B build/sanitize -G Ninja \
  -DUDS_ISO_TP_BUILD_TESTS=ON \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g'
cmake --build build/sanitize --parallel
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
  ctest --test-dir build/sanitize --output-on-failure
```

## STM32F767 cross-build

Install `arm-none-eabi-gcc`, `arm-none-eabi-objcopy`, CMake, and Ninja. Then configure from a clean directory:

```sh
cmake -S . -B build/stm32f767 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build/stm32f767 --parallel
arm-none-eabi-size build/stm32f767/stm32f767_uds_iso_tp.elf
```

The generated image is accompanied by HEX and BIN post-build outputs. The generated CubeMX descriptor is [`stm32f767_uds_iso_tp.ioc`](stm32f767_uds_iso_tp.ioc). Its default diagnostic identifiers are request `0x7E0` and response `0x7E8`; CAN1 uses the generated PI9 RX and PA12 TX assignment, which must be checked against the actual schematic.

The minimal application initializes the HAL CAN peripheral, receives frames in the HAL callback, defers protocol processing to the main loop, and uses the injected millisecond clock. It does not create an object dictionary, initialize a network-management state machine, or start a second protocol stack.

## HIL and flashing

Physical testing requires an external CAN transceiver, correct termination, common ground, a selected peer or analyzer, and board-specific power and pin validation. Use the safety-gated runner in [`tests/standalone/run_uds_iso_tp_hil.py`](tests/standalone/run_uds_iso_tp_hil.py). Dry-run mode is non-destructive and is the default validation path:

```sh
python3 tests/standalone/run_uds_iso_tp_hil.py --dry-run
python3 tests/standalone/run_uds_iso_tp_hil.py --dry-run --can-fd
```

Live tests require `python-can`, a configured SocketCAN interface, and explicit operator approval for destructive cases. Host and cross-build evidence does not prove electrical interoperability, target interrupt latency, vendor message-RAM configuration, bus-off recovery, Flash power-loss behavior, or production security.

## Provenance and dependencies

The first historical commit was an exact snapshot of the requested CubeMX baseline. The subsequent cleanup removed its unrelated application and protocol dependencies; the removal inventory is recorded in [`docs/architecture/canopen_removal_audit.md`](docs/architecture/canopen_removal_audit.md). The only retained vendor material is the STM32F7 HAL/CMSIS validation subset documented in [`THIRD_PARTY.md`](THIRD_PARTY.md).
