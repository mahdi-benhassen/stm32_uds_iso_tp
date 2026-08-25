cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(stm32f767_uds_iso_tp LANGUAGES C ASM)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_executable(${CMAKE_PROJECT_NAME})
add_subdirectory(cmake/stm32cubemx)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    App/Src/can_transport.c
    App/Src/uds_app.c
    App/Src/uds_platform.c
    library/src/isotp.c
    library/src/uds.c
    library/src/uds_did.c
    library/src/uds_download.c
    library/src/endpoint.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    App/Inc
    library/include
    library/include/uds_iso_tp
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    ISOTP_MAX_PAYLOAD=16384
)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wformat=2 -Wconversion -Wshadow -Wundef -Werror=conversion
)

# The CubeMX-generated syscalls shim contains intentional libc compatibility
# conversions; keep project-owned application and library sources fail-on-warning.
set_source_files_properties(Core/Src/syscalls.c PROPERTIES
    COMPILE_OPTIONS "-Wno-error=conversion;-Wno-error=sign-conversion")

target_link_libraries(${CMAKE_PROJECT_NAME} stm32cubemx)

find_program(ARM_NONE_EABI_OBJCOPY arm-none-eabi-objcopy)
if(NOT ARM_NONE_EABI_OBJCOPY)
    set(ARM_NONE_EABI_OBJCOPY objcopy)
endif()
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${ARM_NONE_EABI_OBJCOPY} -O ihex
        $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${ARM_NONE_EABI_OBJCOPY} -O binary
        $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
)
