# SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
ROOT := $(abspath ../..)
BUILD_DIR := $(ROOT)/build/host
SANITIZER_BUILD_DIR := $(ROOT)/build/host-sanitize
COVERAGE_BUILD_DIR := $(ROOT)/build/host-coverage
CC ?= cc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
SANITIZER_FLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g
COVERAGE_FLAGS := --coverage -O0 -g
# Release coverage floor; override only for exploratory local work.
COVERAGE_MIN_LINE ?= 90
COVERAGE_MIN_FUNCTION ?= 95
COVERAGE_MIN_BRANCH ?= 85
FUZZER ?= $(shell command -v clang 2>/dev/null || command -v clang-18 2>/dev/null || echo clang)
FUZZ_BUILD_DIR := $(ROOT)/build/host-fuzz
FUZZER_FLAGS ?= -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer -O1 -g
UDS_SANITIZER_BINS := $(SANITIZER_BUILD_DIR)/test_uds_adversarial $(SANITIZER_BUILD_DIR)/test_uds_runtime

.PHONY: all clean test test-uds test-sanitize test-sanitize-report test-coverage test-coverage-report test-fuzz test-stm32-facade test-gateway-default-deny test-cia302 test-lss-policy test-cia418 test-inventus-battery test-inventus-battery-data test-protocol-contract test-mock-canopen test-recovery test-acceptance-filter check-vcan

all: $(BUILD_DIR)/test_can_port $(BUILD_DIR)/test_can_port_stm32 $(BUILD_DIR)/test_gateway_default_deny $(BUILD_DIR)/test_cia302_nmt_master $(BUILD_DIR)/test_lss_policy $(BUILD_DIR)/test_cia418_reference $(BUILD_DIR)/test_inventus_battery_od $(BUILD_DIR)/test_protocol_contract $(BUILD_DIR)/test_can_recovery $(BUILD_DIR)/test_can_acceptance_filter $(UDS_TEST_BINS)

$(BUILD_DIR):
	mkdir -p $@

UDS_INCLUDES := -I$(ROOT)/tests/fakes -I$(ROOT)/App/Inc \
	-I$(ROOT)/library/compat/legacy_diagnostics/isotp -I$(ROOT)/library/compat/legacy_diagnostics/uds \
	-I$(ROOT)/library/compat/legacy_diagnostics/uds_stm32
UDS_FLAGS := -DUSE_HAL_CAN_REGISTER_CALLBACKS=1
UDS_CORE_SRCS := $(ROOT)/library/compat/legacy_diagnostics/isotp/isotp.c \
	$(ROOT)/library/compat/legacy_diagnostics/uds/uds.c
UDS_TEST_BINS := $(BUILD_DIR)/test_isotp_contract $(BUILD_DIR)/test_uds_core \
	$(BUILD_DIR)/test_uds_adversarial $(BUILD_DIR)/test_uds_did \
	$(BUILD_DIR)/test_uds_security_provider $(BUILD_DIR)/test_uds_download \
	$(BUILD_DIR)/test_uds_stm32 $(BUILD_DIR)/test_uds_runtime \
	$(BUILD_DIR)/test_uds_canopen_coexistence

$(BUILD_DIR)/test_isotp_contract: $(ROOT)/tests/isotp/test_isotp_contract.c $(ROOT)/library/compat/legacy_diagnostics/isotp/isotp.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(ROOT)/library/compat/legacy_diagnostics/isotp $^ -o $@

