#!/usr/bin/env bash
# SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
# Check a GNU ld map produced by the target build. This is a size budget gate,
# not a stack-depth or worst-case execution-time measurement.
set -euo pipefail

MAP_FILE=${1:?usage: check_memory_budget.sh MAP_FILE [JSON_OUTPUT]}
JSON_OUTPUT=${2:-build/memory-budget.json}
MAX_TEXT_BYTES=${MAX_TEXT_BYTES:-1572864}
MAX_DATA_BYTES=${MAX_DATA_BYTES:-524288}
MAX_BSS_BYTES=${MAX_BSS_BYTES:-524288}
MAX_FLASH_LOAD_BYTES=${MAX_FLASH_LOAD_BYTES:-1572864}
MAX_RAM_BYTES=${MAX_RAM_BYTES:-524288}

[[ -s "$MAP_FILE" ]] || { echo "memory budget: missing map file: $MAP_FILE" >&2; exit 2; }

hex_section() {
    local section=$1
    awk -v section="$section" '$1 == section && $2 != "0x00000000" { print $3; exit }' "$MAP_FILE"
}
hex_to_dec() {
    local value=${1#0x}
    printf '%d' "$((16#$value))"
}

text_hex=$(hex_section .text)
data_hex=$(hex_section .data)
bss_hex=$(hex_section .bss)
[[ -n "$text_hex" && -n "$data_hex" && -n "$bss_hex" ]] || {
    echo "memory budget: required .text/.data/.bss sections are absent from $MAP_FILE" >&2
    exit 2
}
text_bytes=$(hex_to_dec "$text_hex")
data_bytes=$(hex_to_dec "$data_hex")
bss_bytes=$(hex_to_dec "$bss_hex")
flash_load_bytes=$((text_bytes + data_bytes))
ram_bytes=$((data_bytes + bss_bytes))

status=passed
for check in \
    "$text_bytes:$MAX_TEXT_BYTES:.text" \
    "$data_bytes:$MAX_DATA_BYTES:.data" \
    "$bss_bytes:$MAX_BSS_BYTES:.bss" \
    "$flash_load_bytes:$MAX_FLASH_LOAD_BYTES:flash_load" \
    "$ram_bytes:$MAX_RAM_BYTES:ram"; do
    IFS=: read -r actual limit label <<< "$check"
    if (( actual > limit )); then
        status=failed
        printf 'memory budget: FAIL %-10s actual=%d limit=%d bytes\n' "$label" "$actual" "$limit" >&2
    else
        printf 'memory budget: PASS %-10s actual=%d limit=%d bytes\n' "$label" "$actual" "$limit"
    fi
done

mkdir -p "$(dirname "$JSON_OUTPUT")"
cat > "$JSON_OUTPUT" <<EOF
{
  "schema": "stm32-canopen-memory-budget-v1",
  "map_file": "$MAP_FILE",
  "status": "$status",
  "sections": {
    "text_bytes": $text_bytes,
    "data_bytes": $data_bytes,
    "bss_bytes": $bss_bytes,
    "flash_load_bytes": $flash_load_bytes,
    "ram_bytes": $ram_bytes
  },
  "limits": {
    "text_bytes": $MAX_TEXT_BYTES,
    "data_bytes": $MAX_DATA_BYTES,
    "bss_bytes": $MAX_BSS_BYTES,
    "flash_load_bytes": $MAX_FLASH_LOAD_BYTES,
    "ram_bytes": $MAX_RAM_BYTES
  },
  "scope": "GNU ld section-size budget; stack depth and worst-case timing require target instrumentation"
}
EOF
cat "$JSON_OUTPUT"
[[ "$status" = passed ]]
