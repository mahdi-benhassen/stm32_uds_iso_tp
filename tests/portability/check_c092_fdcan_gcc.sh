#!/bin/sh
set -eu

REPO_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
C092_ROOT=${1:-${C092_PROJECT_ROOT:-}}

if [ -z "$C092_ROOT" ]; then
    printf '%s\n' "usage: $0 /path/to/STM32C092_UDS" >&2
    printf '%s\n' "or set C092_PROJECT_ROOT; vendor HAL/CubeMX sources are intentionally not committed" >&2
    exit 2
fi

INCLUDES="-I${REPO_ROOT}/examples/stm32c092 -I${REPO_ROOT}/library/include -I${C092_ROOT}/Inc -I${C092_ROOT}/Drivers/STM32C0xx_HAL_Driver/Inc -I${C092_ROOT}/Drivers/CMSIS/Device/ST/STM32C0xx/Include -I${C092_ROOT}/Drivers/CMSIS/Include"
COMMON="-std=c99 -ffreestanding -fsyntax-only -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wundef -DSTM32C092xx -DUSE_HAL_DRIVER -DUSE_HAL_FDCAN_REGISTER_CALLBACKS=0 -DISOTP_MAX_PAYLOAD=4095 ${INCLUDES}"

for source in can_transport_fdcan.c uds_app_fdcan.c uds_platform_fdcan.c uds_diagnostics.c; do
    arm-none-eabi-gcc ${COMMON} "${REPO_ROOT}/examples/stm32c092/${source}"
done

printf '%s\n' "C092 FDCAN adapter syntax check passed against ${C092_ROOT}"
