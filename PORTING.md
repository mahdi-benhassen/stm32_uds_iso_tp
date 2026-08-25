# CubeMX branch — CANopen integration notes

This branch carries the STM32CubeMX-generated project for the STM32F767
(`stm32f767_canopen.ioc`). The CANopen stack and application layers from the
`main` branch are ported here **without modifying any generated function**:
all firmware integration lives in `USER CODE` sections, in new project-owned
files, and in the designated user areas of `CMakeLists.txt`.

## Added layers

| Layer | Location | Origin |
|---|---|---|
| CANopenNode + STM32 binding | `third_party/CanOpenSTM32` (submodule, pinned `b313b2b`) | upstream |
| Runtime wrapper, profiles, storage, watchdog, recovery, diagnostics, LSS policy, gateway seam, timing | `App/Src`, `App/Inc` | ported from `main` |
| CiA 302 NMT-master helper, acceptance-filter policy | `middleware/canopen/core` | ported from `main` |
| Generated Object Dictionary (CiA 401 default personality) | `Generated/OD.c/.h` | ported from `main` |

## Generated-code interaction rules

- `Core/**`, `Drivers/**`, startup, linker script: untouched.
- `Core/Src/main.c`: only `USER CODE` sections filled (includes, PV, PD,
  callback in section 0, board/timing/watchdog init in `Init`, app start in
  section 2, mainline loop in `WHILE`, fault recording inside
  `Error_Handler_Debug`).
- Root `CMakeLists.txt`: extended only in the marked user areas; HAL/CMSIS
  remain inside the generated `STM32_Drivers` library.

## Deliberate peripheral fix-ups (`canopen_reference_port_fixup.c`)

The generated `MX_CAN1_Init` cannot express two protocol requirements, so an
application-layer fix-up re-initializes CAN1 through HAL before the runtime
starts:

| Item | Generated value | Applied value | Why |
|---|---|---|---|
| Auto retransmission | DISABLE | ENABLE | CiA 301 mandates frame retransmission on error; disabled bxCAN silently drops frames |
| Sample point @500 kbit/s | 12/18 tq = 66.7 % | 15/18 tq = 83.3 % | matches reference timing table and typical CiA network expectations |
| Auto bus-off | DISABLE | kept DISABLE | bus-off recovery is owned by the bounded software recovery state machine |
| TIM7 NVIC priority | (0,0) — equal to CAN RX | demoted to (1,0) | 1 ms dispatch must not preempt CAN reception |

TIM7 keeps the generated 1 ms cadence (108 MHz / 108 / 1000).
