#!/usr/bin/env python3
"""Validate deterministic core CANopen regression vectors.

This runner provides software-only regression evidence. It does not claim
physical CAN, transceiver, timing, HIL, or official CANopen conformance.
"""
from __future__ import annotations

import argparse
import json
from collections import Counter
from pathlib import Path


REQUIRED_CATEGORIES = {
    "nmt", "heartbeat", "emcy", "sdo", "pdo", "pdo_mapping", "sync",
    "time", "lss", "reset", "invalid_od", "timeout", "busoff", "recovery",
}


def fail(message: str) -> None:
    raise AssertionError(message)


def frame(vector: dict) -> tuple[int, bytes, dict]:
    request = vector["request"]
    expected = vector["expected"]
    if "cob_id" not in request or "data" not in request:
        fail(f"{vector['name']}: frame vector requires cob_id and data")
    cob_id = int(request["cob_id"])
    payload_hex = request["data"]
    if not 0 <= cob_id <= 0x7FF:
        fail(f"{vector['name']}: COB-ID out of range")
    if len(payload_hex) % 2:
        fail(f"{vector['name']}: payload has odd hex length")
    try:
        payload = bytes.fromhex(payload_hex)
    except ValueError as exc:
        fail(f"{vector['name']}: invalid payload hex: {exc}")
    if len(payload) > 8:
        fail(f"{vector['name']}: classic CAN payload exceeds 8 bytes")
    return cob_id, payload, expected


def validate_sdo(vector: dict, node_id: int, cob_id: int, payload: bytes, expected: dict) -> None:
    service = expected["service"]
    if service in {"sdo_upload_request", "sdo_upload_segment_request", "sdo_download_request", "sdo_timeout"}:
        if cob_id != 0x600 + node_id or len(payload) != 8:
            fail(f"{vector['name']}: invalid SDO client request frame")
    elif service == "sdo_abort":
        if cob_id != 0x580 + node_id or len(payload) != 8 or payload[0] != 0x80:
            fail(f"{vector['name']}: invalid SDO abort response frame")
        actual_abort = int.from_bytes(payload[4:8], "little")
        if actual_abort != expected["abort_code"]:
            fail(f"{vector['name']}: SDO abort code mismatch")
        return
    else:
        return

    command = payload[0]
    if service == "sdo_upload_request":
        if command != 0x40:
            fail(f"{vector['name']}: expected expedited/segmented upload initiate")
        transfer = "expedited" if expected.get("transfer") == "expedited" else "segmented"
        if expected.get("transfer") != transfer:
            fail(f"{vector['name']}: invalid upload transfer metadata")
    elif service == "sdo_upload_segment_request":
        if command != 0x60 or expected.get("transfer") != "segmented":
            fail(f"{vector['name']}: invalid segmented upload request")
    elif service == "sdo_download_request":
        if command & 0xE0 != 0x20:
            fail(f"{vector['name']}: invalid download initiate command")
        expedited = bool(command & 0x02)
        if expected.get("transfer") == "expedited" and not expedited:
            fail(f"{vector['name']}: expected expedited download")
        if expected.get("transfer") == "segmented" and expedited:
            fail(f"{vector['name']}: expected segmented download")
        if expedited:
            size = 4 - ((command >> 2) & 0x03) if command & 0x01 else None
            if size is None or expected.get("size") != size:
                fail(f"{vector['name']}: expedited download size mismatch")
    elif service == "sdo_timeout":
        if command != 0x40 or expected.get("timeout_ms") != 1000:
            fail(f"{vector['name']}: invalid timeout stimulus")
        return

    if "index" in expected and int.from_bytes(payload[1:3], "little") != expected["index"]:
        fail(f"{vector['name']}: SDO index mismatch")
    if "subindex" in expected and payload[3] != expected["subindex"]:
        fail(f"{vector['name']}: SDO subindex mismatch")


