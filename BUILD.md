# Build and Hardware Bring-Up

This guide describes the supported build paths for the STM32F767 CANopen reference firmware. The repository is built with CMake and GNU Arm Embedded GCC. STM32CubeF7 is supplied separately and is never fetched implicitly by the firmware build.

## Supported reference target

The project targets an STM32F767 device using the STM32 HAL and CAN1/bxCAN. The reference hardware contract is:

| Item | Reference value |
|---|---|
| MCU family | STM32F767; verify the exact package and memory map before flashing |
| CAN pins | PI9 = CAN1_RX, PA12 = CAN1_TX, AF9 (from `stm32f767_canopen.ioc`) |
| CAN rate | 500 kbit/s nominal |
| Default node-ID | 10 |
| Default heartbeat producer | 1,000 ms |
| Timer cadence | TIM7 at 1 ms |
| External clock assumption | 25 MHz HSE |
| Default application | CiA 401 I/O reference |

The STM32F767 does not contain a CAN transceiver. Add a compatible external transceiver, common ground, bus protection, connector, and two 120 Ω end-of-bus terminations. Transceiver standby and enable behavior is board-specific.

## Toolchain

The GitHub Actions build currently validates against the following Ubuntu package versions:

| Tool | CI reference |
|---|---|
| Ubuntu runner | `ubuntu-24.04` |
| CMake | `3.28.3-1build7` |
| GNU Arm Embedded GCC | `15:13.2.rel1-2` |
| Newlib | `4.4.0.20231231-2` |
| cppcheck | `2.13.0-2ubuntu3` |
| STM32CubeF7 | revision `c2ecfd2d863d4cb1a138e63be4c8c1c4acd43d4d` |

A compatible recent version of the ARM GNU toolchain is normally sufficient for local development, but release artifacts should record the compiler version, linker script, submodule revisions, and STM32CubeF7 revision.

## Clone and dependencies

```sh
git clone --recurse-submodules https://github.com/mahdi-benhassen/stm32_canopen_reference.git
cd stm32_canopen_reference
git submodule update --init --recursive
```

Obtain a controlled STM32CubeF7 package from STMicroelectronics and set its path:

```sh
export STM32_CUBE_F7_DIR=/opt/STM32CubeF7
```

The repository’s CMake build expects the CMSIS and STM32F7 HAL directories below that path. Do not commit the Cube package into the repository; the local checkout is ignored by `.gitignore`.

## Firmware build

The reference linker script assumes an STM32F767 memory map with 2 MiB flash and 512 KiB SRAM. Replace it when the exact target package or board differs.

```sh
cmake -S . -B build/f767 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
  -DSTM32_CUBE_F7_DIR="$STM32_CUBE_F7_DIR" \
  -DSTM32_F7_LINKER_SCRIPT="$PWD/linker/STM32F767_2M_512K_FLASH.ld" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/f767 --parallel
arm-none-eabi-size build/f767/stm32f767_canopen_reference
```

The output directory contains the ELF image and post-build HEX, BIN, and MAP artifacts. Retain the MAP file with release evidence.

### UDS profile

UDS is enabled by default in the current reference build. This branch reuses the generated CAN1 handle, GPIO, NVIC, and TIM7 infrastructure. The profile uses a separate FIFO1 filter/callback and a bounded bare-metal main-loop budget. To disable it deliberately, configure `-DCANOPEN_REFERENCE_ENABLE_UDS=OFF`. It does not provide a production bootloader or production SecurityAccess provider.

```sh
cmake -S . -B build/f767-uds -DCMAKE_BUILD_TYPE=Release \
  -DCANOPEN_REFERENCE_ENABLE_UDS=ON
cmake --build build/f767-uds --parallel
arm-none-eabi-size build/f767-uds/stm32f767_canopen.elf
```

Review [`docs/uds/configuration.md`](docs/uds/configuration.md), [`docs/uds/can_ids.md`](docs/uds/can_ids.md), [`docs/uds/stm32f767.md`](docs/uds/stm32f767.md), and [`docs/uds/cubemx_integration.md`](docs/uds/cubemx_integration.md) before using the enabled profile on a board. The generated `.ioc` assigns CAN1 RX to PI9 and TX to PA12; reconcile this with the actual schematic.

## Build personalities

The default image enables CiA 401 and leaves optional personalities disabled. Build each product personality in a clean build directory.