$(BUILD_DIR)/test_uds_core: $(ROOT)/tests/uds/test_uds_core.c $(UDS_CORE_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(UDS_INCLUDES) $^ -o $@

$(BUILD_DIR)/test_uds_adversarial: $(ROOT)/tests/uds/test_uds_adversarial.c $(UDS_CORE_SRCS) $(ROOT)/library/compat/legacy_diagnostics/uds/uds_security_provider.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(UDS_INCLUDES) $^ -o $@

$(BUILD_DIR)/test_uds_did: $(ROOT)/tests/uds/test_uds_did.c $(ROOT)/library/compat/legacy_diagnostics/uds/uds_did.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(UDS_INCLUDES) $^ -o $@

$(BUILD_DIR)/test_uds_security_provider: $(ROOT)/tests/uds/test_uds_security_provider.c $(ROOT)/library/compat/legacy_diagnostics/uds/uds_security_provider.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(UDS_INCLUDES) $^ -o $@

$(BUILD_DIR)/test_uds_download: $(ROOT)/tests/uds/test_uds_download.c $(ROOT)/library/compat/legacy_diagnostics/uds/uds_download.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(UDS_INCLUDES) $^ -o $@

$(BUILD_DIR)/test_uds_stm32: $(ROOT)/tests/uds/test_uds_stm32.c $(ROOT)/library/compat/legacy_diagnostics/uds_stm32/uds_stm32.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(UDS_FLAGS) $(UDS_INCLUDES) $^ -o $@

$(BUILD_DIR)/test_uds_runtime: $(ROOT)/tests/uds/test_uds_runtime.c $(ROOT)/tests/fakes/canopen_reference_timing_stub.c \
	$(ROOT)/App/Src/canopen_reference_uds.c $(UDS_CORE_SRCS) \
	$(ROOT)/library/compat/legacy_diagnostics/uds/uds_did.c $(ROOT)/library/compat/legacy_diagnostics/uds/uds_security_provider.c \
	$(ROOT)/library/compat/legacy_diagnostics/uds/uds_download.c $(ROOT)/library/compat/legacy_diagnostics/uds_stm32/uds_stm32.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(UDS_FLAGS) -DCANOPEN_REFERENCE_ENABLE_UDS=1 $(UDS_INCLUDES) $^ -o $@

$(BUILD_DIR)/test_uds_canopen_coexistence: $(ROOT)/tests/integration/test_uds_canopen_coexistence.c \
	$(ROOT)/middleware/canopen/core/can_acceptance_filter.c $(ROOT)/library/compat/legacy_diagnostics/uds_stm32/uds_stm32.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(UDS_FLAGS) $(UDS_INCLUDES) -I$(ROOT)/middleware/canopen/core $^ -o $@

test-uds: $(UDS_TEST_BINS)
	@set -e; for test in $(UDS_TEST_BINS); do "$$test"; done

$(BUILD_DIR)/test_can_port: $(ROOT)/tests/host/test_can_port.c $(ROOT)/middleware/canopen/port/vcan_port.c $(ROOT)/middleware/canopen/port/can_port.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -DCAN_PORT_SOCKETCAN -I$(ROOT)/middleware/canopen/port $< $(ROOT)/middleware/canopen/port/vcan_port.c -o $@

$(BUILD_DIR)/test_cia302_nmt_master: $(ROOT)/tests/test_cia302_nmt_master.c $(ROOT)/middleware/canopen/core/cia302_nmt_master.c $(ROOT)/middleware/canopen/core/cia302_nmt_master.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(ROOT)/middleware/canopen/core $^ -o $@

$(BUILD_DIR)/test_lss_policy: $(ROOT)/tests/test_lss_policy.c $(ROOT)/App/Src/canopen_reference_lss.c $(ROOT)/App/Inc/canopen_reference_lss.h $(ROOT)/tests/fakes/CANopen.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(ROOT)/tests/fakes -I$(ROOT)/App/Inc $^ -o $@

CIA418_OD_FLAGS := -DCANOPEN_REFERENCE_ENABLE_CIA401=0 -DCANOPEN_REFERENCE_ENABLE_CIA402=0 -DCANOPEN_REFERENCE_ENABLE_CIA418=1
CIA418_OD_INCLUDES := -I$(ROOT)/tests/fakes \
	-I$(ROOT)/App/Inc -I$(ROOT)/Generated \
	-I$(ROOT)/third_party/CanOpenSTM32/CANopenNode \
	-I$(ROOT)/third_party/CanOpenSTM32/CANopenNode/301 \
	-I$(ROOT)/third_party/CanOpenSTM32/CANopenNode_STM32

INVENTUS_OD_FLAGS := -DCANOPEN_REFERENCE_ENABLE_CIA401=0 -DCANOPEN_REFERENCE_ENABLE_CIA402=0 -DCANOPEN_REFERENCE_ENABLE_CIA418=0 -DCANOPEN_REFERENCE_ENABLE_INVENTUS_BATTERY=1
INVENTUS_OD_INCLUDES := $(CIA418_OD_INCLUDES)

$(BUILD_DIR)/test_cia418_reference: $(ROOT)/tests/test_cia418_reference.c $(ROOT)/App/Src/cia418_reference.c $(ROOT)/Generated/cia418_OD.c $(ROOT)/third_party/CanOpenSTM32/CANopenNode/301/CO_ODinterface.c $(ROOT)/App/Inc/cia418_reference.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CIA418_OD_FLAGS) $(CIA418_OD_INCLUDES) $^ -o $@

