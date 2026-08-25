#!/usr/bin/env python3
"""Deterministic source-contract tests for the STM32F767 CANopen reference.

These checks deliberately validate electrical-interface and timing assumptions that
cannot be observed from a host-only executable. They prevent a source edit from
silently changing the documented 25 MHz HSE / 216 MHz system clock, 500 kbit/s
bxCAN timing, PA11/PA12 pin assignment, or 1 ms CANopen timer cadence.
"""
from __future__ import annotations

import json
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = (ROOT / "Core" / "Src" / "main.c").read_text(encoding="utf-8")
CLOCK = MAIN  # CubeMX branch: SystemClock_Config lives in the generated main.c
PORT_FIXUP = (ROOT / "App" / "Src" / "canopen_reference_port_fixup.c").read_text(encoding="utf-8")
MSP = (ROOT / "Core" / "Src" / "stm32f7xx_hal_msp.c").read_text(encoding="utf-8")
FEATURES = (ROOT / "App" / "Inc" / "CO_driver_custom.h").read_text(encoding="utf-8")
STORAGE_HEADER = (ROOT / "App" / "Inc" / "canopen_reference_storage.h").read_text(encoding="utf-8")
STORAGE_SOURCE = (ROOT / "App" / "Src" / "canopen_reference_storage.c").read_text(encoding="utf-8")
WATCHDOG_HEADER = (ROOT / "App" / "Inc" / "canopen_reference_watchdog.h").read_text(encoding="utf-8")
WATCHDOG_SOURCE = (ROOT / "App" / "Src" / "canopen_reference_watchdog.c").read_text(encoding="utf-8")
RECOVERY_HEADER = (ROOT / "App" / "Inc" / "canopen_reference_can_recovery.h").read_text(encoding="utf-8")
RECOVERY_SOURCE = (ROOT / "App" / "Src" / "canopen_reference_can_recovery.c").read_text(encoding="utf-8")
LINKER = (ROOT / "STM32F767xx_FLASH.ld").read_text(encoding="utf-8")
PROFILE = (ROOT / "App" / "Inc" / "canopen_reference_config.h").read_text(encoding="utf-8")
BOARD = (ROOT / "App" / "Src" / "canopen_reference_board.c").read_text(encoding="utf-8")
CIA302_HEADER = (ROOT / "App" / "Inc" / "canopen_reference_cia302.h").read_text(encoding="utf-8")
CIA302_SOURCE = (ROOT / "App" / "Src" / "canopen_reference_cia302.c").read_text(encoding="utf-8")
CIA418_HEADER = (ROOT / "App" / "Inc" / "cia418_reference.h").read_text(encoding="utf-8")
CIA418_OD_HEADER = (ROOT / "Generated" / "cia418_OD.h").read_text(encoding="utf-8")
CIA418_SOURCE = (ROOT / "App" / "Src" / "cia418_reference.c").read_text(encoding="utf-8")
APP_RUNTIME = (ROOT / "App" / "Src" / "CO_app_STM32_reference.c").read_text(encoding="utf-8")
TIMING_HEADER = (ROOT / "App" / "Inc" / "canopen_reference_timing.h").read_text(encoding="utf-8")
TIMING_SOURCE = (ROOT / "App" / "Src" / "canopen_reference_timing.c").read_text(encoding="utf-8")
IRQ_SOURCE = (ROOT / "Core" / "Src" / "stm32f7xx_it.c").read_text(encoding="utf-8")
HIL_PLAN = (ROOT / "tests" / "hardware" / "cia401_hil_plan.json").read_text(encoding="utf-8")
HIL_RUNNER = (ROOT / "tests" / "hardware" / "run_cia401_hil_campaign.py").read_text(encoding="utf-8")
HIL_DOC = (ROOT / "docs" / "cia401_hil_validation.md").read_text(encoding="utf-8")
LIFECYCLE = (ROOT / "App" / "Inc" / "canopen_reference_lifecycle.h").read_text(encoding="utf-8")
FILTER_SOURCE = APP_RUNTIME
FILTER_HELPER_SOURCE = (ROOT / "middleware" / "canopen" / "core" / "can_acceptance_filter.c").read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
MANIFEST_SCRIPT = (ROOT / "scripts" / "write_build_manifest.sh").read_text(encoding="utf-8")
HAL_CONF = (ROOT / "Core" / "Inc" / "stm32f7xx_hal_conf.h").read_text(encoding="utf-8")
CAN_PORT = (ROOT / "middleware" / "canopen" / "port" / "can_port.c").read_text(encoding="utf-8")
CI = (ROOT / ".github" / "workflows" / "ci.yml").read_text(encoding="utf-8")
HOST_MAKEFILE = (ROOT / "tests" / "host" / "Makefile").read_text(encoding="utf-8")
VALIDATE_SCRIPT = (ROOT / "scripts" / "validate_reference.sh").read_text(encoding="utf-8")
PRODUCTION_PLAN = (ROOT / "docs" / "production_validation_plan.md").read_text(encoding="utf-8")
RELEASE_CANDIDATE = (ROOT / "docs" / "release_v0.9.0_candidate.md").read_text(encoding="utf-8")
RELEASE_GATE = (ROOT / "scripts" / "check_production_release_gate.sh").read_text(encoding="utf-8")
MEMORY_GATE = (ROOT / "scripts" / "check_memory_budget.sh").read_text(encoding="utf-8")
EVIDENCE_INIT = (ROOT / "scripts" / "init_external_evidence_package.sh").read_text(encoding="utf-8")
PRODUCT_SCOPE = (ROOT / "PRODUCT_SCOPE.md").read_text(encoding="utf-8")
FEATURE_MATRIX = (ROOT / "docs" / "feature_matrix.md").read_text(encoding="utf-8")
UDS_MODEL = (ROOT / "middleware" / "diagnostics" / "uds_isotp.py").read_text(encoding="utf-8")
NMEA_MODEL = (ROOT / "middleware" / "gateway" / "nmea2000_gateway.py").read_text(encoding="utf-8")
FUZZ_HARNESS = (ROOT / "tests" / "fuzz" / "fuzz_canopen_frame.c").read_text(encoding="utf-8")
FUZZ_VECTOR_DATA = json.loads((ROOT / "tests" / "conformance" / "core_vectors.json").read_text(encoding="utf-8"))


