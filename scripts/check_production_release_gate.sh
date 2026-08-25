#!/usr/bin/env bash
# SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
# Release checklist gate. Production mode requires externally archived evidence;
# it never treats host reports as substitutes for hardware or conformance tests.
set -euo pipefail

MODE=software
EVIDENCE_DIR=
while [[ $# -gt 0 ]]; do
    case "$1" in
        --software) MODE=software; shift ;;
        --production) MODE=production; shift ;;
        --evidence-dir) EVIDENCE_DIR=${2:?--evidence-dir requires a path}; shift 2 ;;
        -h|--help)
            printf '%s\n' 'usage: check_production_release_gate.sh [--software|--production] [--evidence-dir DIR]'
            exit 0
            ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
require_file() {
    [[ -s "$1" ]] || { echo "release gate: missing or empty: $1" >&2; exit 1; }
}

require_file "$ROOT/build/ci-build-manifest.json"
require_file "$ROOT/build/ci-build-manifest.txt"
require_file "$ROOT/build/memory-budget.json"
require_file "$ROOT/build/reports/test-results.xml"
require_file "$ROOT/build/reports/coverage-summary.json"
require_file "$ROOT/build/reports/sanitizer-report.txt"
python3 "$ROOT/scripts/validate_release_artifacts.py" "$ROOT"

if [[ "$MODE" = software ]]; then
    printf '%s\n' 'software release gate: PASS (hardware and formal evidence intentionally not evaluated)'
    exit 0
fi

[[ -n "$EVIDENCE_DIR" ]] || { echo 'production release gate: --evidence-dir is required' >&2; exit 1; }
[[ -d "$EVIDENCE_DIR" ]] || { echo "production release gate: evidence directory does not exist: $EVIDENCE_DIR" >&2; exit 1; }

release_commit=$(git -C "$ROOT" rev-parse HEAD)
python3 "$ROOT/scripts/validate_external_evidence.py" "$EVIDENCE_DIR" --release-commit "$release_commit"
printf '%s\n' 'production release gate: PASS (software artifacts and machine-validated external evidence are complete)'
