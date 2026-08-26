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
target_include_directories(uds_iso_tp_isotp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(uds_iso_tp_crypto STATIC
    crypto/aes.c
    crypto/aes_cmac.c
    crypto/uds_security_cmac.c)
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
    src/uds_services.c)
target_include_directories(uds_iso_tp_uds PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp)
target_compile_options(uds_iso_tp_uds PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
target_link_libraries(uds_iso_tp_uds PUBLIC uds_iso_tp_isotp)

if(UDS_ISO_TP_BUILD_PORTABILITY_CHECK)
    add_library(uds_iso_tp_c99_portability OBJECT
        src/isotp.c
        src/uds.c
        src/endpoint.c
        src/uds_did.c
        src/uds_download.c
        src/uds_dtc.c
        src/uds_services.c)
    target_include_directories(uds_iso_tp_c99_portability PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp)
    target_compile_definitions(uds_iso_tp_c99_portability PRIVATE
        ISOTP_MAX_PAYLOAD=${UDS_ISO_TP_MAX_PAYLOAD})
    target_compile_options(uds_iso_tp_c99_portability PRIVATE
        -std=c99 -ffreestanding -Wall -Wextra -Wconversion -Wsign-conversion -Werror)
endif()

if(UDS_ISO_TP_BUILD_TESTS)
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
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_options(uds_iso_tp_security_test_key PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_security_reference_utility
             COMMAND uds_iso_tp_security_test_key 1 00112233)
    set_tests_properties(uds_iso_tp_security_reference_utility PROPERTIES
                         PASS_REGULAR_EXPRESSION "key  = A5 4B E1 0F")

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
endif()

if(UDS_ISO_TP_BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32f767_bxcan
                     ${CMAKE_CURRENT_BINARY_DIR}/stm32f767_bxcan)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../examples/stm32_fdcan
                     ${CMAKE_CURRENT_BINARY_DIR}/stm32_fdcan)
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

    add_executable(uds_iso_tp_ecu_reset_lifecycle_test tests/uds/test_ecu_reset_lifecycle.c)
    target_link_libraries(uds_iso_tp_ecu_reset_lifecycle_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_ecu_reset_lifecycle_test PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_ecu_reset_transaction_lifecycle
             COMMAND uds_iso_tp_ecu_reset_lifecycle_test)

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
endif()
