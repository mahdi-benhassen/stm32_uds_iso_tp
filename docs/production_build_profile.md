# Production build profile

The CI STM32F767 personalities are built with `CANOPEN_REFERENCE_PRODUCTION_PROFILE=ON`. The option is deliberately separate from the default development configuration so debug and CubeMX iteration remain convenient while release candidates exercise the production compiler contract.

The profile adds `-O2`, `-fstack-protector-strong`, `-fno-common`, `-Wformat=2`, `-Wconversion`, `-Wshadow`, `-Wundef`, and `-Werror`, and enables fatal linker warnings. The profile does not claim that stack depth, worst-case execution time, interrupt latency, CAN timing, or safety behavior have been measured; those remain target-board and HIL evidence gates.

The target memory gate uses the linker map for the exact STM32F767 reference linker script. The current CI budgets retain approximately 20% headroom against the configured application Flash and RAM capacities:

| Resource | CI budget | Purpose |
|---|---:|---|
| `.text` | 1,228,800 bytes | Application Flash headroom |
| Flash load | 1,228,800 bytes | Total initialized Flash load headroom |
| `.data` | 262,144 bytes | Initialized RAM budget |
| `.bss` | 262,144 bytes | Zero-initialized RAM budget |
| Total RAM load | 409,600 bytes | Combined initialized and zero-initialized RAM headroom |

These byte limits are release-budget checks, not proof of production resource margin. Stack watermarking, ISR latency, main-loop worst-case timing, CAN queue occupancy, CPU utilization, voltage/temperature corners, and long-duration stress measurements require the exact board and released hardware configuration.

## Local validation

```sh
cmake -S . -B build/ci-firmware \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
  -DSTM32_CUBE_F7_DIR="$PWD/third_party/STM32CubeF7" \
  -DSTM32_F7_LINKER_SCRIPT="$PWD/linker/STM32F767_2M_512K_FLASH.ld" \
  -DCANOPEN_REFERENCE_PRODUCTION_PROFILE=ON
cmake --build build/ci-firmware --parallel 2
```
