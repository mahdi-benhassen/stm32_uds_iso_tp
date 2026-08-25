#!/usr/bin/env python3
"""Validate the reproducibility manifest emitted by write_build_manifest.sh."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

SHA256 = re.compile(r"^[0-9a-f]{64}$")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    args = parser.parse_args()
    document = json.loads(args.manifest.read_text(encoding="utf-8"))
    assert document["schema"] == "stm32-canopen-build-manifest-v2"
    assert isinstance(document["source"]["revision"], str) and document["source"]["revision"]
    assert isinstance(document["source"]["dirty"], bool)
    assert document["submodules"]["canopenstm32_revision"]
    assert document["submodules"]["stm32cubef7_revision"]
    assert document["configuration"]["personality"]
    assert SHA256.fullmatch(document["inputs"]["object_dictionary_sha256"])
    assert SHA256.fullmatch(document["inputs"]["linker_script_sha256"])
    print(f"validated build manifest for {document['configuration']['personality']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
