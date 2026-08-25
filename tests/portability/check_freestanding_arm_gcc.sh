#!/bin/sh
set -eu

PAYLOAD_BOUND="${1:-4095}"
OUTPUT_DIR="${2:-build/portability-arm-gcc}"

mkdir -p "$OUTPUT_DIR"
for source in library/src/*.c; do
    object="$OUTPUT_DIR/$(basename "$source" .c).o"
    arm-none-eabi-gcc \
        -mcpu=cortex-m0plus \
        -mthumb \
        -std=c99 \
        -ffreestanding \
        -fno-builtin \
        -Wall \
        -Wextra \
        -Wconversion \
        -Wsign-conversion \
        -Werror \
        -DISOTP_MAX_PAYLOAD="$PAYLOAD_BOUND" \
        -Ilibrary/include \
        -Ilibrary/include/uds_iso_tp \
        -c "$source" \
        -o "$object"
done

printf 'freestanding ARM GCC portability compile passed (Cortex-M0+, ISOTP_MAX_PAYLOAD=%s)\n' "$PAYLOAD_BOUND"
