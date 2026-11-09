cmake_minimum_required(VERSION 3.20)
project(uds_iso_tp_library LANGUAGES C)

include(CTest)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(UDS_ISO_TP_BUILD_TESTS "Build standalone host tests" ON)
option(UDS_ISO_TP_BUILD_EXAMPLES "Build STM32 adapter contracts" ON)
option(UDS_ISO_TP_BUILD_PORTABILITY_CHECK "Build strict C99 freestanding portability objects" ON)
set(UDS_ISO_TP_MAX_PAYLOAD 16384 CACHE STRING "Maximum bounded ISO-TP payload")

add_library(uds_iso_tp_isotp STATIC src/isotp.c)
target_include_directories(uds_iso_tp_isotp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(uds_iso_tp_crypto STATIC
    crypto/aes.c
    crypto/aes_cmac.c
    crypto/uds_security_cmac.c
    crypto/sha256.c)
target_include_directories(uds_iso_tp_crypto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/crypto)
target_compile_options(uds_iso_tp_crypto PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
target_compile_definitions(uds_iso_tp_isotp PUBLIC ISOTP_MAX_PAYLOAD=${UDS_ISO_TP_MAX_PAYLOAD})
target_compile_options(uds_iso_tp_isotp PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_library(uds_iso_tp_uds STATIC
    src/uds.c
    src/endpoint.c
    src/uds_did.c
    src/uds_download.c
    src/uds_dtc.c
    src/uds_memory.c
    src/uds_services.c
    src/uds_wear_leveling.c
    src/uds_flash_fsm.c
    src/uds_security_gate.c
    src/uds_security_nvm.c
    src/boot_floor.c
    src/boot_verify.c)
target_include_directories(uds_iso_tp_uds PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include/uds_iso_tp)
target_compile_options(uds_iso_tp_uds PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
target_link_libraries(uds_iso_tp_uds PUBLIC uds_iso_tp_isotp uds_iso_tp_crypto)

if(UDS_ISO_TP_BUILD_PORTABILITY_CHECK)
    add_library(uds_iso_tp_c99_portability OBJECT
        src/isotp.c
        src/uds.c
        src/endpoint.c
        src/uds_did.c
        src/uds_download.c
        src/uds_dtc.c
        src/uds_memory.c
        src/uds_services.c
        src/uds_wear_leveling.c
        src/uds_flash_fsm.c
        src/uds_security_gate.c
        src/uds_security_nvm.c
        src/boot_floor.c
        src/boot_verify.c
        crypto/sha256.c)
    target_include_directories(uds_iso_tp_c99_portability PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include/uds_iso_tp)
    target_compile_definitions(uds_iso_tp_c99_portability PRIVATE
        ISOTP_MAX_PAYLOAD=${UDS_ISO_TP_MAX_PAYLOAD})
    target_compile_options(uds_iso_tp_c99_portability PRIVATE
        -std=c99 -ffreestanding -Wall -Wextra -Wconversion -Wsign-conversion -Werror)
endif()

if(UDS_ISO_TP_BUILD_TESTS)
    add_compile_definitions(UDS_ISO_TP_TESTING=1)
    enable_testing()
    add_executable(uds_iso_tp_isotp_test tests/isotp/test_isotp.c)
    target_link_libraries(uds_iso_tp_isotp_test PRIVATE uds_iso_tp_isotp)
    target_compile_options(uds_iso_tp_isotp_test PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_isotp_contract COMMAND uds_iso_tp_isotp_test)

    add_executable(uds_iso_tp_uds_test tests/uds/test_uds.c)
    target_link_libraries(uds_iso_tp_uds_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_uds_test PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_uds_contract COMMAND uds_iso_tp_uds_test)

    add_executable(uds_iso_tp_session_security_test
        tests/uds/test_session_security.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/security/uds_security_reference.c)
    target_include_directories(uds_iso_tp_session_security_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/security)
    target_link_libraries(uds_iso_tp_session_security_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_session_security_test PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_session_security_contract COMMAND uds_iso_tp_session_security_test)

    add_executable(uds_iso_tp_security_test_key
        ${CMAKE_CURRENT_SOURCE_DIR}/../tools/security_test_key.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/security/uds_security_reference.c)
    target_include_directories(uds_iso_tp_security_test_key PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/security
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(uds_iso_tp_security_test_key PRIVATE uds_iso_tp_crypto)
    target_compile_options(uds_iso_tp_security_test_key PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_security_reference_utility
             COMMAND uds_iso_tp_security_test_key 1 00112233)
    set_tests_properties(uds_iso_tp_security_reference_utility PROPERTIES
                         PASS_REGULAR_EXPRESSION "key  = A5 4B E1 0F")
    add_test(NAME uds_iso_tp_security_reference_utility_level2
             COMMAND uds_iso_tp_security_test_key 2 0102030405060708090a0b0c0d0e0f10)
    set_tests_properties(uds_iso_tp_security_reference_utility_level2 PROPERTIES
                         PASS_REGULAR_EXPRESSION "key  = ")

    add_executable(uds_iso_tp_multilevel_security_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/uds/test_multilevel_security.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_security_app.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_platform.c)
    target_include_directories(uds_iso_tp_multilevel_security_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Inc)
    target_link_libraries(uds_iso_tp_multilevel_security_test PRIVATE
        uds_iso_tp_uds uds_iso_tp_crypto)
    target_compile_options(uds_iso_tp_multilevel_security_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_multilevel_security_contract
             COMMAND uds_iso_tp_multilevel_security_test)

    add_executable(uds_iso_tp_endpoint_test tests/uds/test_endpoint.c)
    target_link_libraries(uds_iso_tp_endpoint_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_endpoint_test PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_endpoint_contract COMMAND uds_iso_tp_endpoint_test)

    add_executable(uds_iso_tp_services_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/uds/test_services.c)
    target_include_directories(uds_iso_tp_services_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(uds_iso_tp_services_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_services_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_services_contract
             COMMAND uds_iso_tp_services_test)

    add_executable(uds_iso_tp_services_extended_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/uds/test_services_extended.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_io_control_app.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_link_control_app.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_roe_app.c)
    target_include_directories(uds_iso_tp_services_extended_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Inc)
    target_link_libraries(uds_iso_tp_services_extended_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_services_extended_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_services_extended_contract
             COMMAND uds_iso_tp_services_extended_test)

    add_executable(uds_iso_tp_bootloader_dtc_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/uds/test_bootloader_dtc_app.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_dtc_app.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_bootloader.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/boot_jump.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_platform.c)
    target_include_directories(uds_iso_tp_bootloader_dtc_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Inc)
    target_link_libraries(uds_iso_tp_bootloader_dtc_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_bootloader_dtc_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_bootloader_dtc_contract
             COMMAND uds_iso_tp_bootloader_dtc_test)

    add_executable(uds_iso_tp_boot_floor_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/uds/test_boot_floor.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_bootloader.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/boot_jump.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_platform.c)
    target_include_directories(uds_iso_tp_boot_floor_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/uds)
    target_link_libraries(uds_iso_tp_boot_floor_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_boot_floor_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_boot_floor_contract COMMAND uds_iso_tp_boot_floor_test)

    add_executable(uds_iso_tp_c092_bootloader_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/uds/test_c092_bootloader.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_bootloader.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/boot_jump.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_platform.c)
    target_include_directories(uds_iso_tp_c092_bootloader_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Inc)
    target_link_libraries(uds_iso_tp_c092_bootloader_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_c092_bootloader_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_c092_bootloader_contract
             COMMAND uds_iso_tp_c092_bootloader_test)

    add_executable(uds_iso_tp_wear_leveling_test
        tests/uds/test_wear_leveling.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_dtc_app.c)
    target_include_directories(uds_iso_tp_wear_leveling_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Inc)
    target_link_libraries(uds_iso_tp_wear_leveling_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_wear_leveling_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_wear_leveling_contract
             COMMAND uds_iso_tp_wear_leveling_test)

    add_executable(uds_iso_tp_flash_fsm_test
        tests/uds/test_flash_fsm.c)
    target_include_directories(uds_iso_tp_flash_fsm_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp)
    target_link_libraries(uds_iso_tp_flash_fsm_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_flash_fsm_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_flash_fsm_contract
             COMMAND uds_iso_tp_flash_fsm_test)

    add_executable(uds_iso_tp_did_security_auth_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/uds/test_did_security_auth_app.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_did_app.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_dtc_app.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_auth_app.c)
    target_include_directories(uds_iso_tp_did_security_auth_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Inc)
    target_link_libraries(uds_iso_tp_did_security_auth_test PRIVATE uds_iso_tp_uds uds_iso_tp_crypto)
    target_compile_options(uds_iso_tp_did_security_auth_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_did_security_auth_contract
             COMMAND uds_iso_tp_did_security_auth_test)

    add_executable(uds_iso_tp_memory_service_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/uds/test_memory_service.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_memory_app.c)
    target_include_directories(uds_iso_tp_memory_service_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Inc)
    target_link_libraries(uds_iso_tp_memory_service_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_memory_service_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_memory_service_contract
             COMMAND uds_iso_tp_memory_service_test)

    add_executable(uds_iso_tp_security_nvm_test
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/uds/test_security_nvm.c)
    target_include_directories(uds_iso_tp_security_nvm_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(uds_iso_tp_security_nvm_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_security_nvm_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_security_nvm_contract COMMAND uds_iso_tp_security_nvm_test)

    add_executable(uds_iso_tp_security_gate_test
        tests/uds/test_security_gate.c)
    target_include_directories(uds_iso_tp_security_gate_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp
        ${CMAKE_CURRENT_BINARY_DIR}/include)
    target_link_libraries(uds_iso_tp_security_gate_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_security_gate_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_security_gate_contract
             COMMAND uds_iso_tp_security_gate_test)

    add_executable(uds_iso_tp_download_test
        tests/uds/test_download.c)
    target_include_directories(uds_iso_tp_download_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp)
    target_link_libraries(uds_iso_tp_download_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_download_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_download_contract
             COMMAND uds_iso_tp_download_test)

    add_library(uds_iso_tp_ports STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../ports/stm32c0/port_stm32c0.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../ports/stm32f1/port_stm32f1.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../ports/stm32f4/port_stm32f4.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../ports/stm32f7/port_stm32f7.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../ports/stm32g4/port_stm32g4.c)
    target_include_directories(uds_iso_tp_ports PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../ports/include)
    target_compile_options(uds_iso_tp_ports PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wmissing-prototypes -Werror)

    add_executable(uds_iso_tp_ports_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/portability/test_ports.c)
    target_include_directories(uds_iso_tp_ports_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../ports/include)
    target_link_libraries(uds_iso_tp_ports_test PRIVATE uds_iso_tp_ports)
    target_compile_options(uds_iso_tp_ports_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_ports_contract
             COMMAND uds_iso_tp_ports_test)

    add_executable(uds_iso_tp_sha256_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/crypto/test_sha256.c)
    target_include_directories(uds_iso_tp_sha256_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto)
    target_link_libraries(uds_iso_tp_sha256_test PRIVATE uds_iso_tp_crypto)
    target_compile_options(uds_iso_tp_sha256_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_sha256_contract
             COMMAND uds_iso_tp_sha256_test)

    add_executable(uds_iso_tp_boot_verify_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/uds/test_boot_verify.c)
    target_include_directories(uds_iso_tp_boot_verify_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto)
    target_link_libraries(uds_iso_tp_boot_verify_test PRIVATE uds_iso_tp_uds uds_iso_tp_crypto)
    target_compile_options(uds_iso_tp_boot_verify_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_boot_verify_contract
             COMMAND uds_iso_tp_boot_verify_test)

    add_executable(uds_iso_tp_fuzz_isotp_rx
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/fuzzing/fuzz_isotp_rx.c)
    target_include_directories(uds_iso_tp_fuzz_isotp_rx PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(uds_iso_tp_fuzz_isotp_rx PRIVATE uds_iso_tp_isotp)
    target_compile_options(uds_iso_tp_fuzz_isotp_rx PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_fuzz_isotp_rx_contract
             COMMAND uds_iso_tp_fuzz_isotp_rx)

    add_executable(uds_iso_tp_fuzz_uds_request
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/fuzzing/fuzz_uds_request.c)
    target_include_directories(uds_iso_tp_fuzz_uds_request PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp)
    target_link_libraries(uds_iso_tp_fuzz_uds_request PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_fuzz_uds_request PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_fuzz_uds_request_contract
             COMMAND uds_iso_tp_fuzz_uds_request)
endif()

if(UDS_ISO_TP_BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32f767_bxcan
                     ${CMAKE_CURRENT_BINARY_DIR}/stm32f767_bxcan)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32_fdcan
                     ${CMAKE_CURRENT_BINARY_DIR}/stm32_fdcan)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../examples/body_control_module_bcm
                     ${CMAKE_CURRENT_BINARY_DIR}/body_control_module_bcm)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../examples/battery_management_system_bms
                     ${CMAKE_CURRENT_BINARY_DIR}/battery_management_system_bms)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../examples/telematics_gateway_fota_client
                     ${CMAKE_CURRENT_BINARY_DIR}/telematics_gateway_fota_client)
endif()

if(UDS_ISO_TP_BUILD_TESTS AND UDS_ISO_TP_BUILD_EXAMPLES)
    add_executable(uds_iso_tp_adapters_test tests/uds/test_adapters.c)
    target_include_directories(uds_iso_tp_adapters_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32f767_bxcan
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32_fdcan)
    target_link_libraries(uds_iso_tp_adapters_test PRIVATE
        stm32f767_bxcan_adapter stm32_fdcan_adapter uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_adapters_test PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_adapters_contract COMMAND uds_iso_tp_adapters_test)

    add_executable(uds_iso_tp_reset_recovery_test tests/uds/test_reset_recovery.c)
    target_link_libraries(uds_iso_tp_reset_recovery_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_reset_recovery_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_reset_recovery_contract
             COMMAND uds_iso_tp_reset_recovery_test)

    add_executable(uds_iso_tp_c092_transport_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/portability/test_c092_transport_shim.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32c092/can_transport_fdcan.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32c092/uds_diagnostics.c)
    target_include_directories(uds_iso_tp_c092_transport_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/portability/c092_shim
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32c092
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_options(uds_iso_tp_c092_transport_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_c092_transport_contract
             COMMAND uds_iso_tp_c092_transport_test)

    add_executable(uds_iso_tp_c092_diagnostics_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/portability/test_c092_diagnostics.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32c092/uds_diagnostics.c)
    target_include_directories(uds_iso_tp_c092_diagnostics_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32c092)
    target_compile_options(uds_iso_tp_c092_diagnostics_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_c092_diagnostics_contract
             COMMAND uds_iso_tp_c092_diagnostics_test)

    add_executable(uds_iso_tp_c092_immediate_reset_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/portability/test_c092_app_shim.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32c092/uds_app_fdcan.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32c092/can_transport_fdcan.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32c092/uds_diagnostics.c)
    target_include_directories(uds_iso_tp_c092_immediate_reset_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/portability/c092_shim
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32c092
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(uds_iso_tp_c092_immediate_reset_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_c092_immediate_reset_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_c092_immediate_reset_rx_contract
             COMMAND uds_iso_tp_c092_immediate_reset_test)

    add_executable(uds_iso_tp_c092_security_shim_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/portability/test_c092_security_shim.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src/uds_security_app.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32c092/uds_platform_fdcan.c)
    target_include_directories(uds_iso_tp_c092_security_shim_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32c092
        ${CMAKE_CURRENT_SOURCE_DIR}/../App/Inc
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto)
    target_compile_definitions(uds_iso_tp_c092_security_shim_test PRIVATE
        STM32C092xx)
    target_link_libraries(uds_iso_tp_c092_security_shim_test PRIVATE uds_iso_tp_uds uds_iso_tp_crypto)
    target_compile_options(uds_iso_tp_c092_security_shim_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_c092_security_shim_contract
             COMMAND uds_iso_tp_c092_security_shim_test)


    add_executable(uds_iso_tp_aes_cmac_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/crypto/test_aes_cmac.c)
    target_include_directories(uds_iso_tp_aes_cmac_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto)
    target_link_libraries(uds_iso_tp_aes_cmac_test PRIVATE uds_iso_tp_crypto)
    target_compile_options(uds_iso_tp_aes_cmac_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_aes_cmac_contract
             COMMAND uds_iso_tp_aes_cmac_test)

    add_executable(uds_iso_tp_dtc_test
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/uds/test_dtc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/fixtures/dtc/dtc_fixture.c)
    target_include_directories(uds_iso_tp_dtc_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/fixtures/dtc)
    target_link_libraries(uds_iso_tp_dtc_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_dtc_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_dtc_contract
             COMMAND uds_iso_tp_dtc_test)

    add_test(NAME bcm_application_example_contract COMMAND bcm_application_example)
    add_test(NAME bms_application_example_contract COMMAND bms_application_example)
    add_test(NAME fota_client_example_contract COMMAND fota_client_example)
endif()
