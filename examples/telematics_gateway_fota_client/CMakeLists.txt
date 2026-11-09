cmake_minimum_required(VERSION 3.20)
project(fota_client_example LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(fota_client_example
    main.c
    fota_client.c
    simulated_ecu.c
)

target_include_directories(fota_client_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../library/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../library/include/uds_iso_tp
)

target_link_libraries(fota_client_example PRIVATE
    uds_iso_tp_uds
    uds_iso_tp_isotp
    uds_iso_tp_crypto
)

target_compile_options(fota_client_example PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror
)
