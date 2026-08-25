add_library(stm32f767_bxcan_adapter STATIC adapter_contract.c)
target_include_directories(stm32f767_bxcan_adapter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(stm32f767_bxcan_adapter PUBLIC uds_iso_tp_uds)
target_compile_options(stm32f767_bxcan_adapter PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
