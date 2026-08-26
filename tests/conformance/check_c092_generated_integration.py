#!/usr/bin/env python3
"""Check a generated STM32C092 project against the maintained adapter contract."""

from pathlib import Path
import argparse


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("project", type=Path, help="generated STM32C092 project root")
    args = parser.parse_args()
    root = args.project.resolve()
    errors = []
    warnings = []

    def read(relative: str) -> str:
        path = root / relative
        if not path.exists():
            errors.append(f"missing file: {relative}")
            return ""
        return path.read_text(encoding="utf-8")

    main_c = read("Src/main.c")
    fdcan_c = read("Src/fdcan.c")
    app_c = read("stm32c092/uds_app_fdcan.c")
    transport_c = read("stm32c092/can_transport_fdcan.c")
    diagnostics_h = read("stm32c092/uds_diagnostics.h")

    required = (
        (main_c, "uds_c092_fdcan_transport_init", "transport initialization"),
        (main_c, "uds_c092_app_init_default", "application initialization"),
        (fdcan_c, "TxFifoQueueMode", "generated TX FIFO configuration"),
        (transport_c, "FDCAN_STORE_TX_EVENTS", "stored TX-event headers"),
        (app_c, "uds_c092_fdcan_poll_tx_events", "mainline TX-event polling"),
        (diagnostics_h, "UDS_C092_DIAG_BOOTING", "BOOTING state"),
        (diagnostics_h, "UDS_C092_DIAG_READY", "READY state"),
        (diagnostics_h, "UDS_C092_DIAG_FAULT", "FAULT state"),
        (main_c, "uds_c092_diagnostic_init", "diagnostic initialization"),
        (main_c, "uds_c092_app_attach_diagnostics", "diagnostic attachment"),
        (main_c, "UDS_C092_BOOT_DIAGNOSTIC_READY", "readiness boundary"),
    )
    for text, token, label in required:
        if token not in text:
            errors.append(f"missing {label}: {token}")

    if "HAL_FDCAN_TxEventFifoCallback" not in main_c or "uds_c092_fdcan_on_tx_event" not in main_c:
        warnings.append("optional TX-event IRQ forwarding is absent; mainline polling must remain active")
    if "FDCAN_IT_TX_EVT_FIFO_NEW_DATA" not in main_c:
        warnings.append("optional TX-event IRQ notification is absent; mainline polling must remain active")
    if "FDCAN_FILTER_RANGE" in main_c:
        warnings.append("broad range filter remains; use exact physical/functional IDs")
    if "(uint8_t)rxHeader.DataLength" in main_c:
        warnings.append("RX path passes raw HAL DataLength; use uds_c092_fdcan_data_length_bytes()")
    if "uds_c092_app_rx_from_isr(rxHeader.Identifier" in main_c:
        warnings.append("RX path uses compatibility wrapper; use _ex() to preserve frame metadata")
    if "FDCAN_NO_TX_EVENTS" in main_c:
        warnings.append("unused generated TX header still says FDCAN_NO_TX_EVENTS; adapter headers must store events")

    if errors:
        print(f"C092 generated integration contract FAILED: {root}")
        for error in errors:
            print(f"- ERROR: {error}")
        for warning in warnings:
            print(f"- WARNING: {warning}")
        return 1
    print(f"C092 generated integration contract OK: {root}")
    for warning in warnings:
        print(f"- WARNING: {warning}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
