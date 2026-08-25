#!/usr/bin/env bash
# SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
# Create a blank, non-passing external evidence package for a release candidate.
set -euo pipefail

OUTPUT_DIR=
RELEASE_COMMIT=
BOARD_REVISION=UNSET
HARDWARE_SERIAL=UNSET

while [[ $# -gt 0 ]]; do
    case "$1" in
        --output) OUTPUT_DIR=${2:?--output requires a directory}; shift 2 ;;
        --release-commit) RELEASE_COMMIT=${2:?--release-commit requires a SHA}; shift 2 ;;
        --board-revision) BOARD_REVISION=${2:?--board-revision requires a value}; shift 2 ;;
        --hardware-serial) HARDWARE_SERIAL=${2:?--hardware-serial requires a value}; shift 2 ;;
        -h|--help)
            printf '%s\n' 'usage: init_external_evidence_package.sh --output DIR --release-commit SHA [--board-revision REV] [--hardware-serial SERIAL]'
            exit 0
            ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

[[ -n "$OUTPUT_DIR" ]] || { echo '--output is required' >&2; exit 2; }
[[ -n "$RELEASE_COMMIT" ]] || { echo '--release-commit is required' >&2; exit 2; }
[[ "$RELEASE_COMMIT" =~ ^[0-9a-fA-F]{7,64}$ ]] || { echo '--release-commit must be a hexadecimal git SHA' >&2; exit 2; }

if [[ -e "$OUTPUT_DIR" ]]; then
    echo "refusing to overwrite existing evidence directory: $OUTPUT_DIR" >&2
    exit 1
fi
mkdir -p "$OUTPUT_DIR"

python3 - "$OUTPUT_DIR" "$RELEASE_COMMIT" "$BOARD_REVISION" "$HARDWARE_SERIAL" <<'PY'
import json
import sys
from pathlib import Path

output = Path(sys.argv[1])
release_commit, board_revision, hardware_serial = sys.argv[2:]
records = (
    "board_electrical_review.md",
    "physical_can_interoperability.md",
    "bus_off_campaign.md",
    "flash_power_loss_endurance.md",
    "watchdog_timing.md",
    "cia401_acceptance.md",
    "cia402_acceptance.md",
    "cia302_lss_commissioning.md",
    "security_update_approval.md",
    "formal_canopen_conformance.md",
)
manifest = {
    "schema": "stm32-canopen-release-evidence-v1",
    "status": "PENDING",
    "release_commit": release_commit,
    "board_revision": board_revision,
    "hardware_serial": hardware_serial,
    "note": "Template only; replace with approved evidence before production release.",
}
(output / "release-evidence-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
for name in records:
    (output / name).write_text(
        "# External evidence record\n\n"
        "status: PENDING\n"
        f"release_commit: {release_commit}\n"
        "evidence_id: TODO\n"
        "reviewer: TODO\n"
        "evidence_type: TODO\n\n"
        "Replace this template with the controlled test record. Do not mark PASS without the required hardware, security, or conformance evidence.\n",
        encoding="utf-8",
    )
(output / "release-socketcan-status.txt").write_text(
    "status: unavailable\n"
    "native_runtime_tests: not-run\n"
    "note: replace with actual native SocketCAN or physical CAN evidence\n",
    encoding="utf-8",
)
PY

printf 'created pending external evidence template: %s\n' "$OUTPUT_DIR"