```sh
# CiA 402 reference personality
cmake -S . -B build/cia402 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
  -DSTM32_CUBE_F7_DIR="$STM32_CUBE_F7_DIR" \
  -DSTM32_F7_LINKER_SCRIPT="$PWD/linker/STM32F767_2M_512K_FLASH.ld" \
  -DCMAKE_C_FLAGS="-DCANOPEN_REFERENCE_ENABLE_CIA401=0 -DCANOPEN_REFERENCE_ENABLE_CIA402=1"
cmake --build build/cia402 --parallel

# CiA 302 NMT-master personality
cmake -S . -B build/cia302 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
  -DSTM32_CUBE_F7_DIR="$STM32_CUBE_F7_DIR" \
  -DSTM32_F7_LINKER_SCRIPT="$PWD/linker/STM32F767_2M_512K_FLASH.ld" \
  -DCANOPEN_REFERENCE_ENABLE_CIA302_MASTER=ON
cmake --build build/cia302 --parallel

# Optional CiA 309 gateway foundation
cmake -S . -B build/gateway \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
  -DSTM32_CUBE_F7_DIR="$STM32_CUBE_F7_DIR" \
  -DSTM32_F7_LINKER_SCRIPT="$PWD/linker/STM32F767_2M_512K_FLASH.ld" \
  -DCMAKE_C_FLAGS="-DCANOPEN_REFERENCE_ENABLE_GATEWAY=1"
cmake --build build/gateway --parallel
```

The CiA 302 master and gateway are opt-in. Do not enable them in a product image without a matching Object Dictionary, authorization policy, peer-node plan, and hardware acceptance evidence.

## Host validation and virtual CAN

Run deterministic checks before using target hardware:

```sh
python3 scripts/validate_od.py
python3 scripts/validate_cia418.py
python3 tests/test_firmware_configuration.py
python3 tests/test_canopen_wire_contract.py
python3 tests/run_uds_isotp_contract.py
python3 tests/run_nmea2000_gateway_contract.py
make -C tests/host all test-stm32-facade test-gateway-default-deny
make -C tests/host test-uds
```

On a Linux host with the `vcan` kernel module, create the virtual CAN interface and run the host protocol suite:

```sh
sudo ./scripts/setup_vcan.sh
make -C tests/host test
```

The hardware acceptance runner is separate from the virtual-CAN suite. Use [`tests/hardware/README.md`](tests/hardware/README.md) and [`docs/uds/hil_testing.md`](docs/uds/hil_testing.md) for the physical SocketCAN procedure. It is non-destructive by default; reset, Flash, malformed-frame, and active CANopen traffic gates require explicit operator flags. Host and fake-HAL tests do not constitute target timing, bus-error, watchdog, electrical, or Flash power-loss evidence.

## Flashing with ST-LINK

The repository does not assume a particular board or ST-LINK installation. Before flashing, confirm the exact MCU package, memory map, reset behavior, and external transceiver wiring.

Using STM32CubeProgrammer:

```sh
STM32_Programmer_CLI -c port=SWD -w \
  build/f767/stm32f767_canopen_reference.hex \
  -v -rst
```

Using OpenOCD with an ST-LINK adapter, select the interface and target configuration appropriate for the exact board:

```sh
openocd -f interface/stlink.cfg -f target/stm32f7x.cfg \
  -c "program build/f767/stm32f767_canopen_reference.elf verify reset exit"
```

Do not use these commands until the target configuration has been confirmed. An incorrect linker script, target file, voltage level, or board connection can corrupt the image or damage hardware.

## Expected first boot

With the default firmware and a correctly wired CAN transceiver, the node should initialize in CANopen pre-operational state and publish a boot-up heartbeat on `0x700 + node_id`, which is `0x70A` for the default node-ID 10. The default heartbeat state is `0x7F` after initialization. No application output should be energized by default.

A CAN analyzer can be used to verify the boot-up frame, heartbeat timing, NMT transitions, and error frames. The expected physical behavior depends on the board-level transceiver and application hooks; the reference repository does not assign LEDs, outputs, or a universal connector pinout.

## CubeMX and CubeIDE boundary

The generated `Core/` and `Drivers/` files remain authoritative. Do not replace `MX_CAN1_Init`, generated GPIO/MSP/NVIC code, or TIM7 with a parallel platform implementation. The only generated-file change required by the UDS callback path is enabling `USE_HAL_CAN_REGISTER_CALLBACKS`; regeneration must be followed by a diff review and a target build.



CubeMX-generated clock, GPIO, CAN, timer, startup, and interrupt files belong under `Core/`. Project-owned runtime and application code belongs under `App/` and `middleware/`. Compile the project-owned `App/Src/CO_app_STM32_reference.c` instead of the upstream CANopenNode STM32 application wrapper; linking both runtime wrappers causes duplicate symbols and conflicting ownership.

The project-owned compilation definitions are:

```text
STM32F767xx
USE_HAL_DRIVER
CO_DRIVER_CUSTOM
CO_USE_GLOBALS
```

Regenerate CubeMX files only after preserving these boundaries and reviewing the resulting clock, CAN, TIM7, NVIC, and MSP changes.
