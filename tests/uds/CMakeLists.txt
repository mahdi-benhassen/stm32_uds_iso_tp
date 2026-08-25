cmake_minimum_required(VERSION 3.22)
project(uds_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
enable_testing()

set(ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(UDS_INCLUDES
    "${ROOT}/tests/fakes"
    "${ROOT}/App/Inc"
    "${ROOT}/middleware/canopen/core"
    "${ROOT}/library/compat/legacy_diagnostics/isotp"
    "${ROOT}/library/compat/legacy_diagnostics/uds"
    "${ROOT}/library/compat/legacy_diagnostics/uds_stm32")
set(STRICT_FLAGS -Wall -Wextra -Wpedantic -Wconversion -Werror)
set(ISOTP_SRC "${ROOT}/library/compat/legacy_diagnostics/isotp/isotp.c")
set(UDS_SRC "${ROOT}/library/compat/legacy_diagnostics/uds/uds.c")

function(add_uds_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE ${UDS_INCLUDES})
    target_compile_options(${name} PRIVATE ${STRICT_FLAGS})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_uds_test(test_isotp_contract
    "${ROOT}/tests/isotp/test_isotp_contract.c" ${ISOTP_SRC})
add_uds_test(test_uds_core
    "${ROOT}/tests/uds/test_uds_core.c" ${ISOTP_SRC} ${UDS_SRC})
add_uds_test(test_uds_adversarial
    "${ROOT}/tests/uds/test_uds_adversarial.c" ${ISOTP_SRC} ${UDS_SRC}
    "${ROOT}/library/compat/legacy_diagnostics/uds/uds_security_provider.c")
add_uds_test(test_uds_did
    "${ROOT}/tests/uds/test_uds_did.c"
    "${ROOT}/library/compat/legacy_diagnostics/uds/uds_did.c")
add_uds_test(test_uds_security_provider
    "${ROOT}/tests/uds/test_uds_security_provider.c"
    "${ROOT}/library/compat/legacy_diagnostics/uds/uds_security_provider.c")
add_uds_test(test_uds_download
    "${ROOT}/tests/uds/test_uds_download.c"
    "${ROOT}/library/compat/legacy_diagnostics/uds/uds_download.c")

add_uds_test(test_uds_stm32
    "${ROOT}/tests/uds/test_uds_stm32.c"
    "${ROOT}/library/compat/legacy_diagnostics/uds_stm32/uds_stm32.c")
target_compile_definitions(test_uds_stm32 PRIVATE USE_HAL_CAN_REGISTER_CALLBACKS=1)

add_uds_test(test_uds_runtime
    "${ROOT}/tests/uds/test_uds_runtime.c"
    "${ROOT}/tests/fakes/canopen_reference_timing_stub.c"
    "${ROOT}/App/Src/canopen_reference_uds.c" ${ISOTP_SRC} ${UDS_SRC}
    "${ROOT}/library/compat/legacy_diagnostics/uds/uds_did.c"
    "${ROOT}/library/compat/legacy_diagnostics/uds/uds_security_provider.c"
    "${ROOT}/library/compat/legacy_diagnostics/uds/uds_download.c"
    "${ROOT}/library/compat/legacy_diagnostics/uds_stm32/uds_stm32.c")
target_compile_definitions(test_uds_runtime PRIVATE
    USE_HAL_CAN_REGISTER_CALLBACKS=1 CANOPEN_REFERENCE_ENABLE_UDS=1)

add_uds_test(test_uds_canopen_coexistence
    "${ROOT}/tests/integration/test_uds_canopen_coexistence.c"
    "${ROOT}/middleware/canopen/core/can_acceptance_filter.c"
    "${ROOT}/library/compat/legacy_diagnostics/uds_stm32/uds_stm32.c")
target_compile_definitions(test_uds_canopen_coexistence PRIVATE
    USE_HAL_CAN_REGISTER_CALLBACKS=1)
