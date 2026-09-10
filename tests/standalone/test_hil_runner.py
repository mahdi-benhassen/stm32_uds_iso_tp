#!/usr/bin/env python3
"""Pure-Python contracts for the HIL framing and response verdict helpers."""

from dataclasses import replace
from pathlib import Path
import importlib.util
import sys


RUNNER = Path(__file__).with_name("run_uds_iso_tp_hil.py")
SPEC = importlib.util.spec_from_file_location("hil_runner", RUNNER)
assert SPEC is not None and SPEC.loader is not None
hil = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = hil
SPEC.loader.exec_module(hil)


def case(name: str, can_fd: bool = False):
    return next(item for item in hil.inventory(can_fd) if item.name == name)


def main() -> int:
    assert case("tester_present").data == "023E00"
    assert case("read_data_by_identifier").data == "0322F190"
    assert case("isotp_ff_4095_boundary").expected_pci == 3
    assert case("isotp_canfd_sf_62_bytes", True).data.startswith("003E")

    positive = replace(case("tester_present"))
    assert hil.validate_response(positive, "payload", bytes.fromhex("7E00"), None)
    assert positive.verdict == "PASS"

    negative = replace(case("tester_present"))
    assert not hil.validate_response(negative, "payload", bytes.fromhex("7F3E11"), None)
    assert negative.verdict == "FAIL"

    silence = replace(case("isotp_wrong_can_id"))
    assert hil.validate_response(silence, "timeout", None, None)
    assert silence.verdict == "PASS"

    flow_control = replace(case("isotp_ff_4095_boundary"))
    assert hil.validate_response(flow_control, "pci", None, bytes.fromhex("300000"))
    assert flow_control.verdict == "PASS"
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
