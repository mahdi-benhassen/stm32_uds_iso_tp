# CANopen Middleware Integration Layer

This directory supplies the project-facing integration surface requested for the CANopen milestone. It does **not** implement a second CANopen stack. The production protocol owner remains the pinned CANopenNode source under `third_party/CanOpenSTM32/CANopenNode`.[1]

| Path | Purpose | Build ownership |
|---|---|---|
| `core/` | Stable lifecycle facade over the native CANopenNode STM32 runtime. | Compiled into the Cortex-M7 firmware. |
| `port/can_port.c` | Optional STM32 HAL classic-CAN facade for exclusive-owner diagnostics or standalone clients. | Object-only target; not linked beside CANopenNode’s bxCAN driver. |
| `port/vcan_port.c` | SocketCAN implementation of the same transport contract. | Compiled by `tests/host/Makefile`. |
| `od/imported/` | Staged objdictgen C/H/EDS artifacts. | Not active until explicitly imported with `--activate`. |
| `examples/` | Host device and diagnostic examples used by integration tests. | Host-only. |

## API and ownership

Application code may call `canopen_core_init()`, `canopen_core_process()`, and `canopen_core_process_cycle()`. The facade delegates to the native CANopenNode lifecycle; it intentionally does not provide the unrelated `CO_Data`/`CO_init()` interface.

`can_port.h` provides `can_port_init()`, `can_port_send()`, `can_port_register_rx()`, polling, and deinitialization. On STM32, the CANopenNode driver owns bxCAN lifecycle and HAL callbacks in the production image. Therefore, `can_port.c` is for an exclusive-owner test/diagnostic build and must **not** be linked concurrently with `CO_driver_STM32.c` for the same CAN controller. The SocketCAN port has no such conflict and is used by host tests.

## Object Dictionary import

Run the following command to stage output created by `objdictgen`:

```sh
tools/import_objdict.sh /path/to/objdictgen-output --stage
tools/import_objdict.sh /path/to/objdictgen-output.zip --activate
```

The importer accepts one `OD.c`/`OD.h` pair and optionally one EDS/DCF file. It rejects unsafe archive paths, stages the exact input under `od/imported/`, records hashes, and invokes the generic OD structural validator. Activation is explicit because successful C/H import does not prove EDS semantics, PDO mapping, profile compliance, or target compilation.

## References

[1]: https://github.com/CANopenNode/CANopenNode "CANopenNode source repository"
