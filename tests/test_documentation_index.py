#!/usr/bin/env python3
"""Check that living documentation is discoverable and validation guidance stays current."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MARKDOWN_LINK = re.compile(r"\[[^\]]+\]\(([^)]+)\)")


def check_links(path: Path) -> list[str]:
    errors: list[str] = []
    for target in MARKDOWN_LINK.findall(path.read_text(encoding="utf-8")):
        if target.startswith(("http://", "https://", "mailto:", "#")):
            continue
        target_path = target.split("#", 1)[0]
        if not target_path:
            continue
        resolved = (path.parent / target_path).resolve()
        if not resolved.exists():
            errors.append(f"{path.relative_to(ROOT)} -> {target}")
    return errors


def main() -> int:
    checked = [
        ROOT / "README.md",
        ROOT / "CONTRIBUTING.md",
        ROOT / "docs" / "README.md",
        ROOT / "docs" / "handling_third_party_od_requests.md",
        ROOT / "docs" / "mock_canopen_protocol_smoke_testing.md",
    ]
    errors = [error for path in checked for error in check_links(path)]
    contributing = (ROOT / "CONTRIBUTING.md").read_text(encoding="utf-8")
    required_commands = (
        "python3 scripts/validate_inventus_battery.py",
        "python3 scripts/mock_canopen_runner.py",
        "make -C tests/host all test-stm32-facade test-gateway-default-deny test-inventus-battery test-mock-canopen",
    )
    errors.extend(f"CONTRIBUTING.md missing command: {command}" for command in required_commands if command not in contributing)
    if errors:
        for error in errors:
            print(f"FAIL: {error}")
        return 1
    print(f"documentation_index: PASS ({len(checked)} files, required validation commands present)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
