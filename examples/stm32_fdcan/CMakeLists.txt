add_library(stm32_fdcan_adapter STATIC adapter_contract.c)
target_include_directories(stm32_fdcan_adapter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(stm32_fdcan_adapter PUBLIC uds_iso_tp_uds)
target_compile_options(stm32_fdcan_adapter PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
