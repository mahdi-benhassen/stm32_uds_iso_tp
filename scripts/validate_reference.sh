#!/usr/bin/env sh
# SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
# Reproducible local validation for the STM32F767 CANopen reference.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CUBE_DIR=${STM32_CUBE_F7_DIR:-"$ROOT/third_party/STM32CubeF7"}
LINKER_SCRIPT=${STM32_F7_LINKER_SCRIPT:-"$ROOT/linker/STM32F767_2M_512K_FLASH.ld"}
BUILD_DIR=${BUILD_DIR:-"$ROOT/build/firmware"}
CIA402_BUILD_DIR=${CIA402_BUILD_DIR:-"$ROOT/build/firmware-cia402"}
GATEWAY_BUILD_DIR=${GATEWAY_BUILD_DIR:-"$ROOT/build/firmware-gateway"}
CIA418_BUILD_DIR=${CIA418_BUILD_DIR:-"$ROOT/build/firmware-cia418"}
INVENTUS_BUILD_DIR=${INVENTUS_BUILD_DIR:-"$ROOT/build/firmware-inventus-battery"}

command -v python3 >/dev/null
command -v gcc >/dev/null
command -v cmake >/dev/null
command -v arm-none-eabi-gcc >/dev/null
command -v arm-none-eabi-size >/dev/null

python3 "$ROOT/scripts/validate_od.py"
python3 "$ROOT/scripts/validate_inventus_battery.py"
PYTHONDONTWRITEBYTECODE=1 python3 "$ROOT/tests/test_firmware_configuration.py"
PYTHONDONTWRITEBYTECODE=1 python3 "$ROOT/scripts/mock_canopen_runner.py"
PYTHONDONTWRITEBYTECODE=1 python3 "$ROOT/tests/test_documentation_index.py"
mkdir -p "$ROOT/build/tests"

gcc -std=c11 -Wall -Wextra -Werror \
    -DCANOPEN_REFERENCE_ENABLE_CIA401=1 \
    -DCANOPEN_REFERENCE_ENABLE_CIA402=1 \
    -DCANOPEN_REFERENCE_ALLOW_COMBINED_PROFILES=1 \
    -I"$ROOT/tests/fakes" -I"$ROOT/App/Inc" \
    "$ROOT/tests/test_profiles.c" \
    "$ROOT/App/Src/cia401_reference.c" \
    "$ROOT/App/Src/cia402_reference.c" \
    -o "$ROOT/build/tests/test_profiles"
"$ROOT/build/tests/test_profiles"
make -C "$ROOT/tests/host" all test-stm32-facade test-gateway-default-deny test-inventus-battery test-inventus-battery-data test-protocol-contract test-acceptance-filter test-sanitize test-coverage
PYTHONDONTWRITEBYTECODE=1 python3 "$ROOT/tests/test_canopen_wire_contract.py"
PYTHONDONTWRITEBYTECODE=1 python3 "$ROOT/tests/conformance/run_core_vectors.py"
PYTHONPATH="$ROOT:$ROOT/tests" PYTHONDONTWRITEBYTECODE=1 python3 "$ROOT/tests/run_uds_isotp_contract.py"
PYTHONPATH="$ROOT:$ROOT/tests" PYTHONDONTWRITEBYTECODE=1 python3 "$ROOT/tests/run_nmea2000_gateway_contract.py"

cmake -S "$ROOT" -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/arm-none-eabi-gcc.cmake" \
    -DSTM32_CUBE_F7_DIR="$CUBE_DIR" \
    -DSTM32_F7_LINKER_SCRIPT="$LINKER_SCRIPT"
cmake --build "$BUILD_DIR" --parallel 2
arm-none-eabi-size "$BUILD_DIR/stm32f767_canopen_reference"
test -s "$BUILD_DIR/stm32f767_canopen_reference.hex"
test -s "$BUILD_DIR/stm32f767_canopen_reference.bin"
CANOPEN_REFERENCE_PERSONALITY=default "$ROOT/scripts/write_build_manifest.sh" "$BUILD_DIR/build-manifest.txt" "$CUBE_DIR"
PYTHONDONTWRITEBYTECODE=1 python3 "$ROOT/scripts/validate_build_manifest.py" "$BUILD_DIR/build-manifest.json"

# Compile optional personalities independently. The reference avoids a default
# combined 401/402 product personality because those profiles have different
# product conformance obligations.
cmake -S "$ROOT" -B "$CIA402_BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/arm-none-eabi-gcc.cmake" \
    -DSTM32_CUBE_F7_DIR="$CUBE_DIR" \
    -DSTM32_F7_LINKER_SCRIPT="$LINKER_SCRIPT" \
    -DCMAKE_C_FLAGS="-DCANOPEN_REFERENCE_ENABLE_CIA401=0 -DCANOPEN_REFERENCE_ENABLE_CIA402=1"
cmake --build "$CIA402_BUILD_DIR" --parallel 2
arm-none-eabi-size "$CIA402_BUILD_DIR/stm32f767_canopen_reference"
test -s "$CIA402_BUILD_DIR/stm32f767_canopen_reference.hex"
test -s "$CIA402_BUILD_DIR/stm32f767_canopen_reference.bin"

cmake -S "$ROOT" -B "$GATEWAY_BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/arm-none-eabi-gcc.cmake" \
    -DSTM32_CUBE_F7_DIR="$CUBE_DIR" \
    -DSTM32_F7_LINKER_SCRIPT="$LINKER_SCRIPT" \
    -DCMAKE_C_FLAGS="-DCANOPEN_REFERENCE_ENABLE_GATEWAY=1"
cmake --build "$GATEWAY_BUILD_DIR" --parallel 2
arm-none-eabi-size "$GATEWAY_BUILD_DIR/stm32f767_canopen_reference"
test -s "$GATEWAY_BUILD_DIR/stm32f767_canopen_reference.hex"
test -s "$GATEWAY_BUILD_DIR/stm32f767_canopen_reference.bin"

cmake -S "$ROOT" -B "$CIA418_BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/arm-none-eabi-gcc.cmake" \
    -DSTM32_CUBE_F7_DIR="$CUBE_DIR" \
    -DSTM32_F7_LINKER_SCRIPT="$LINKER_SCRIPT" \
    -DCANOPEN_REFERENCE_ENABLE_CIA418=ON
cmake --build "$CIA418_BUILD_DIR" --parallel 2
arm-none-eabi-size "$CIA418_BUILD_DIR/stm32f767_canopen_reference"
test -s "$CIA418_BUILD_DIR/stm32f767_canopen_reference.hex"
test -s "$CIA418_BUILD_DIR/stm32f767_canopen_reference.bin"

cmake -S "$ROOT" -B "$INVENTUS_BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/arm-none-eabi-gcc.cmake" \
    -DSTM32_CUBE_F7_DIR="$CUBE_DIR" \
    -DSTM32_F7_LINKER_SCRIPT="$LINKER_SCRIPT" \
    -DCANOPEN_REFERENCE_ENABLE_INVENTUS_BATTERY=ON
cmake --build "$INVENTUS_BUILD_DIR" --parallel 2
arm-none-eabi-size "$INVENTUS_BUILD_DIR/stm32f767_canopen_reference"
test -s "$INVENTUS_BUILD_DIR/stm32f767_canopen_reference.hex"
test -s "$INVENTUS_BUILD_DIR/stm32f767_canopen_reference.bin"

printf '%s\n' 'Reference validation completed successfully.'
