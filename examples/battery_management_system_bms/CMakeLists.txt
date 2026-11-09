cmake_minimum_required(VERSION 3.20)
project(bms_application_example LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(bms_application_example
    main.c
    bms_app.c
    bms_uds_adapter.c
)

target_include_directories(bms_application_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../library/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../library/include/uds_iso_tp
)

target_link_libraries(bms_application_example PRIVATE
    uds_iso_tp_uds
    uds_iso_tp_isotp
    uds_iso_tp_crypto
)

target_compile_options(bms_application_example PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror
)