$(BUILD_DIR)/test_inventus_battery_od: $(ROOT)/tests/test_inventus_battery_od.c $(ROOT)/App/Src/inventus_battery_data.c $(ROOT)/Generated/inventus_battery_OD.c $(ROOT)/third_party/CanOpenSTM32/CANopenNode/301/CO_ODinterface.c $(ROOT)/App/Inc/canopen_reference_od.h $(ROOT)/App/Inc/inventus_battery_data.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INVENTUS_OD_FLAGS) $(INVENTUS_OD_INCLUDES) $^ -o $@

$(BUILD_DIR)/test_can_recovery: $(ROOT)/tests/test_can_recovery.c $(ROOT)/App/Src/canopen_reference_can_recovery.c $(ROOT)/App/Inc/canopen_reference_can_recovery.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(ROOT)/App/Inc $^ -o $@

$(BUILD_DIR)/test_can_acceptance_filter: $(ROOT)/tests/test_can_acceptance_filter.c $(ROOT)/middleware/canopen/core/can_acceptance_filter.c $(ROOT)/middleware/canopen/core/can_acceptance_filter.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(ROOT)/middleware/canopen/core $< $(ROOT)/middleware/canopen/core/can_acceptance_filter.c -o $@

$(BUILD_DIR)/test_can_port_stm32: $(ROOT)/tests/test_can_port_stm32.c $(ROOT)/middleware/canopen/port/can_port.c $(ROOT)/middleware/canopen/port/can_port.h $(ROOT)/tests/fakes/stm32f7xx_hal.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -DCAN_PORT_STM32 -DCAN_PORT_FAKE_F767 -I$(ROOT)/tests/fakes -I$(ROOT)/middleware/canopen/port $< $(ROOT)/middleware/canopen/port/can_port.c -o $@

test-cia302: $(BUILD_DIR)/test_cia302_nmt_master
	$(BUILD_DIR)/test_cia302_nmt_master

test-lss-policy: $(BUILD_DIR)/test_lss_policy
	$(BUILD_DIR)/test_lss_policy

test-cia418: $(BUILD_DIR)/test_cia418_reference
	$(BUILD_DIR)/test_cia418_reference

test-inventus-battery: $(BUILD_DIR)/test_inventus_battery_od
	$(BUILD_DIR)/test_inventus_battery_od

test-inventus-battery-data: test-inventus-battery

$(BUILD_DIR)/test_protocol_contract: $(ROOT)/tests/test_protocol_contract.c $(ROOT)/middleware/canopen/core/canopen_reference_protocol.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(ROOT)/middleware/canopen/core $< -o $@

test-protocol-contract: $(BUILD_DIR)/test_protocol_contract
	$(BUILD_DIR)/test_protocol_contract

test-mock-canopen:
	PYTHONDONTWRITEBYTECODE=1 python3 $(ROOT)/scripts/mock_canopen_runner.py

test-recovery: $(BUILD_DIR)/test_can_recovery
	$(BUILD_DIR)/test_can_recovery

test-acceptance-filter: $(BUILD_DIR)/test_can_acceptance_filter
	$(BUILD_DIR)/test_can_acceptance_filter

test-stm32-facade: $(BUILD_DIR)/test_can_port_stm32
	$(BUILD_DIR)/test_can_port_stm32

$(BUILD_DIR)/test_gateway_default_deny: $(ROOT)/tests/test_gateway_default_deny.c $(ROOT)/App/Src/canopen_reference_gateway.c $(ROOT)/App/Inc/canopen_reference_gateway.h $(ROOT)/tests/fakes/CANopen.h $(ROOT)/tests/fakes/CO_gateway_ascii.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -DCANOPEN_REFERENCE_ENABLE_GATEWAY=1 -I$(ROOT)/tests/fakes -I$(ROOT)/App/Inc $< $(ROOT)/App/Src/canopen_reference_gateway.c -o $@

