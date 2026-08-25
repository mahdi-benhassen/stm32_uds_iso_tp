# CANopen Removal Audit

## Audit gate

This inventory was generated before deleting CANopen files, as required by Issue #2. The baseline was the clean standalone repository tip `270f046`. The target architecture is an independent ISO 14229 UDS layer over ISO 15765-2 ISO-TP and a generic CAN/CAN-FD callback boundary; no CANopen protocol, object dictionary, or CANopenNode source is required. Paths in this historical inventory, including the former `library/src/uds_security_provider.c` and header, describe the pre-relocation baseline only; the current deterministic reference/test implementation is under `tests/security/` and is not part of the generic library.

> This repository implements ISO 15765-2 ISO-TP and ISO 14229 UDS independently of CANopen. CANopen is neither required nor included.

## Tracked CANopen-related paths

| File/path | CANopen dependency | Used by | Replacement | Action |
|---|---|---|---|---|
| `App/Inc/CO_driver_custom.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_board.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_can_recovery.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_cia302.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_co.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_config.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_diagnostics.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_gateway.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_hw.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_lifecycle.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_lss.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_od.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_port_fixup.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_storage.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_timing.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_uds.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/canopen_reference_watchdog.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/cia401_reference.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/cia402_reference.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Inc/cia418_reference.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/CO_app_STM32_reference.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/canopen_reference_board.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/canopen_reference_can_recovery.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/canopen_reference_cia302.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/canopen_reference_diagnostics.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/canopen_reference_gateway.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/canopen_reference_hw.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/canopen_reference_lss.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/canopen_reference_port_fixup.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/canopen_reference_storage.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/canopen_reference_timing.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/canopen_reference_uds.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/canopen_reference_watchdog.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/cia401_reference.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/cia402_reference.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/cia418_reference.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `Generated/cia418_OD.c` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `Generated/cia418_OD.h` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `ObjectDictionary/stm32f767_canopen_reference.eds` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `ObjectDictionary/stm32f767_cia418_reference.eds` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `ObjectDictionary/stm32f767_inventus_battery_test.eds` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `PRODUCT_CIA401.md` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `docs/canopen_conformance_gate.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/cia401_hil_validation.md` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `linker/canopen_nvm_reservation.ld` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `middleware/canopen/README.md` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/core/can_acceptance_filter.c` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/core/can_acceptance_filter.h` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/core/canopen_core.c` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/core/canopen_core.h` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/core/canopen_reference_protocol.h` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/core/cia302_nmt_master.c` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/core/cia302_nmt_master.h` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/examples/canopen_vcan_device.py` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/od/.gitkeep` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/od/imported/IMPORT_MANIFEST.txt` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/od/imported/OD.c` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/od/imported/OD.h` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/port/can_port.c` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/port/can_port.h` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/canopen/port/vcan_port.c` | CANopen core, port, examples, or OD import | Inherited CubeMX application and legacy tests | Generic CAN callback concepts only where independently useful | Remove after audit; migrate generic pieces if any |
| `middleware/gateway/nmea2000_gateway.py` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `product/cia401_od.yaml` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `scripts/cia402_catalog.py` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `scripts/cia418_catalog.py` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `scripts/generate_cia418_od.py` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `scripts/mock_canopen_runner.py` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/validate_cia401_product.py` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `scripts/validate_cia418.py` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `stm32f767_canopen.ioc` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `tests/fakes/CANopen.h` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/fakes/CO_gateway_ascii.h` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/fakes/canopen_reference_timing_stub.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/fuzz/fuzz_canopen_frame.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/hardware/cia401_hil_plan.json` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `tests/hardware/run_cia401_hil_campaign.py` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `tests/integration/test_uds_canopen_coexistence.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/run_nmea2000_gateway_contract.py` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/test_canopen_wire_contract.py` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/test_cia418_reference.c` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `tests/test_nmea2000_gateway.py` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `third_party/CanOpenSTM32` | CANopenNode/CanOpenSTM32 git submodule | CMake, CubeMX firmware, CI, and legacy application | None; standalone library owns UDS/ISO-TP | Remove .gitmodules entry and submodule |

## Reference-only files

| File/path | CANopen dependency | Used by | Replacement | Action |
|---|---|---|---|---|
| `.github/pull_request_template.md` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `.github/workflows/ci.yml` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `.gitmodules` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `App/Inc/inventus_battery_data.h` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `App/Src/inventus_battery_data.c` | CANopen application, lifecycle, OD, gateway, or UDS wrapper coupling | Inherited CubeMX firmware target | Standalone endpoint, generic platform callbacks, or none | Remove CANopen files; migrate only generic platform code |
| `BUILD.md` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `CHANGELOG.md` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `CMakeLists.txt` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `CONTRIBUTING.md` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Core/Src/main.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Drivers/CMSIS/Device/ST/STM32F7xx/Include/stm32f767xx.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Drivers/CMSIS/Include/core_armv8mml.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Drivers/CMSIS/Include/core_cm3.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Drivers/CMSIS/Include/core_cm33.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Drivers/CMSIS/Include/core_cm4.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Drivers/CMSIS/Include/core_cm7.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Drivers/CMSIS/Include/core_sc300.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Drivers/STM32F7xx_HAL_Driver/Inc/Legacy/stm32_hal_legacy.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_hal_gpio_ex.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_hal_rcc.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_hal_rcc_ex.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_hal_tim.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_ll_system.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `FINAL_REPORT.md` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `Generated/OD.c` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `Generated/OD.h` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `Generated/inventus_battery_OD.c` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `Generated/inventus_battery_OD.h` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `LICENSE` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `PORTING.md` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `PRODUCT_SCOPE.md` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `README.md` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `SECURITY.md` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `THIRD_PARTY.md` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `build-flow-audit/CMakeFiles/3.28.3/CompilerIdC/CMakeCCompilerId.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `build-flow-coverage/CMakeFiles/3.28.3/CompilerIdC/CMakeCCompilerId.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `build-flow-final/CMakeFiles/3.28.3/CompilerIdC/CMakeCCompilerId.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `build-flow-sanitize/CMakeFiles/3.28.3/CompilerIdC/CMakeCCompilerId.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `build-legacy-compat/.ninja_deps` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `build-legacy-compat/.ninja_log` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `build-legacy-compat/CMakeFiles/3.28.3/CompilerIdC/CMakeCCompilerId.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `build-legacy-compat/build.ninja` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `build-legacy-compat/test_uds_canopen_coexistence` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `build-legacy-compat/test_uds_runtime` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `docs/11_uds_iso_tp.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/13_uds_download_and_recovery.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/audit_2026-08_reconciliation.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/bus_off_qualification.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/can_physical_layer_qualification.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/cia302_peer_supervision_qualification.md` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `docs/external_evidence_package.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/feature_matrix.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/inventus_battery_test_profile.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/issue16_branch_analysis.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/production_validation_plan.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/release_v0.9.0_candidate.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/security_v1_release_gate.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/standalone/architecture.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/standalone/hil.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/standalone/release_audit.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/standalone/stm32_examples.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/stress_soak_resource_qualification.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/uds/ISSUE16_PRODUCTION_AUDIT.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/uds/README.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/uds/architecture.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/uds/can_ids.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/uds/configuration.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/uds/cubemx_integration.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/uds/flash_programming.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/uds/hil_testing.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/uds/stm32f767.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/uds/timing.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/uds/troubleshooting.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `docs/v1_release_readiness_gate.md` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `library/README.md` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/compat/legacy_diagnostics/isotp/isotp.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/compat/legacy_diagnostics/isotp/isotp.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/compat/legacy_diagnostics/uds/uds.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/compat/legacy_diagnostics/uds/uds.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/compat/legacy_diagnostics/uds/uds_did.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/compat/legacy_diagnostics/uds/uds_did.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/compat/legacy_diagnostics/uds/uds_download.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/compat/legacy_diagnostics/uds/uds_download.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/compat/legacy_diagnostics/uds/uds_security_provider.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/compat/legacy_diagnostics/uds/uds_security_provider.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/compat/legacy_diagnostics/uds_stm32/uds_stm32.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/compat/legacy_diagnostics/uds_stm32/uds_stm32.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/include/uds_iso_tp/uds.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/include/uds_iso_tp/uds_did.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/include/uds_iso_tp/uds_download.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/include/uds_iso_tp/uds_security_provider.h` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/src/uds.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/src/uds_did.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/src/uds_download.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/src/uds_security_provider.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `library/tests/uds/test_uds.c` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |
| `scripts/check_compiler_hardening.sh` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/check_memory_budget.sh` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/check_production_release_gate.sh` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/generate_inventus_battery_od.py` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/generate_reference_od.py` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/init_external_evidence_package.sh` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/inventus_battery_catalog.py` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/run_validation_junit.py` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/setup_vcan.sh` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/validate_external_evidence.py` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/validate_inventus_battery.py` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/validate_od.py` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/validate_reference.sh` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/validate_repository.py` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `scripts/write_build_manifest.sh` | CANopen-specific documentation, tooling, gateway, or memory layout | CANopen project workflows | Standalone docs/tools or none | Remove or rewrite for standalone scope |
| `tests/conformance/core_vectors.json` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/conformance/run_core_vectors.py` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/fakes/OD.h` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/fakes/main.h` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/fakes/stm32f7xx_hal.h` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/hardware/README.md` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/hardware/__init__.py` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/hardware/run_uds_cia302_acceptance.py` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `tests/hardware/run_uds_stm32f767_acceptance.py` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/host/Makefile` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/host/test_can_port.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/host/test_sdo.py` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/isotp/test_isotp_contract.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/test_can_acceptance_filter.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/test_can_port_stm32.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/test_can_recovery.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/test_cia302_nmt_master.c` | CANopen OD/profile/product artifact | CANopen firmware profiles and validation | None in standalone diagnostic repository | Remove |
| `tests/test_firmware_configuration.py` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/test_gateway_default_deny.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/test_inventus_battery_od.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/test_lss_policy.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/test_profiles.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/test_protocol_contract.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/uds/CMakeLists.txt` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/uds/test_uds_adversarial.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/uds/test_uds_core.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/uds/test_uds_did.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/uds/test_uds_download.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/uds/test_uds_runtime.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/uds/test_uds_security_provider.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tests/uds/test_uds_stm32.c` | CANopen test fixture, coexistence, or path dependency | Legacy host/integration/CI tests | Standalone ISO-TP/UDS tests and adapter tests | Migrate relevant generic tests; remove CANopen-only tests |
| `tools/import_objdict.sh` | CANopen reference in build or project metadata | Inherited build and automation | Standalone library CMake and STM32 adapter targets | Rewrite/remove reference |

## Standalone paths reviewed

| File/path | Finding | Action |
|---|---|---|
| `library/include/uds_iso_tp/isotp.h` and `library/src/isotp.c` | Protocol-neutral ISO-TP types and state machine; no CANopen headers | Retain as authoritative |
| `library/include/uds_iso_tp/uds.h` and `library/src/uds.c` | Callback-based UDS API with fixed-width lengths and no CANopen types | Retain as authoritative |
| `examples/stm32f767_bxcan/` | Classical CAN adapter contract only | Retain and document bxCAN limitation |
| `examples/stm32_fdcan/` | CAN-FD adapter contract carrying DLC and BRS | Retain and document board-specific HIL requirement |
| `library/compat/legacy_diagnostics/` | Temporary inherited ABI retained for compatibility only; not part of standalone target or CI | Retire after application wrapper migration |

## Removal sequence

1. Remove the `third_party/CanOpenSTM32` submodule and its `.gitmodules` entry.
2. Remove CANopen application, OD/profile, gateway, linker-reservation, example, and CANopen-only test files.
3. Rewrite the inherited CubeMX build metadata or remove the copied firmware target if it cannot be converted without retaining CANopen.
4. Preserve only generic platform/timing/watchdog/CAN adapter behavior in standalone form.
5. Add architecture tests that fail when CANopen paths, headers, submodule metadata, or forbidden symbols remain.
6. Perform a clean checkout build and review all remaining occurrences as justified residual documentation or compatibility history.

## Expected residuals after cleanup

The final repository may retain historical references in the audit itself and in the provenance statement explaining that the first commit was copied from the CubeMX baseline. Such references are documentation, not build or link dependencies. No CANopen source, header, submodule, object dictionary, or runtime symbol may remain in the final standalone build graph.