def validate_vector(node_id: int, vector: dict) -> None:
    category = vector.get("category")
    expected = vector.get("expected", {})
    if not isinstance(category, str) or not category:
        fail(f"{vector.get('name', '<unnamed>')}: category is required")

    request = vector.get("request", {})
    if "event" in request:
        event = request["event"]
        if category in {"busoff", "recovery", "reset"}:
            if event not in {"bus_off", "recovery_retry", "recovery_complete", "recovery_exhausted", "reset_requested"}:
                fail(f"{vector['name']}: unknown recovery/reset event")
            if expected.get("service") != "can_recovery":
                fail(f"{vector['name']}: recovery/reset event must expect can_recovery")
            if expected.get("state") not in {"reset_requested", "running", "safe_fault"}:
                fail(f"{vector['name']}: invalid recovery state")
            return
        if category == "can_error":
            allowed = {"warning", "passive", "rx_overflow", "tx_failure", "ack", "stuff", "form", "bit", "crc", "error_warning_transition", "recovered"}
            if event not in allowed or expected.get("service") != "can_error" or expected.get("error") != event:
                fail(f"{vector['name']}: invalid CAN controller error vector")
            return
        fail(f"{vector['name']}: event request is only valid for recovery, reset, or CAN-error categories")
        

    cob_id, payload, expected = frame(vector)
    service = expected.get("service")

    if service == "nmt" and expected.get("command") != "invalid":
        commands = {1: "start", 2: "stop", 0x80: "pre_operational", 0x81: "reset_node", 0x82: "reset_communication"}
        if cob_id != 0 or len(payload) != 2 or payload[1] not in (0, node_id):
            fail(f"{vector['name']}: invalid NMT frame")
        if commands.get(payload[0]) != expected.get("command"):
            fail(f"{vector['name']}: NMT command mismatch")
    elif category == "nmt_invalid":
        if cob_id != 0 or len(payload) != 2 or payload[1] not in (0, node_id) or payload[0] in {1, 2, 0x80, 0x81, 0x82}:
            fail(f"{vector['name']}: invalid NMT stimulus is not invalid")
        if expected.get("service") != "nmt" or expected.get("command") != "invalid":
            fail(f"{vector['name']}: invalid NMT vector metadata mismatch")
    elif service == "heartbeat":
        states = {0: "bootup", 4: "stopped", 5: "operational", 0x7F: "pre_operational"}
        if cob_id != 0x700 + node_id or len(payload) != 1 or states.get(payload[0]) != expected.get("nmt_state"):
            fail(f"{vector['name']}: invalid heartbeat vector")
    elif service == "emcy":
        if cob_id != 0x80 + node_id or len(payload) != 8:
            fail(f"{vector['name']}: invalid EMCY vector")
        if int.from_bytes(payload[:2], "little") != expected.get("error_code"):
            fail(f"{vector['name']}: EMCY code mismatch")
    elif service in {"sdo_upload_request", "sdo_upload_segment_request", "sdo_download_request", "sdo_abort", "sdo_timeout"}:
        validate_sdo(vector, node_id, cob_id, payload, expected)
    elif service in {"sdo_abort_expected"}:
        if cob_id != 0x600 + node_id or len(payload) != 8:
            fail(f"{vector['name']}: invalid SDO abort stimulus frame")
        if expected.get("index") != int.from_bytes(payload[1:3], "little") or expected.get("subindex") != payload[3]:
            fail(f"{vector['name']}: SDO abort stimulus object mismatch")
        if expected.get("abort_code") is None:
            fail(f"{vector['name']}: SDO abort stimulus missing expected code")
    elif service in {"tpdo1", "tpdo2", "tpdo3", "tpdo4", "rpdo1", "rpdo2", "rpdo3", "rpdo4"}:
        pdo_bases = {"tpdo1": 0x180, "tpdo2": 0x280, "tpdo3": 0x380, "tpdo4": 0x480, "rpdo1": 0x200, "rpdo2": 0x300, "rpdo3": 0x400, "rpdo4": 0x500}
        if cob_id != pdo_bases[service] + node_id or not 0 <= len(payload) <= 8:
            fail(f"{vector['name']}: invalid {service} frame")
        if expected.get("node_id", node_id) != node_id:
            fail(f"{vector['name']}: PDO node mismatch")
    elif service == "sync":
        if cob_id != 0x80 or len(payload) not in (0, 1):
            fail(f"{vector['name']}: invalid SYNC vector")
        if expected.get("counter") != (payload[0] if payload else None):
            fail(f"{vector['name']}: SYNC counter mismatch")
    elif service == "time":
        if cob_id != 0x100 or len(payload) != expected.get("payload_length"):
            fail(f"{vector['name']}: invalid TIME vector")
    elif service in {"lss", "lss_invalid_sequence"}:
        if cob_id != 0x7E4 or len(payload) != 8:
            fail(f"{vector['name']}: invalid LSS frame")
        commands = {
            0x04: "switch_state_global", 0x11: "configure_node_id", 0x13: "configure_bitrate",
            0x15: "activate_bitrate", 0x17: "store_configuration", 0x40: "switch_state_selective",
            0x5A: "inquire_vendor_id", 0x5B: "inquire_product_code", 0x5E: "inquire_node_id",
        }
        if commands.get(payload[0]) != expected.get("command"):
            fail(f"{vector['name']}: LSS command mismatch")
        if service == "lss_invalid_sequence" and not expected.get("invalid_sequence"):
            fail(f"{vector['name']}: LSS invalid-sequence marker missing")
    elif category == "invalid_od":
        if service != "sdo_download_request" or int.from_bytes(payload[1:3], "little") != 0xFFFF or not expected.get("invalid"):
            fail(f"{vector['name']}: invalid-OD vector is not an invalid SDO access")
    elif service == "time_invalid":
        if cob_id != 0x100 or len(payload) != expected.get("payload_length"):
            fail(f"{vector['name']}: invalid TIME vector shape mismatch")
    else:
        fail(f"{vector['name']}: unsupported service {service!r}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("path", nargs="?", type=Path, default=Path(__file__).with_name("core_vectors.json"))
    args = parser.parse_args()
    document = json.loads(args.path.read_text(encoding="utf-8"))
    if document.get("schema") != "canopen-reference-core-vector-v2":
        fail("unsupported vector schema")
    node_id = int(document["node_id"])
    if not 1 <= node_id <= 127:
        fail("node_id must be in the CANopen range 1..127")
    vectors = document.get("vectors")
    if not isinstance(vectors, list) or len(vectors) < 100:
        fail("vectors must contain at least 100 cases")
    names = [vector.get("name") for vector in vectors]
    if len(names) != len(set(names)):
        fail("vector names must be unique")
    counts = Counter(vector.get("category") for vector in vectors)
    missing = REQUIRED_CATEGORIES - set(counts)
    if missing:
        fail(f"missing required categories: {sorted(missing)}")
    for vector in vectors:
        validate_vector(node_id, vector)
    category_summary = ", ".join(f"{name}={counts[name]}" for name in sorted(counts))
    print(f"validated {len(vectors)} core CANopen vectors (software contract only)")
    print(f"categories: {category_summary}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