test-gateway-default-deny: $(BUILD_DIR)/test_gateway_default_deny
	$(BUILD_DIR)/test_gateway_default_deny

$(SANITIZER_BUILD_DIR):
	mkdir -p $@

$(SANITIZER_BUILD_DIR)/test_can_recovery: $(ROOT)/tests/test_can_recovery.c $(ROOT)/App/Src/canopen_reference_can_recovery.c $(ROOT)/App/Inc/canopen_reference_can_recovery.h | $(SANITIZER_BUILD_DIR)
	$(CC) $(CFLAGS) $(SANITIZER_FLAGS) -I$(ROOT)/App/Inc $< $(ROOT)/App/Src/canopen_reference_can_recovery.c -o $@

$(SANITIZER_BUILD_DIR)/test_cia302_nmt_master: $(ROOT)/tests/test_cia302_nmt_master.c $(ROOT)/middleware/canopen/core/cia302_nmt_master.c $(ROOT)/middleware/canopen/core/cia302_nmt_master.h | $(SANITIZER_BUILD_DIR)
	$(CC) $(CFLAGS) $(SANITIZER_FLAGS) -I$(ROOT)/middleware/canopen/core $^ -o $@

$(SANITIZER_BUILD_DIR)/test_uds_adversarial: $(ROOT)/tests/uds/test_uds_adversarial.c $(UDS_CORE_SRCS) $(ROOT)/library/compat/legacy_diagnostics/uds/uds_security_provider.c | $(SANITIZER_BUILD_DIR)
	$(CC) $(CFLAGS) $(SANITIZER_FLAGS) $(UDS_INCLUDES) $^ -o $@

$(SANITIZER_BUILD_DIR)/test_uds_runtime: $(ROOT)/tests/uds/test_uds_runtime.c $(ROOT)/tests/fakes/canopen_reference_timing_stub.c \
	$(ROOT)/App/Src/canopen_reference_uds.c $(UDS_CORE_SRCS) \
	$(ROOT)/library/compat/legacy_diagnostics/uds/uds_did.c $(ROOT)/library/compat/legacy_diagnostics/uds/uds_security_provider.c \
	$(ROOT)/library/compat/legacy_diagnostics/uds/uds_download.c $(ROOT)/library/compat/legacy_diagnostics/uds_stm32/uds_stm32.c | $(SANITIZER_BUILD_DIR)
	$(CC) $(CFLAGS) $(SANITIZER_FLAGS) $(UDS_FLAGS) -DCANOPEN_REFERENCE_ENABLE_UDS=1 $(UDS_INCLUDES) $^ -o $@

test-sanitize: $(SANITIZER_BUILD_DIR)/test_can_recovery $(SANITIZER_BUILD_DIR)/test_cia302_nmt_master $(UDS_SANITIZER_BINS)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 $(SANITIZER_BUILD_DIR)/test_can_recovery
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 $(SANITIZER_BUILD_DIR)/test_cia302_nmt_master
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 $(SANITIZER_BUILD_DIR)/test_uds_adversarial
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 $(SANITIZER_BUILD_DIR)/test_uds_runtime

test-sanitize-report: $(SANITIZER_BUILD_DIR)/test_can_recovery $(SANITIZER_BUILD_DIR)/test_cia302_nmt_master $(UDS_SANITIZER_BINS)
	@mkdir -p $(SANITIZER_BUILD_DIR)/report
	@set +e; { \
		printf '%s\n' '# Host sanitizer report'; \
		printf 'asan_ubsan=address,undefined\n'; \
		ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 $(SANITIZER_BUILD_DIR)/test_can_recovery; \
		ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 $(SANITIZER_BUILD_DIR)/test_cia302_nmt_master; \
		ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 $(SANITIZER_BUILD_DIR)/test_uds_adversarial; \
		ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 $(SANITIZER_BUILD_DIR)/test_uds_runtime; \
	} > $(SANITIZER_BUILD_DIR)/report/sanitizer-report.txt 2>&1; \
	status=$$?; cat $(SANITIZER_BUILD_DIR)/report/sanitizer-report.txt; \
	printf 'status=%s\n' "$$status" >> $(SANITIZER_BUILD_DIR)/report/sanitizer-report.txt; \
	exit $$status

