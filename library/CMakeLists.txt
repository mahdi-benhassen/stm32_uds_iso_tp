cmake_minimum_required(VERSION 3.20)
project(uds_iso_tp_library LANGUAGES C)

include(CTest)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(UDS_ISO_TP_BUILD_TESTS "Build standalone host tests" ON)
option(UDS_ISO_TP_BUILD_EXAMPLES "Build STM32 adapter contracts" ON)
set(UDS_ISO_TP_MAX_PAYLOAD 16384 CACHE STRING "Maximum bounded ISO-TP payload")

add_library(uds_iso_tp_isotp STATIC src/isotp.c)
target_include_directories(uds_iso_tp_isotp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(uds_iso_tp_isotp PUBLIC ISOTP_MAX_PAYLOAD=${UDS_ISO_TP_MAX_PAYLOAD})
target_compile_options(uds_iso_tp_isotp PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_library(uds_iso_tp_uds STATIC
    src/uds.c
    src/endpoint.c
    src/uds_did.c
    src/uds_security_provider.c
    src/uds_download.c)
target_include_directories(uds_iso_tp_uds PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/uds_iso_tp)
target_compile_options(uds_iso_tp_uds PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
target_link_libraries(uds_iso_tp_uds PUBLIC uds_iso_tp_isotp)

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

    add_executable(uds_iso_tp_endpoint_test tests/uds/test_endpoint.c)
    target_link_libraries(uds_iso_tp_endpoint_test PRIVATE uds_iso_tp_uds)
    target_compile_options(uds_iso_tp_endpoint_test PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
    add_test(NAME uds_iso_tp_endpoint_contract COMMAND uds_iso_tp_endpoint_test)
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
endif()
