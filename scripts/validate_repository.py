#!/usr/bin/env python3
"""Validate repository support files and documentation invariants."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REQUIRED_FILES = (
    "README.md",
    "LICENSE",
    "BUILD.md",
    "CONTRIBUTING.md",
    "CODE_OF_CONDUCT.md",
    "SECURITY.md",
    "CHANGELOG.md",
    "VERSION",
    "THIRD_PARTY.md",
    ".clang-format",
    ".github/CODEOWNERS",
    ".github/pull_request_template.md",
    ".github/ISSUE_TEMPLATE/bug_report.md",
    ".github/ISSUE_TEMPLATE/feature_request.md",
)
LOCAL_LINK_RE = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
FENCE_RE = re.compile(r"^\s*(```|~~~)")
SEMVER_RE = re.compile(r"^0\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$")


def fail(message: str) -> None:
    raise SystemExit(f"repository validation failed: {message}")


def main() -> None:
    for relative in REQUIRED_FILES:
        path = ROOT / relative
        if not path.is_file() or not path.read_text(encoding="utf-8").strip():
            fail(f"required file is missing or empty: {relative}")

    license_text = (ROOT / "LICENSE").read_text(encoding="utf-8")
    if "STM32 CANopen Reference Research and Education License" not in license_text or "LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0" not in license_text:
        fail("LICENSE does not contain the project research/education license")

    version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    if not SEMVER_RE.fullmatch(version):
        fail(f"VERSION is not a supported pre-1.0 semantic version: {version!r}")

    markdown_files = [
        ROOT / "README.md",
        *sorted((ROOT / "docs").rglob("*.md")),
        *sorted((ROOT / "examples").rglob("*.md")),
    ]
    for path in markdown_files:
        text = path.read_text(encoding="utf-8")
        if "Manus AI" in text or "**Author:**" in text:
            fail(f"stale author attribution in {path.relative_to(ROOT)}")

        fence = None
        for line_number, line in enumerate(text.splitlines(), start=1):
            match = FENCE_RE.match(line)
            if not match:
                continue
            marker = match.group(1)
            if fence is None:
                fence = marker
            elif marker == fence:
                fence = None
        if fence is not None:
            fail(f"unclosed Markdown fence in {path.relative_to(ROOT)}")

        for target in LOCAL_LINK_RE.findall(text):
            target = target.strip().split("#", 1)[0]
            if not target or target.startswith(("http://", "https://", "mailto:", "data:")):
                continue
            link_path = (path.parent / target).resolve()
            if not link_path.exists():
                fail(
                    f"broken local Markdown link in {path.relative_to(ROOT)}: {target}"
                )

    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    for required_link in ("BUILD.md", "LICENSE", "COMMERCIAL-LICENSE.md", "CONTRIBUTING.md", "SECURITY.md"):
        if required_link not in readme:
            fail(f"README does not link to {required_link}")

    print("repository validation passed")


if __name__ == "__main__":
    main()
