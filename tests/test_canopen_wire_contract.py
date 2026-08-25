"""Host-independent CANopen frame and communication-state contract tests."""
from __future__ import annotations

import importlib.util
import sys
import traceback
from contextlib import contextmanager
from pathlib import Path
from typing import Iterator
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
HOST = ROOT / "tests" / "host"
sys.path.insert(0, str(HOST))
from can_socket import Frame  # noqa: E402

MODEL_PATH = ROOT / "middleware" / "canopen" / "examples" / "canopen_vcan_device.py"
SPEC = importlib.util.spec_from_file_location("canopen_vcan_device_under_test", MODEL_PATH)
assert SPEC is not None and SPEC.loader is not None
MODEL = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODEL)


NODE_ID = 0x0A


@contextmanager
def device_session(sent: list[tuple[int, bytes]]) -> Iterator[object]:
    """Create a model device while retaining the transport patch for its lifetime."""
    with patch.object(MODEL, "open_socket", return_value=object()), patch.object(
        MODEL,
        "send",
        side_effect=lambda _sock, arbitration_id, data: sent.append((arbitration_id, data)),
    ):
        yield MODEL.Device("fake", NODE_ID, 100, False)


def test_nmt_state_machine_and_bootup_contract() -> None:
    sent: list[tuple[int, bytes]] = []
    with device_session(sent) as device:
        sent.clear()

        device.handle(Frame(0x000, bytes((0x01, NODE_ID))))
        assert device.state == 0x05
        assert sent[-1] == (0x700 + NODE_ID, b"\x05")

        device.handle(Frame(0x000, bytes((0x02, NODE_ID))))
        assert device.state == 0x04
        assert sent[-1] == (0x700 + NODE_ID, b"\x04")

        device.handle(Frame(0x000, bytes((0x80, 0x00))))
        assert device.state == 0x7F
        assert sent[-1] == (0x700 + NODE_ID, b"\x7f")

        device.handle(Frame(0x000, bytes((0x81, NODE_ID))))
        assert device.state == 0x7F
        assert sent[-1] == (0x700 + NODE_ID, b"\x00")


def test_malformed_nmt_and_state_gated_sync_are_ignored() -> None:
    sent: list[tuple[int, bytes]] = []
    with device_session(sent) as device:
        sent.clear()

        device.handle(Frame(0x000, b"\x01"))
        assert device.state == 0x7F
        assert sent == []

        device.handle(Frame(0x080, b""))
        assert sent == []

        device.handle(Frame(0x000, bytes((0x01, NODE_ID))))
        sent.clear()
        device.handle(Frame(0x080, b""))
        assert sent == [(0x180 + NODE_ID, b"\x00\x00")]


def test_sdo_abort_contracts() -> None:
    sent: list[tuple[int, bytes]] = []
    with device_session(sent) as device:
        sent.clear()

        device.handle(Frame(0x600 + NODE_ID, bytes((0x40, 0x99, 0x99, 0, 0, 0, 0, 0))))
        assert sent[-1] == (
            0x580 + NODE_ID,
            bytes((0x80, 0x99, 0x99, 0)) + (0x06020000).to_bytes(4, "little"),
        )

        device.handle(Frame(0x600 + NODE_ID, bytes((0x23, 0x18, 0x10, 1, 0, 0, 0, 0))))
        assert sent[-1] == (
            0x580 + NODE_ID,
            bytes((0x80, 0x18, 0x10, 1)) + (0x06010002).to_bytes(4, "little"),
        )


def run_tests() -> int:
    """Run all contract functions when this file is invoked as a script."""
    tests = [
        (name, test)
        for name, test in sorted(globals().items())
        if name.startswith("test_") and callable(test)
    ]
    failures = 0
    for name, test in tests:
        try:
            test()
        except Exception:
            failures += 1
            print(f"FAIL: {name}", file=sys.stderr)
            traceback.print_exc()
        else:
            print(f"PASS: {name}")

    if failures:
        print(f"{failures} wire-contract test(s) failed", file=sys.stderr)
        return 1
    print(f"{len(tests)} wire-contract tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(run_tests())


__all__ = [
    "run_tests",
    "test_malformed_nmt_and_state_gated_sync_are_ignored",
    "test_nmt_state_machine_and_bootup_contract",
    "test_sdo_abort_contracts",
]

