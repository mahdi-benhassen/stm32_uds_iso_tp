#!/usr/bin/env bash
# SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
# Import CANopenNode-compatible objdictgen C/H output without silently changing
# the active firmware Object Dictionary.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INPUT="${1:-}"
MODE="${2:---stage}"
STAGE_DIR="$ROOT/middleware/canopen/od/imported"
ACTIVE_DIR="$ROOT/Generated"
EDS_DIR="$ROOT/ObjectDictionary"
VALIDATOR="$ROOT/scripts/validate_od.py"

usage() {
    cat <<'USAGE'
Usage: tools/import_objdict.sh <objdictgen-output-directory|zip> [--stage|--activate]

The input must contain exactly one CANopenNode-compatible OD.c and OD.h pair.
An optional .eds file is retained alongside the staged import. --stage is the
safe default. --activate copies the validated pair into Generated/ and, when
present, the EDS into ObjectDictionary/imported_object_dictionary.eds. Review
and rebuild after activation; this command never asserts profile conformance.
USAGE
}

if [[ -z "$INPUT" || ( "$MODE" != "--stage" && "$MODE" != "--activate" ) ]]; then
    usage >&2
    exit 64
fi
if [[ ! -e "$INPUT" ]]; then
    echo "Import failed: input does not exist: $INPUT" >&2
    exit 66
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
SOURCE="$INPUT"

if [[ -f "$INPUT" ]]; then
    case "${INPUT,,}" in
        *.zip)
            # Reject absolute paths and traversal paths before extraction.
            while IFS= read -r entry; do
                if [[ "$entry" == /* || "$entry" == *"../"* || "$entry" == ".." ]]; then
                    echo "Import failed: archive contains unsafe path: $entry" >&2
                    exit 65
                fi
            done < <(unzip -Z1 "$INPUT")
            unzip -q "$INPUT" -d "$WORK/extracted"
            SOURCE="$WORK/extracted"
            ;;
        *)
            echo "Import failed: archive input must be a .zip file" >&2
            exit 65
            ;;
    esac
elif [[ ! -d "$INPUT" ]]; then
    echo "Import failed: input must be a directory or .zip archive" >&2
    exit 65
fi

mapfile -t CANDIDATE_C < <(find "$SOURCE" -type f -name OD.c -print | sort)
mapfile -t CANDIDATE_H < <(find "$SOURCE" -type f -name OD.h -print | sort)
if [[ ${#CANDIDATE_C[@]} -ne 1 || ${#CANDIDATE_H[@]} -ne 1 ]]; then
    echo "Import failed: expected exactly one OD.c and one OD.h in input" >&2
    exit 65
fi

rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR"
install -m 0644 "${CANDIDATE_C[0]}" "$STAGE_DIR/OD.c"
install -m 0644 "${CANDIDATE_H[0]}" "$STAGE_DIR/OD.h"

mapfile -t CANDIDATE_EDS < <(find "$SOURCE" -type f \( -iname '*.eds' -o -iname '*.dcf' \) -print | sort)
if [[ ${#CANDIDATE_EDS[@]} -gt 1 ]]; then
    echo "Import failed: input has multiple EDS/DCF candidates; retain one authoritative file" >&2
    exit 65
fi
if [[ ${#CANDIDATE_EDS[@]} -eq 1 ]]; then
    install -m 0644 "${CANDIDATE_EDS[0]}" "$STAGE_DIR/object_dictionary.eds"
    python3 "$VALIDATOR" --generic --od-c "$STAGE_DIR/OD.c" --od-h "$STAGE_DIR/OD.h" --eds "$STAGE_DIR/object_dictionary.eds"
else
    python3 "$VALIDATOR" --generic --od-c "$STAGE_DIR/OD.c" --od-h "$STAGE_DIR/OD.h"
fi

cat > "$STAGE_DIR/IMPORT_MANIFEST.txt" <<MANIFEST
source=$(realpath "$INPUT")
mode=$MODE
od_c_sha256=$(sha256sum "$STAGE_DIR/OD.c" | awk '{print $1}')
od_h_sha256=$(sha256sum "$STAGE_DIR/OD.h" | awk '{print $1}')
MANIFEST

if [[ "$MODE" == "--activate" ]]; then
    install -m 0644 "$STAGE_DIR/OD.c" "$ACTIVE_DIR/OD.c"
    install -m 0644 "$STAGE_DIR/OD.h" "$ACTIVE_DIR/OD.h"
    if [[ -f "$STAGE_DIR/object_dictionary.eds" ]]; then
        install -m 0644 "$STAGE_DIR/object_dictionary.eds" "$EDS_DIR/imported_object_dictionary.eds"
    fi
    echo "Imported Object Dictionary activated. Run scripts/validate_reference.sh and review profile/EDS semantics."
else
    echo "Imported Object Dictionary staged at $STAGE_DIR. Use --activate only after review."
fi
