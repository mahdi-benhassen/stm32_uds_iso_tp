#!/usr/bin/env sh
# SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
# Record the source and toolchain inputs required to reproduce a firmware build.
#
# Branch adaptation (stm32f767_canopen_cubemx): the STM32CubeF7 HAL/CMSIS live
# in-repo under Drivers/ instead of an external pinned worktree, so vendor
# provenance is recorded as a deterministic content hash of that tree. The
# emitted JSON schema is unchanged.
set -eu

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <output-file>" >&2
    exit 64
fi

OUTPUT=$1
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CANOPENSTM32_DIR="$ROOT/third_party/CanOpenSTM32"
VENDOR_DIR="$ROOT/Drivers"
LINKER_SCRIPT=${STM32_F7_LINKER_SCRIPT:-$ROOT/STM32F767xx_FLASH.ld}
OD_FILE="$ROOT/Generated/OD.c"
PERSONALITY=${CANOPEN_REFERENCE_PERSONALITY:-unspecified}
JSON_OUTPUT=${BUILD_MANIFEST_JSON:-${OUTPUT%.txt}.json}

if ! git -C "$CANOPENSTM32_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "Manifest generation requires the CanOpenSTM32 Git worktree." >&2
    exit 66
fi

mkdir -p "$(dirname -- "$OUTPUT")"
source_revision=$(git -C "$ROOT" rev-parse HEAD)
source_dirty=$(test -n "$(git -C "$ROOT" status --porcelain)" && printf true || printf false)
canopenstm32_revision=$(git -C "$CANOPENSTM32_DIR" rev-parse HEAD)
vendor_tree_hash=$(cd "$VENDOR_DIR" && find . -type f ! -path '*/.*' -print0 \
    | sort -z | xargs -0 sha256sum | sha256sum | awk '{print $1}')
arm_gcc_version=$(arm-none-eabi-gcc --version | sed -n '1p')
arm_ld_version=$(arm-none-eabi-ld --version | sed -n '1p')
cmake_version=$(cmake --version | sed -n '1p')
od_sha256=$(sha256sum "$OD_FILE" | awk '{print $1}')
linker_sha256=$(sha256sum "$LINKER_SCRIPT" | awk '{print $1}')
{
    printf '%s\n' '# STM32F767 CANopen reference build manifest'
    printf 'source_revision=%s\n' "$source_revision"
    printf 'source_dirty=%s\n' "$(git -C "$ROOT" status --porcelain | wc -l | tr -d ' ')"
    printf 'canopenstm32_revision=%s\n' "$canopenstm32_revision"
    printf 'vendor_tree_sha256=%s\n' "$vendor_tree_hash"
    printf 'arm_none_eabi_gcc=%s\n' "$(arm-none-eabi-gcc --version | sed -n '1p')"
    printf 'arm_none_eabi_ld=%s\n' "$(arm-none-eabi-ld --version | sed -n '1p')"
    printf 'cmake=%s\n' "$(cmake --version | sed -n '1p')"
    printf 'c_flags=%s\n' "${CMAKE_C_FLAGS:-}"
    printf 'toolchain_file=%s\n' "${CMAKE_TOOLCHAIN_FILE:-cmake/gcc-arm-none-eabi.cmake}"
} > "$OUTPUT"
cat > "$JSON_OUTPUT" <<EOF
{
  "schema": "stm32-canopen-build-manifest-v2",
  "source": {
    "revision": "$source_revision",
    "dirty": $source_dirty
  },
  "submodules": {
    "canopenstm32_revision": "$canopenstm32_revision",
    "stm32cubef7_revision": "$vendor_tree_hash"
  },
  "toolchain": {
    "arm_none_eabi_gcc": "$arm_gcc_version",
    "arm_none_eabi_ld": "$arm_ld_version",
    "cmake": "$cmake_version",
    "toolchain_file": "${CMAKE_TOOLCHAIN_FILE:-cmake/gcc-arm-none-eabi.cmake}"
  },
  "configuration": {
    "personality": "$PERSONALITY",
    "c_flags": "${CMAKE_C_FLAGS:-}",
    "linker_script": "$LINKER_SCRIPT"
  },
  "inputs": {
    "object_dictionary_sha256": "$od_sha256",
    "linker_script_sha256": "$linker_sha256"
  }
}
EOF
printf 'text_manifest=%s\njson_manifest=%s\n' "$OUTPUT" "$JSON_OUTPUT"