$(COVERAGE_BUILD_DIR):
	mkdir -p $@

$(COVERAGE_BUILD_DIR)/test_can_recovery: $(ROOT)/tests/test_can_recovery.c $(ROOT)/App/Src/canopen_reference_can_recovery.c $(ROOT)/App/Inc/canopen_reference_can_recovery.h | $(COVERAGE_BUILD_DIR)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -I$(ROOT)/App/Inc $< $(ROOT)/App/Src/canopen_reference_can_recovery.c -o $@

$(COVERAGE_BUILD_DIR)/test_cia302_nmt_master: $(ROOT)/tests/test_cia302_nmt_master.c $(ROOT)/middleware/canopen/core/cia302_nmt_master.c $(ROOT)/middleware/canopen/core/cia302_nmt_master.h | $(COVERAGE_BUILD_DIR)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -I$(ROOT)/middleware/canopen/core $< $(ROOT)/middleware/canopen/core/cia302_nmt_master.c -o $@

test-coverage: $(COVERAGE_BUILD_DIR)/test_can_recovery $(COVERAGE_BUILD_DIR)/test_cia302_nmt_master
	$(COVERAGE_BUILD_DIR)/test_can_recovery
	$(COVERAGE_BUILD_DIR)/test_cia302_nmt_master
	@mkdir -p $(COVERAGE_BUILD_DIR)/report
	@cd $(COVERAGE_BUILD_DIR) && gcov --json-format -b -c -f -o . test_can_recovery-canopen_reference_can_recovery.gcno > report/test_can_recovery.gcov.txt
	@cd $(COVERAGE_BUILD_DIR) && gcov --json-format -b -c -f -o . test_cia302_nmt_master-cia302_nmt_master.gcno > report/test_cia302.gcov.txt
	@test -s $(COVERAGE_BUILD_DIR)/report/test_can_recovery.gcov.txt
	@test -s $(COVERAGE_BUILD_DIR)/report/test_cia302.gcov.txt

test-coverage-report: test-coverage
	python3 $(ROOT)/scripts/check_host_coverage.py $(COVERAGE_BUILD_DIR) \
		--output $(COVERAGE_BUILD_DIR)/report/coverage-summary.json \
		--min-line $(COVERAGE_MIN_LINE) \
		--min-function $(COVERAGE_MIN_FUNCTION) \
		--min-branch $(COVERAGE_MIN_BRANCH)

test-fuzz:
	@command -v $(FUZZER) >/dev/null 2>&1 || (echo "$(FUZZER) with libFuzzer support is required for test-fuzz" >&2; exit 127)
	@mkdir -p $(FUZZ_BUILD_DIR)
	$(FUZZER) $(CFLAGS) $(FUZZER_FLAGS) \
		-I$(ROOT)/tests/fakes -I$(ROOT)/App/Inc -I$(ROOT)/middleware/canopen/core \
		$(ROOT)/tests/fuzz/fuzz_canopen_frame.c \
		$(ROOT)/middleware/canopen/core/cia302_nmt_master.c \
		$(ROOT)/App/Src/canopen_reference_lss.c \
		-o $(FUZZ_BUILD_DIR)/fuzz_canopen_frame

check-vcan:
	@test -d /sys/class/net/$${CAN_PORT_IFACE:-vcan0} || (echo "Missing $${CAN_PORT_IFACE:-vcan0}; run scripts/setup_vcan.sh" >&2; exit 1)

test: all test-stm32-facade test-gateway-default-deny test-acceptance-filter check-vcan
	CAN_PORT_IFACE=$${CAN_PORT_IFACE:-vcan0} $(BUILD_DIR)/test_can_port
	PYTHONDONTWRITEBYTECODE=1 CAN_PORT_IFACE=$${CAN_PORT_IFACE:-vcan0} python3 $(ROOT)/tests/host/test_sdo.py

clean:
	rm -rf $(BUILD_DIR) $(SANITIZER_BUILD_DIR) $(COVERAGE_BUILD_DIR) $(FUZZ_BUILD_DIR)
