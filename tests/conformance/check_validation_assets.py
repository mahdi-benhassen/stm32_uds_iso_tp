#!/ usr / bin / env python3
"""Validate standards and physical-validation artifacts for repository CI."""
from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MATRIX = ROOT / "docs/conformance/iso15765_iso14229_matrix.md"
PROFILE = ROOT / "docs/physical_validation/board_profile.yaml"
PLAN = ROOT / "tests/physical/hil_test_plan.json"
VECTORS = ROOT / "tests/conformance/conformance_vectors.json"
C092_DIR = ROOT / "examples/stm32c092"


def main() -> int:
    matrix = MATRIX.read_text(encoding="utf-8")
    required_matrix_tokens = (
        "ISO-TP transport matrix",
        "ISO 14229-1 UDS matrix",
        "HOST-COVERED",
        "TARGET-CROSS-BUILD",
        "PHYSICAL-HIL",
        "REVIEW-REQUIRED",
        "0xF1–0xF9",
        "References",
        "https://www.iso.org/standard/66574.html",
        "https://www.iso.org/standard/72439.html",
    )
    missing = [token for token in required_matrix_tokens if token not in matrix]
    if missing:
        raise SystemExit(f"conformance matrix missing required content: {missing}")

    profile = PROFILE.read_text(encoding="utf-8")
    required_profile_tokens = (
        "STM32F767",
        "CAN1 bxCAN",
        "0x7E0",
        "0x7E8",
        "termination_ohms",
        "destructive_tests_enabled: false",
        "download_flash_tests_enabled: false",
    )
    missing = [token for token in required_profile_tokens if token not in profile]
    if missing:
        raise SystemExit(f"board profile missing required content: {missing}")

    c092_files = {
        "can_transport_fdcan.c",
        "can_transport_fdcan.h",
        "uds_app_config.h",
        "uds_app_fdcan.c",
        "uds_app_fdcan.h",
        "uds_platform_fdcan.c",
        "uds_platform_fdcan.h",
        "README.md",
    }
    missing_files = sorted(name for name in c092_files if not (C092_DIR / name).is_file())
    if missing_files:
        raise SystemExit(f"C092 adapter missing required files: {missing_files}")
    c092_transport = (C092_DIR / "can_transport_fdcan.c").read_text(encoding="utf-8")
    c092_app_header = (C092_DIR / "uds_app_fdcan.h").read_text(encoding="utf-8")
    c092_app_source = (C092_DIR / "uds_app_fdcan.c").read_text(encoding="utf-8")
    c092_profile = (C092_DIR / "uds_app_config.h").read_text(encoding="utf-8")
    c092_guide = (C092_DIR / "README.md").read_text(encoding="utf-8")
    c092_required_tokens = (
        "FDCAN_TX_FIFO_OPERATION",
        "FDCAN_STORE_TX_EVENTS",
        "FDCAN_TX_EVENT",
        "HAL_FDCAN_GetTxEvent",
        "isotp_config_set_padding",
        "0xCCU",
        "uds_c092_app_init_default",
    )
    missing = [
        token
        for token in c092_required_tokens
        if token not in c092_transport + c092_profile + c092_guide
    ]
    if missing:
        raise SystemExit(f"C092 adapter/profile missing required content: {missing}")

    f767_profile = (ROOT / "App/Inc/uds_app_config.h").read_text(encoding="utf-8")
    if "UDS_APP_CLASSIC_PADDING_ENABLED 1U" not in f767_profile or "0xCCU" not in f767_profile:
        raise SystemExit("F767 application profile must explicitly enable 0xCC padding")

    bxcan_header = (ROOT / "App/Inc/can_transport.h").read_text(encoding="utf-8")
    bxcan_transport = (ROOT / "App/Src/can_transport.c").read_text(encoding="utf-8")
    bxcan_app = (ROOT / "App/Src/uds_app.c").read_text(encoding="utf-8")
    if "tx_pending" in bxcan_header + bxcan_transport + bxcan_app:
        raise SystemExit("F767 bxCAN application transport must not depend on tx_pending")
    bxcan_required_tokens = (
        "HAL_CAN_AddTxMessage",
        "HAL_CAN_IsTxMessagePending",
        "tx_mailbox_mask",
    )
    missing = [token for token in bxcan_required_tokens if token not in bxcan_header + bxcan_transport]
    if missing:
        raise SystemExit(f"F767 bxCAN completion contract missing required content: {missing}")
    if "HAL_Delay" in bxcan_header + bxcan_transport + bxcan_app:
        raise SystemExit("Issue #13 bxCAN path must not introduce an artificial delay")

    endpoint_source = (ROOT / "library/src/endpoint.c").read_text(encoding="utf-8")
    c092_header = (C092_DIR / "can_transport_fdcan.h").read_text(encoding="utf-8")
    c092_source = c092_transport
    if ("tx_pending" not in endpoint_source) or ("uds_server_complete_reset" not in endpoint_source):
        raise SystemExit("generic endpoint must retain protocol-owned tx_pending and reset completion")
    if ("tx_pending" not in c092_header + c092_source) or ("FDCAN_TX_EVENT" not in c092_source):
        raise SystemExit("C092 FDCAN adapter must retain hardware-specific TX event state")
    if ("uds_c092_app_init_default" not in c092_app_header + c092_app_source + c092_guide):
        raise SystemExit("C092 application must provide a copy-ready default initializer")
    if "uds_c092_app_init_default(&uds_transport" not in c092_guide:
        raise SystemExit("C092 guide must show the default initializer call")
    if "uds_c092_app_init(&uds_transport, uds_c092_platform_now_ms(),\n                  NULL, NULL, NULL, NULL);" not in c092_guide:
        raise SystemExit("C092 guide must show the explicit NULL callback/context call")

    vectors = json.loads(VECTORS.read_text(encoding="utf-8"))
    if vectors.get("schema_version") != 1 or not vectors.get("vectors"):
        raise SystemExit("conformance vector file is empty or has an unsupported schema")
    allowed_targets = {
        "uds_iso_tp_isotp_contract",
        "uds_iso_tp_uds_contract",
        "uds_iso_tp_endpoint_contract",
        "uds_iso_tp_adapters_contract",
    }
    vector_ids = [vector.get("id") for vector in vectors["vectors"]]
    if len(vector_ids) != len(set(vector_ids)) or any(not vector_id for vector_id in vector_ids):
        raise SystemExit("conformance vector IDs must be unique and non-empty")
    for vector in vectors["vectors"]:
        if vector.get("test_target") not in allowed_targets:
            raise SystemExit(f"unknown conformance test target: {vector.get('test_target')}")
        if "stimulus" not in vector or "expected" not in vector:
            raise SystemExit(f"incomplete conformance vector: {vector.get('id')}")

    plan = json.loads(PLAN.read_text(encoding="utf-8"))
    if plan.get("schema_version") != 1:
        raise SystemExit("unsupported HIL plan schema")
    profiles = plan.get("profiles", {})
    for name in ("classic-can", "c092-fdcan-classic", "can-fd"):
        if name not in profiles:
            raise SystemExit(f"HIL plan missing profile: {name}")
    cases = plan.get("cases", [])
    if not cases:
        raise SystemExit("HIL plan contains no cases")
    case_ids = [case.get("id") for case in cases]
    if len(case_ids) != len(set(case_ids)) or any(not case_id for case_id in case_ids):
        raise SystemExit("HIL plan case IDs must be unique and non-empty")
    if not any(case.get("destructive") for case in cases):
        raise SystemExit("HIL plan must retain explicitly marked destructive cases")
    if plan.get("required_evidence", []) == []:
        raise SystemExit("HIL plan must define required evidence")
    print(f"validation assets OK: {len(vectors['vectors'])} conformance vectors, {len(cases)} physical cases")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