def source_integer(name: str) -> int:
    match = re.search(rf"^#define\s+{name}\s+([0-9]+)(?:U|UL|L)?$", MAIN, re.MULTILINE)
    if match is None:
        raise AssertionError(f"missing integer macro {name}")
    return int(match.group(1))


class FirmwareConfigurationTests(unittest.TestCase):
    def test_clock_tree_contract(self) -> None:
        """The documented 25 MHz HSE clock tree produces 216/54/108 MHz domains."""
        for expected in (
            "RCC_OscInitStruct.PLL.PLLM = 25;",
            "RCC_OscInitStruct.PLL.PLLN = 432;",
            "RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;",
            "RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;",
            "RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;",
            "RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;",
        ):
            self.assertIn(expected, CLOCK)
        hse_hz = 25_000_000
        sysclk_hz = hse_hz // 25 * 432 // 2
        self.assertEqual(sysclk_hz, 216_000_000)
        self.assertEqual(sysclk_hz // 4, 54_000_000)

    def test_bxcan_bit_timing_is_500_kbit(self) -> None:
        """CAN1 uses 54 MHz PCLK1 with an exact 500 kbit/s nominal rate.

        The generated init keeps the prescaler and leaves AutoBusOff disabled
        (the bounded software recovery owns bus-off); the application-layer
        port fix-up enforces the reference bit timing and mandatory automatic
        retransmission.
        """
        for expected in (
            "hcan1.Init.Prescaler = 6;",
            "hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;",
            "hcan1.Init.AutoBusOff = DISABLE;",
        ):
            self.assertIn(expected, MAIN)
        for expected in (
            "hcan->Init.AutoRetransmission = ENABLE;",
            "hcan->Init.SyncJumpWidth = CAN_SJW_1TQ;",
            "hcan->Init.TimeSeg1 = CAN_BS1_14TQ;",
            "hcan->Init.TimeSeg2 = CAN_BS2_3TQ;",
        ):
            self.assertIn(expected, PORT_FIXUP)
        pclk1_hz = 54_000_000
        time_quanta = 1 + 14 + 3
        self.assertEqual(pclk1_hz // (6 * time_quanta), 500_000)
        self.assertEqual(pclk1_hz % (6 * time_quanta), 0)
        self.assertAlmostEqual((1 + 14) / time_quanta, 15.0 / 18.0)

    def test_standalone_can_port_validates_common_bitrates(self) -> None:
        """The STM32 facade applies a bounded 54 MHz APB1 timing table."""
        for bitrate in (10000, 20000, 50000, 125000, 250000, 500000, 800000, 1000000):
            self.assertIn(f"{{{bitrate}U,", CAN_PORT)
        self.assertIn("const can_port_timing_t *timing = can_port_find_timing(bitrate);", CAN_PORT)
        self.assertIn("if (timing == NULL)", CAN_PORT)
        self.assertNotIn("(void)bitrate;", CAN_PORT)

    def test_tim7_is_exactly_one_millisecond(self) -> None:
        """TIM7 runs from the doubled APB1 timer clock when APB1 is divided by four."""
        # Generated configuration: 108 MHz / 108 / 1000 -> exactly 1 kHz.
        self.assertIn("htim7.Init.Prescaler = 108-1;", MAIN)
        self.assertIn("htim7.Init.Period = 1000-1;", MAIN)
        timer_input_hz = 108_000_000
        prescaler_div = 108
        period_ticks = 1000
        self.assertEqual(timer_input_hz // (prescaler_div * period_ticks), 1_000)
        self.assertEqual(timer_input_hz % (prescaler_div * period_ticks), 0)
        self.assertIn("canopen_app_interrupt();", MAIN)
        # The dispatch must not preempt CAN reception at (0,0).
        self.assertIn("HAL_NVIC_SetPriority(TIM7_IRQn, 1, 0);", PORT_FIXUP)

    def test_can_pins_and_interrupts_match_can1_contract(self) -> None:
        """CAN1 is wired to PI9/PA12 AF9 with IRQ-backed reception and dispatch."""
        for expected in (
            "__HAL_RCC_CAN1_CLK_ENABLE();",
            "GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;",
            "GPIO_InitStruct.Pin = GPIO_PIN_9;",
            "GPIO_InitStruct.Pin = GPIO_PIN_12;",
            "HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);",
            "HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);",
            "HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);",
            "HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);",
            "HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 0, 0);",
            "HAL_NVIC_SetPriority(TIM7_IRQn, 0, 0);",
        ):
            self.assertIn(expected, MSP)
        # The application fix-up demotes the 1 ms dispatch below CAN RX.
        self.assertIn("HAL_NVIC_SetPriority(TIM7_IRQn, 1, 0);", PORT_FIXUP)

    def test_canopennode_feature_dependencies_are_complete(self) -> None:
        """Block-transfer options include every upstream prerequisite used by this reference."""
        for expected in (
            "#define CO_CONFIG_SDO_SRV             (0x02U | 0x04U | 0x4000U)",
            "#define CO_CONFIG_SDO_SRV_BUFFER_SIZE 1024U",
            "#define CO_CONFIG_SDO_CLI             (0x01U | 0x02U | 0x04U | 0x08U | 0x4000U)",
            "#define CO_CONFIG_SDO_CLI_BUFFER_SIZE 1024U",
            "#define CO_CONFIG_FIFO                (0x01U | 0x02U | 0x04U)",
            "#define CO_CONFIG_CRC16               0x01U",
            "#define CO_CONFIG_PDO                 (0x01U | 0x02U | 0x04U | 0x08U | 0x10U | 0x20U | 0x40U | 0x4000U)",
            "#define CO_CONFIG_LSS                 (0x01U | 0x02U)",
            "#define CO_CONFIG_LEDS                0x01U",
        ):
            self.assertIn(expected, FEATURES)

    def test_node_specific_can_acceptance_filter_is_configured(self) -> None:
        """bxCAN uses exact 16-bit list filters for the configured node and LSS."""
        self.assertIn("CANopenReference_ConfigureCanFilter", FILTER_SOURCE)
        self.assertIn("CAN_FILTERMODE_IDLIST", FILTER_SOURCE)
        self.assertIn("CAN_FILTERSCALE_16BIT", FILTER_SOURCE)
        for expected in (
            "CANopenReference_FilterAdd(ids, &count, 0x000U)",
            "CANopenReference_FilterAdd(ids, &count, 0x080U)",
            "CANopenReference_FilterAdd(ids, &count, 0x7E4U)",
            "CANopenReference_FilterAdd(ids, &count, 0x7E5U)",
            "OD_PERSIST_COMM.x1280_SDOClientParameter.COB_IDServerToClientRx",
            "CANOPEN_REFERENCE_CIA302_PEER_NODE_ID",
            "for (uint32_t bank = 0U; bank < ((count + 3U) / 4U); ++bank)",
        ):
            self.assertIn(expected, FILTER_SOURCE)

    def test_can_errors_and_busoff_recovery_are_mainline_bounded(self) -> None:
        """ISR error capture maps faults and defers bounded recovery to mainline."""
        for expected in (
            "HAL_CAN_ErrorCallback",
            "HAL_CAN_ERROR_BOF",
            "HAL_CAN_ERROR_ACK",
            "HAL_CAN_ERROR_RX_FOV0",
            "HAL_CAN_ERROR_STF",
            "CO_CAN_ERRTX_BUS_OFF",
            "CANopenReferenceCanRecovery_Request",
            "CANopenReferenceCanRecovery_Complete",
            "CANOPEN_REFERENCE_CAN_RECOVERY_MAX_ATTEMPTS",
            "CANopenReference_FailRuntime(0xCA000001UL)",
        ):
            self.assertIn(expected, APP_RUNTIME + RECOVERY_SOURCE)
        self.assertNotIn("CANopenReferenceCanRecovery_Request", MAIN)

    def test_storage_reserves_real_flash_slots_and_validates_images(self) -> None:
        """The default STM32 backend cannot overlap the executable image and validates CRC/sequence metadata."""
        for expected in (
            "CANOPEN_REFERENCE_STORAGE_SLOT_A",
            "CANOPEN_REFERENCE_STORAGE_SLOT_B",
            "storage_flash_newest",
            "storage_crc32",
            "image->magic == CANOPEN_REFERENCE_STORAGE_MAGIC",
            "slot_a->sequence",
        ):
            self.assertIn(expected, STORAGE_SOURCE)
        self.assertIn("FLASH (rx)      : ORIGIN = 0x8000000, LENGTH = 1024K", LINKER)
        # Physical NVM slots sit beyond the generated region; the additive
        # linker guard makes that reservation fail-closed.
        self.assertIn("CANOPEN_REFERENCE_STORAGE_SLOT_A      0x08180000UL", STORAGE_SOURCE)
        self.assertIn("CANOPEN_REFERENCE_STORAGE_SLOT_B      0x081C0000UL", STORAGE_SOURCE)
        self.assertIn("application image must stay below sector 10 origin",
                      (ROOT / "linker" / "canopen_nvm_reservation.ld").read_text(encoding="utf-8"))

    def test_storage_size_and_write_rate_policy_are_explicit(self) -> None:
        """Flash image size and optional write-rate policy are compile-time visible."""
        self.assertIn("CANOPEN_REFERENCE_STORAGE_SLOT_SIZE", STORAGE_HEADER)
        self.assertIn("CANOPEN_REFERENCE_STORAGE_MIN_STORE_INTERVAL_MS", STORAGE_HEADER)
        self.assertIn("sizeof(canopen_reference_storage_image_t) <= CANOPEN_REFERENCE_STORAGE_SLOT_SIZE", STORAGE_SOURCE)
        self.assertIn("storage_store_rate_allowed", STORAGE_SOURCE)
        self.assertIn("CANopenReferenceStorage_StoreCount", STORAGE_SOURCE + STORAGE_HEADER)

    def test_filter_overflow_and_runtime_lifecycle_are_explicit(self) -> None:
        """Active COB-ID overflow fails closed and runtime phases are observable."""
        self.assertIn("CANOPEN_REFERENCE_CAN_FILTER_MAX_IDS", PROFILE)
        self.assertIn("if (*count >= capacity)", FILTER_HELPER_SOURCE)
        self.assertIn("return false;", FILTER_SOURCE)
        self.assertIn("CANopenAcceptanceFilter_Add(ids, CANOPEN_REFERENCE_CAN_FILTER_MAX_IDS, count, id)",
                      FILTER_SOURCE)
        for state in ("CANOPEN_REFERENCE_RUNTIME_INIT", "CANOPEN_REFERENCE_RUNTIME_RUNNING",
                      "CANOPEN_REFERENCE_RUNTIME_RESET_REQUESTED",
                      "CANOPEN_REFERENCE_RUNTIME_REINITIALIZING",
                      "CANOPEN_REFERENCE_RUNTIME_SAFE_FAULT"):
            self.assertIn(state, LIFECYCLE + APP_RUNTIME)
        self.assertIn("CANopenReference_RuntimeState", LIFECYCLE + APP_RUNTIME)

    def test_transport_deadline_and_unsupported_target_contract(self) -> None:
        """The standalone facade no longer ignores timeout or starts an unknown target."""
        self.assertIn("HAL_GetTick()", CAN_PORT)
        self.assertIn("HAL_Delay(1U)", CAN_PORT)
        self.assertIn("return -ENOTSUP;", CAN_PORT)
        self.assertIn("timeout_ms == 0U", CAN_PORT)

    def test_storage_is_enabled_and_initialized_before_canopen(self) -> None:
        """OD 1010h/1011h persistence is enabled through project-owned code."""
        self.assertIn("#define CO_CONFIG_STORAGE_ENABLE      0x01U", FEATURES)
        self.assertIn("#define CO_CONFIG_STORAGE             CO_CONFIG_STORAGE_ENABLE", FEATURES)
        self.assertIn("CO_storage_init", STORAGE_SOURCE)
        self.assertIn("CANopenReferenceStorage_BoardStore", STORAGE_HEADER)
        self.assertIn("CANopenReferenceStorage_BoardRestore", STORAGE_HEADER)
        self.assertLess(APP_RUNTIME.index("CANopenReferenceStorage_Init(CO)"),
                        APP_RUNTIME.index("CO_CANopenInit(CO"))

    def test_watchdog_is_opt_in_and_dual_rate(self) -> None:
        """Watchdog refresh requires progress from both TIM7 and mainline."""
        self.assertIn('option(CANOPEN_REFERENCE_ENABLE_IWDG "Enable dual-rate IWDG supervision" OFF)', CMAKE)
        self.assertIn("#define CANOPEN_REFERENCE_ENABLE_IWDG            0U", PROFILE)
        self.assertIn("CANopenReferenceWatchdog_TickISR", MAIN)
        self.assertIn("CANopenReferenceWatchdog_Process", MAIN)
        self.assertIn("HAL_IWDG_Refresh", WATCHDOG_SOURCE)
        self.assertIn("HAL_IWDG_MODULE_ENABLED", CMAKE)
        self.assertIn("#include \"stm32f7xx_hal_iwdg.h\"", HAL_CONF)
        self.assertIn("CANOPEN_REFERENCE_IWDG_STARTUP_GRACE_MS", PROFILE)
        self.assertIn("__HAL_RCC_CLEAR_RESET_FLAGS", WATCHDOG_SOURCE)
        self.assertIn("CANopenReferenceWatchdog_ResetFlags", WATCHDOG_SOURCE + WATCHDOG_HEADER)
        self.assertIn("HAL_GetTick() - s_start_tick", WATCHDOG_SOURCE)

    def test_build_manifest_and_vector_contracts_are_present(self) -> None:
        """Reproducibility and protocol vector artifacts are checked in and executable."""
        self.assertIn("stm32-canopen-build-manifest-v2", MANIFEST_SCRIPT)
        self.assertIn("BUILD_MANIFEST_JSON", MANIFEST_SCRIPT)
        self.assertTrue((ROOT / "tests" / "conformance" / "core_vectors.json").is_file())
        self.assertTrue((ROOT / "tests" / "conformance" / "run_core_vectors.py").is_file())
        self.assertTrue((ROOT / "docs" / "feature_matrix.md").is_file())
        self.assertGreaterEqual(len(FUZZ_VECTOR_DATA["vectors"]), 100)
        self.assertIn("len(vectors) < 100", (ROOT / "tests" / "conformance" / "run_core_vectors.py").read_text(encoding="utf-8"))

    def test_release_gate_and_local_validation_are_explicit(self) -> None:
        """Release tags require vcan and local validation cannot omit hardening gates."""
        for expected in (
            'tags:',
            '"v*"',
            "release-vcan:",
            "if: startsWith(github.ref, 'refs/tags/v')",
            "Probe and create vcan0 when supported",
            "Run deterministic release regression when vcan is unavailable",
            "make -C tests/host test-coverage-report test-sanitize-report",
        ):
            self.assertIn(expected, CI)
        for expected in ("test-sanitize", "test-coverage-report", "test-sanitize-report", "test-fuzz", "fsanitize=address,undefined", "--coverage", "COVERAGE_MIN_BRANCH"):
            self.assertIn(expected, HOST_MAKEFILE)
        for expected in ("test-results.xml", "coverage-summary.json", "sanitizer-report.txt", "validate_release_artifacts.py", "check_memory_budget.sh", "clang-tidy"):
            self.assertIn(expected, CI)
        for expected in ("test-sanitize test-coverage", "run_uds_isotp_contract.py", "run_nmea2000_gateway_contract.py"):
            self.assertIn(expected, VALIDATE_SCRIPT)
        for expected in ("Bus-off recovery campaign", "Flash persistence and power-loss campaign", "Watchdog timing and reset campaign", "Formal conformance and release record"):
            self.assertIn(expected, PRODUCTION_PLAN)

    def test_candidate_release_gate_is_fail_closed(self) -> None:
        """The candidate milestone distinguishes software evidence from external production evidence."""
        for expected in ("v0.9.0", "hardware-validation-candidate", "not a production approval", "Formal conformance", "Pending"):
            self.assertIn(expected, RELEASE_CANDIDATE)
        for expected in ("--production", "--evidence-dir", "validate_external_evidence.py", "production release gate"):
            self.assertIn(expected, RELEASE_GATE)
        for expected in ("stm32-canopen-memory-budget-v1", "MAX_TEXT_BYTES", "MAX_RAM_BYTES", "stack depth and worst-case timing"):
            self.assertIn(expected, MEMORY_GATE)
        for expected in ("LLVMFuzzerTestOneInput", "cia302_nmt_master_receive", "CANopenReferenceLss_StoreConfiguration"):
            self.assertIn(expected, FUZZ_HARNESS)

    def test_product_scope_freezes_supported_and_unsupported_claims(self) -> None:
        """Public scope distinguishes reference personalities from unsupported production features."""
        for expected in (
            "# Product Scope",
            "CiA 401 I/O device",
            "CiA 402 drive interface",
            "CiA 302 NMT-master supervision",
            "CiA 309 gateway foundation",
            "No embedded UDS server or embedded ISO-TP implementation is claimed",
            "No embedded NMEA 2000 stack or field interoperability is claimed",
            "No secure boot, key provisioning, firmware authentication",
            "v0.9.0",
            "v0.9.0-rc1",
            "509b49c",
            "v1.0.0",
        ):
            self.assertIn(expected, PRODUCT_SCOPE)
        self.assertTrue((ROOT / "PRODUCT_SCOPE.md").is_file())
        self.assertTrue((ROOT / "product" / "cia401_od.yaml").is_file())
        self.assertTrue((ROOT / "scripts" / "validate_cia401_product.py").is_file())
        cia401_product = (ROOT / "PRODUCT_CIA401.md").read_text(encoding="utf-8")
        for expected in (
            "CiA 401 Product Definition and Freeze Gate",
            "CiA 401 I/O device",
            "product/cia401_od.yaml",
            "Hardware/HIL",
            "embedded UDS/ISO-TP server",
            "XDD export remains a separately tracked release gate",
        ):
            self.assertIn(expected, cia401_product)

    def test_cia401_od_pdo_authority_is_explicit(self) -> None:
        """The selected CiA 401 product has one machine-checkable OD/PDO authority."""
        manifest = (ROOT / "product" / "cia401_od.yaml").read_text(encoding="utf-8")
        for expected in (
            '"schema": "stm32-canopen-cia401-product-v1"',
            '"personality": "cia401"',
            '"default_mapping_is_empty": true',
            '"0x6000"',
            '"0x6200"',
            '"0x1400"',
            '"0x1800"',
        ):
            self.assertIn(expected, manifest)
        validator = (ROOT / "scripts" / "validate_cia401_product.py").read_text(encoding="utf-8")
        self.assertIn("fails closed", validator)
        self.assertIn("Generated/OD.c", validator)
        self.assertIn("Generated/OD.h", validator)

    def test_cia401_hil_campaign_is_pending_and_fail_closed(self) -> None:
        """Physical validation has a complete plan but cannot be claimed from the sandbox."""
        plan = (ROOT / "tests" / "hardware" / "cia401_hil_plan.json").read_text(encoding="utf-8")
        initializer = (ROOT / "tests" / "hardware" / "run_cia401_hil_campaign.py").read_text(encoding="utf-8")
        procedure = (ROOT / "docs" / "cia401_hil_validation.md").read_text(encoding="utf-8")
        for expected in (
            '"schema": "stm32-canopen-cia401-hil-v1"',
            '"status": "hardware-execution-pending"',
            '"id": "startup"',
            '"id": "bus_off"',
            '"id": "flash"',
            '"id": "watchdog"',
        ):
            self.assertIn(expected, plan)
        for expected in ("--dry-run", '"status": "PENDING"', '"pass_claim_allowed": False', "physical execution"):
            self.assertIn(expected, initializer)
        for expected in (
            '"can_rx_to_rpdo_latency_cycles_max"',
            '"can_rx_to_application_latency_cycles_max"',
            '"rpdo_to_gpio_output_latency_cycles_max"',
            '"sync_to_tpdo_latency_cycles_max"',
            '"bus_load_campaign"',
            "for load_percent in plan[\"measurement_schema\"][\"bus_load_campaign\"][\"load_percentages\"]",
        ):
            self.assertIn(expected, initializer)
        for expected in ("USB-CAN", "Independent CANopen node", "raw CAN traffic", "250 trials", "cannot be closed by host simulation alone", "Bus-load campaign matrix", "CAN-to-application latency procedure", "25%", "95%", "PENDING"):
            self.assertIn(expected, procedure)

    def test_linker_memory_contract_fails_closed(self) -> None:
        """The linker must reject application/NVM overlap and RAM exhaustion."""
        linker = (ROOT / "STM32F767xx_FLASH.ld").read_text(encoding="utf-8")
        guard = (ROOT / "linker" / "canopen_nvm_reservation.ld").read_text(encoding="utf-8")
        # Generated region declaration for the board as configured today.
        self.assertIn("FLASH (rx)      : ORIGIN = 0x8000000, LENGTH = 1024K", LINKER)
        for expected in (
            "CANopen NVM reservation: application image must stay below sector 10 origin 0x08100000",
            "CANopen NVM reservation: application load image exceeds the declared FLASH region",
        ):
            self.assertIn(expected, guard)
        self.assertIn("canopen_nvm_reservation.ld", CMAKE)
        # The generated script keeps its own RAM heap/stack fail-closed check.
        self.assertIn("heap and stack don't fit into RAM", linker)

    def test_release_readiness_gate_preserves_lineage_and_claim_boundary(self) -> None:
        """Release tags and production claims remain tied to external evidence."""
        release = (ROOT / "docs" / "v1_release_readiness_gate.md").read_text(encoding="utf-8")
        evidence = (ROOT / "docs" / "external_evidence_package.md").read_text(encoding="utf-8")
        for expected in (
            "v0.9.0-rc1",
            "v0.9.0-rc2",
            "v1.0.0",
            "Pending hardware",
            "Pending laboratory",
            "software-validated CANopen reference integration",
            "must not claim **hardware-validated**",
        ):
            self.assertIn(expected, release)
        for expected in ("manufacturing_production_record.md", "emc_environmental_report.md", "v1_release_readiness_gate.md"):
            self.assertIn(expected, evidence)

    def test_flash_qualification_covers_power_loss_and_endurance(self) -> None:
        """Flash persistence requires exact-MCU and power-loss evidence."""
        flash = (ROOT / "docs" / "flash_qualification.md").read_text(encoding="utf-8")
        for expected in (
            "dual-slot validation",
            "0x08180000",
            "During sector erase",
            "During payload write",
            "Both slots invalid",
            "expected writes/year × expected service life",
            "PENDING hardware",
        ):
            self.assertIn(expected, flash)

    def test_bus_off_qualification_is_external_and_trial_counted(self) -> None:
        """Bus-off software tests do not replace the physical fault campaign."""
        bus_off = (ROOT / "docs" / "bus_off_qualification.md").read_text(encoding="utf-8")
        for expected in (
            "bus-off → CAN disabled → safe state → re-initialization → recovery → CAN resumes",
            "Normal operation",
            "PDO active",
            "SDO active",
            "Repeated bus-off",
            "Terminal failure",
            "250 trials",
            "real fault-injection fixture",
        ):
            self.assertIn(expected, bus_off)

    def test_cia302_peer_supervision_scope_is_bounded(self) -> None:
        """The v1 CiA 302 claim is peer supervision, not configuration management."""
        cia302 = (ROOT / "docs" / "cia302_peer_supervision_qualification.md").read_text(encoding="utf-8")
        for expected in (
            "configured peer-heartbeat supervision",
            "0x1F80–0x1F89",
            "Node A",
            "Node B",
            "Heartbeat loss",
            "Multi-node behavior",
            "does not expand the product claim",
        ):
            self.assertIn(expected, cia302)

    def test_can_physical_layer_qualification_remains_external(self) -> None:
        """Electrical CAN behavior requires calibrated board-level evidence."""
        can_phy = (ROOT / "docs" / "can_physical_layer_qualification.md").read_text(encoding="utf-8")
        for expected in (
            "PENDING hardware/laboratory",
            "500 kbit/s",
            "sample point",
            "CANH/CANL",
            "Differential voltage",
            "Termination",
            "ACK and errors",
            "Transceiver control",
            "host `vcan` pass is software evidence only",
        ):
            self.assertIn(expected, can_phy)

    def test_watchdog_qualification_remains_external_and_explicit(self) -> None:
        """Watchdog timing and reset behavior remain board-level evidence gates."""
        watchdog = (ROOT / "docs" / "watchdog_qualification.md").read_text(encoding="utf-8")
        for expected in (
            "opt-in IWDG personality",
            "CANOPEN_REFERENCE_ENABLE_IWDG=ON",
            "PENDING hardware",
            "PENDING laboratory",
            "reset-cause",
            "safe outputs",
            "Host tests prove configuration",
        ):
            self.assertIn(expected, watchdog)

    def test_external_evidence_handoff_is_pending_and_fail_closed(self) -> None:
        """Evidence templates are explicit handoff artifacts and never fabricate PASS results."""
        for expected in (
            "--output",
            "--release-commit",
            '\"status\": \"PENDING\"',
            "refusing to overwrite existing evidence directory",
            "formal_canopen_conformance.md",
            "release-socketcan-status.txt",
        ):
            self.assertIn(expected, EVIDENCE_INIT)
        self.assertIn("PENDING", (ROOT / "docs" / "external_evidence_package.md").read_text(encoding="utf-8"))
        self.assertTrue((ROOT / "scripts" / "validate_external_evidence.py").is_file())

    def test_profile_selection_and_safe_board_defaults(self) -> None:
        """The checked-in personality is CiA 401 and weak board hooks default to de-energized."""
        self.assertIn("#define CANOPEN_REFERENCE_ENABLE_CIA401 1U", PROFILE)
        self.assertIn("#define CANOPEN_REFERENCE_ENABLE_CIA402 0U", PROFILE)
        self.assertIn("CANopenReferenceBoard_SetCanTransceiverEnabled(false);", BOARD)
        self.assertIn("CANopenReferenceHw_DriveSetEnable(false);", BOARD)
        self.assertIn("CANopenReferenceHw_WriteDigitalOutputs(0U);", BOARD)

    def test_cia418_live_od_personality_is_explicit_and_isolated(self) -> None:
        """CiA 418 is opt-in, mutually exclusive, and uses one profile-specific live OD."""
        self.assertIn('option(CANOPEN_REFERENCE_ENABLE_CIA418 "Build the opt-in CiA 418 live Object Dictionary personality" OFF)', CMAKE)
        self.assertIn("CANOPEN_REFERENCE_ENABLE_CIA418=$<BOOL:${CANOPEN_REFERENCE_ENABLE_CIA418}>", CMAKE)
        self.assertIn("set(CANOPEN_REFERENCE_OD_SOURCE Generated/cia418_OD.c)", CMAKE)
        self.assertIn("set(CANOPEN_REFERENCE_OD_SOURCE Generated/OD.c)", CMAKE)
        self.assertIn("#error \"CiA 418 personality cannot be combined", PROFILE)
        self.assertIn("#include \"CO_ODinterface.h\"", CIA418_OD_HEADER)
        self.assertIn("extern OD_ATTR_APP OD_APP_t OD_APP;", CIA418_OD_HEADER)
        self.assertIn("Cia418Reference_Init(&canopenReferenceCia418State);", APP_RUNTIME)
        self.assertNotIn("Cia418Reference_SyncToGeneratedOd", APP_RUNTIME)
        self.assertNotIn("Cia418Reference_SyncToGeneratedOd", CIA418_HEADER)
        self.assertNotIn("Cia418Reference_SyncToGeneratedOd", CIA418_SOURCE)
        self.assertIn("OD_find(OD, index)", CIA418_SOURCE)
        self.assertIn("OD_getSub(entry, sub_index, &io, false)", CIA418_SOURCE)

    def test_inventus_battery_test_od_is_explicit_and_isolated(self) -> None:
        """Inventus battery entries are test-only and cannot replace the v1 CiA 401 OD."""
        self.assertIn('option(CANOPEN_REFERENCE_ENABLE_INVENTUS_BATTERY "Build the isolated Inventus battery test-only Object Dictionary personality" OFF)', CMAKE)
        self.assertIn("CANOPEN_REFERENCE_ENABLE_INVENTUS_BATTERY=$<BOOL:${CANOPEN_REFERENCE_ENABLE_INVENTUS_BATTERY}>", CMAKE)
        self.assertIn("set(CANOPEN_REFERENCE_OD_SOURCE Generated/inventus_battery_OD.c)", CMAKE)
        self.assertIn("#include \"inventus_battery_OD.h\"", (ROOT / "App" / "Inc" / "canopen_reference_od.h").read_text(encoding="utf-8"))
        self.assertIn("Inventus battery test personality must be built as an exclusive OD personality.", PROFILE)
        self.assertIn("test-only, non-commercial", (ROOT / "docs" / "inventus_battery_test_profile.md").read_text(encoding="utf-8"))
        self.assertIn("test-inventus-battery", HOST_MAKEFILE)
        self.assertIn("validate_inventus_battery.py", CI)
        self.assertIn("CANOPEN_REFERENCE_ENABLE_INVENTUS_BATTERY=ON", CI)

    def test_protocol_boundaries_are_documented_at_source_and_scope_levels(self) -> None:
        """Host-only and partial protocol implementations cannot silently become product claims."""
        for expected in (
            "0x1F80–0x1F89",
            "No embedded UDS server",
            "No embedded NMEA 2000 stack",
            "SDO/PDO reachable in that personality",
        ):
            self.assertIn(expected, FEATURE_MATRIX if "live-OD" in expected or "SDO/PDO" in expected or "0x1F80" in expected else PRODUCT_SCOPE)
        self.assertIn("standard CiA 302 Network List/Configuration Manager", CIA302_SOURCE)
        self.assertIn("host-side validation code only", UDS_MODEL)
        self.assertIn("host-side gateway contract code only", NMEA_MODEL)

    def test_audit_reconciliation_matches_current_architecture(self) -> None:
        """The audit status records current fixes and separates external gates."""
        reconciliation = (ROOT / "docs" / "audit_2026-08_reconciliation.md").read_text(encoding="utf-8")
        for expected in (
            "bare-metal HAL firmware",
            "not FreeRTOS",
            "83.33% nominal sample point",
            "Resolved in source; hardware timing still pending",
            "Implemented as opt-in; production qualification pending",
            "No source or host test proves a universal less-than-10-us ISR duration",
            "Embedded UDS/ISO-TP server",
            "CiA 304 SRDO",
            "Do not assign a production or conformance status",
        ):
            self.assertIn(expected, reconciliation)

    def test_timing_instrumentation_is_opt_in_and_covers_required_contexts(self) -> None:
        """Timing measurements are disabled by default and cover ISR/mainline contexts."""
        self.assertIn("CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION 0U", PROFILE)
        self.assertIn("CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION", CMAKE)
        self.assertIn("App/Src/canopen_reference_timing.c", CMAKE)
        for expected in (
            "CANopenReferenceTiming_Tim7Enter",
            "CANopenReferenceTiming_Tim7Exit",
            "CANopenReferenceTiming_CanEnter",
            "CANopenReferenceTiming_CanExit",
            "CANopenReferenceTiming_MainlineEnter",
            "CANopenReferenceTiming_MainlineExit",
            "CANopenReferenceTiming_GetStats",
        ):
            self.assertIn(expected, TIMING_HEADER)
            self.assertIn(expected, TIMING_SOURCE)
        for expected in (
            "CANopenReferenceTiming_Tim7Enter",
            "CANopenReferenceTiming_Tim7Exit",
        ):
            # CubeMX branch: the TIM7 hooks live in the project callback inside
            # the generated main.c USER CODE region; CAN RX0 runs inside the
            # pinned stack driver and cannot be instrumented without touching
            # generated or third-party files.
            self.assertIn(expected, MAIN)
        self.assertIn("CANopenReferenceTiming_MainlineEnter", APP_RUNTIME)
        self.assertIn("CANopenReferenceTiming_MainlineExit", APP_RUNTIME)
        self.assertIn("DWT_CTRL_CYCCNTENA_Msk", TIMING_SOURCE)
        self.assertIn("CANOPEN_REFERENCE_TIMING_ISR_WARNING_US 500U", PROFILE)
        self.assertIn("CANOPEN_REFERENCE_TIMING_ISR_WARNING_US >= CANOPEN_REFERENCE_TIMING_ISR_BUDGET_US", PROFILE)
        self.assertIn("tim7_warning_count", TIMING_HEADER)
        self.assertIn("CANOPEN_REFERENCE_TIMING_ISR_WARNING_US", TIMING_SOURCE)
        for expected in (
            "app_interrupt_cycles_max",
            "sync_cycles_max",
            "rpdo_cycles_max",
            "cia401_cycles_max",
            "cia402_cycles_max",
            "cia418_cycles_max",
            "tpdo_cycles_max",
            "CANopenReferenceTiming_PhaseEnter",
            "CANopenReferenceTiming_PhaseExit",
        ):
            self.assertIn(expected, TIMING_HEADER)
            self.assertIn(expected, TIMING_SOURCE + APP_RUNTIME)
        self.assertIn("CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.app_interrupt_cycles_max", APP_RUNTIME)
        self.assertIn("CO_LOCK_OD(CO->CANmodule);", APP_RUNTIME)
        self.assertIn("CO_UNLOCK_OD(CO->CANmodule);", APP_RUNTIME)

    def test_hil_timing_measurement_schema_is_pending_and_non_claimable(self) -> None:
        """HIL evidence has fixed timing fields but no fabricated measurement values."""
        for expected in (
            '"tim7_isr_cycles_max"',
            '"tim7_period_cycles_max"',
            '"tim7_overrun_count"',
            '"tim7_warning_count"',
            '"app_interrupt_cycles_max"',
            '"sync_cycles_max"',
            '"rpdo_cycles_max"',
            '"cia401_cycles_max"',
            '"cia402_cycles_max"',
            '"cia418_cycles_max"',
            '"tpdo_cycles_max"',
            '"can_irq_cycles_max"',
            '"can_fifo_overflow_count"',
            '"can_rx_to_rpdo_latency_cycles_max"',
            '"can_rx_to_application_latency_cycles_max"',
            '"rpdo_to_gpio_output_latency_cycles_max"',
            '"sync_to_tpdo_latency_cycles_max"',
            '"bus_load_campaign"',
            '"bus_load_percent"',
            '"temperature_c"',
            '"supply_voltage_v"',
        ):
            self.assertIn(expected, HIL_PLAN)
            self.assertIn(expected, HIL_RUNNER)
        self.assertIn('"measurement_capture"', HIL_RUNNER)
        self.assertIn('"pass_claim_allowed": False', HIL_RUNNER)
        self.assertIn("CANopenReferenceTiming_GetStats()", HIL_DOC)
        self.assertIn("CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION=ON", HIL_DOC)

    def test_cia302_master_is_explicitly_opt_in_and_mainline_ordered(self) -> None:
        """The CiA 302 master is disabled by default and receives every heartbeat before stack processing."""
        self.assertIn("option(CANOPEN_REFERENCE_ENABLE_CIA302_MASTER", CMAKE)
        self.assertIn('option(CANOPEN_REFERENCE_ENABLE_CIA302_MASTER "Build the opt-in CiA 302 NMT-master personality" OFF)', CMAKE)
        self.assertRegex(PROFILE, r"#define\s+CANOPEN_REFERENCE_ENABLE_CIA302_MASTER\s+0U")
        self.assertIn("CANopenReferenceCia302_PreProcess(now);", APP_RUNTIME)
        self.assertLess(APP_RUNTIME.index("CANopenReferenceCia302_PreProcess(now);"),
                        APP_RUNTIME.index("resetCommand = CO_process("))
        self.assertIn("CO_FLAG_READ(node->CANrxNew)", CIA302_SOURCE)
        self.assertIn("CANopenReferenceCia302_PreProcess", CIA302_HEADER)
        self.assertIn("event_count_heartbeat_timeout", CIA302_HEADER)


if __name__ == "__main__":
    unittest.main(verbosity=2)
