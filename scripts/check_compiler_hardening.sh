#!/bin/sh
# SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
# Evaluate host compiler support for the hardening profile. This does not claim
# that an STM32 product image has these flags until the target build adopts them.
set -eu

CC=${CC:-cc}
OUTPUT=${1:-build/hardening/compiler-hardening.json}
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

SOURCE="$TMP_DIR/probe.c"
OBJECT="$TMP_DIR/probe.o"
printf '%s\n' 'int main(void) { return 0; }' > "$SOURCE"

FLAGS='-fstack-protector-strong -fno-common -fno-delete-null-pointer-checks -Wformat=2 -Wconversion -Wshadow -Wundef'
# shellcheck disable=SC2086
if "$CC" -std=c11 -Wall -Wextra -Werror $FLAGS -c "$SOURCE" -o "$OBJECT" >"$TMP_DIR/compiler.log" 2>&1; then
    status=passed
else
    status=failed
fi

mkdir -p "$(dirname "$OUTPUT")"
{
    printf '{\n'
    printf '  "schema": "stm32-canopen-hardening-evaluation-v1",\n'
    printf '  "compiler": "%s",\n' "$CC"
    printf '  "status": "%s",\n' "$status"
    printf '  "flags": ['
    first=true
    for flag in $FLAGS; do
        if [ "$first" = true ]; then first=false; else printf ', '; fi
        printf '"%s"' "$flag"
    done
    printf '],\n'
    printf '  "scope": "host compiler flag support probe; target firmware adoption requires target-build review",\n'
    printf '  "diagnostics": '
    if [ "$status" = passed ]; then
        printf 'null\n'
    else
        printf '"compiler probe failed; inspect CI log"\n'
    fi
    printf '}\n'
} > "$OUTPUT"
cat "$OUTPUT"
[ "$status" = passed ]
