# Contributing

Thank you for improving the independent STM32 UDS/ISO-TP stack. Contributions should preserve bounded execution, explicit ownership boundaries, safe defaults, and reproducible host and target builds.

## Before making a change

Read the [README](README.md), [BUILD.md](BUILD.md), [PRODUCT_SCOPE.md](PRODUCT_SCOPE.md), [THIRD_PARTY.md](THIRD_PARTY.md), and the relevant standalone documentation. Confirm whether the change affects the ISO-TP wire format, UDS service behavior, configured payload bounds, generated target code, adapter metadata, safety defaults, or a hardware evidence requirement.

Do not commit secrets, private keys, vendor credentials, proprietary board files, generated build output, or an unmanaged vendor SDK. Keep protocol behavior in `library/`, target bindings in `examples/` or the application adapter, and platform-specific policy in application-owned files.

## Development workflow

Create a topic branch from `main`, make one logically complete change at a time, add deterministic tests for pure logic and source contracts, run the host and relevant target validation, and update documentation when public behavior changes. A change must not introduce a hidden protocol dependency or a new blocking path in an interrupt handler.

## Coding conventions

Use C11-compatible code for project-owned C. Prefer fixed-width integer types, explicit bounds, checked return values, and static storage where practical. Keep CAN and timer interrupt work bounded and non-blocking. Do not allocate memory, write Flash, format strings, or perform blocking I/O from an ISR.

Use the repository `.clang-format` configuration for C formatting. Public interfaces should state ownership, timing context, valid ranges, and failure behavior. Python code should use the standard library where possible, explicit argument validation, and deterministic output suitable for CI.

## Validation commands

Run these checks from the repository root:

```sh
python3 tests/architecture/check_standalone_architecture.py
cmake -S library -B build/standalone -G Ninja \
  -DUDS_ISO_TP_BUILD_TESTS=ON -DUDS_ISO_TP_BUILD_EXAMPLES=ON
cmake --build build/standalone --parallel
ctest --test-dir build/standalone --output-on-failure
cmake -S . -B build/stm32f767 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build/stm32f767 --parallel
python3 tests/standalone/run_uds_iso_tp_hil.py --dry-run
python3 tests/standalone/run_uds_iso_tp_hil.py --dry-run --can-fd
```

The hosted workflow additionally runs coverage instrumentation, AddressSanitizer, UndefinedBehaviorSanitizer, clang-format, clang-tidy, cppcheck, and the STM32F767 cross-build. Physical HIL must identify the exact board, transceiver, timing, wiring, peer equipment, and captured evidence.

## Commit and pull-request format

Use imperative scoped subjects such as `fix(isotp): reject reserved STmin`, `test(uds): cover response bounds`, `ci: run architecture check`, or `docs: clarify FDCAN adapter limits`. A pull request should explain the behavior change, list affected profiles and configuration values, identify validation performed, and state hardware tests that could not be run.

## Review and merge requirements

A change is ready for merge when deterministic tests pass, the affected target build succeeds, `git diff --check` is clean, documentation is updated, and reviewers understand any hardware or conformance limitations. Changes involving Flash, reset, security, or board safety require explicit product-owner review before release use.
