#!/usr/bin/env python3
"""Fail if the standalone build graph regresses toward CANopen coupling."""
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
ALLOWED_DOCUMENTATION = {
    Path("docs/architecture/canopen_removal_audit.md"),
    Path("library/README.md"),
}
ARCHITECTURE_CHECKER = Path("tests/architecture/check_standalone_architecture.py")
FORBIDDEN_PATH = re.compile(r"(?:CanOpenSTM32|CANopenNode|CANopen|canopen_reference|middleware/canopen|third_party/CanOpenSTM32)")
FORBIDDEN_SYMBOL = re.compile(
    r"(?:CanOpenSTM32|CANopenNode|CANopen|CANOPEN|canopen_reference|canopen_nvm|"
    r"(?<![A-Za-z0-9_])CO_[A-Za-z0-9_]+|(?<![A-Za-z0-9_])OD_[A-Za-z0-9_]+|"
    r"cia401|cia402|cia418|\b(?:NMT|PDO|SDO|EMCY|Heartbeat|LSS|SYNC)\b)",
    re.IGNORECASE,
)
SOURCE_SUFFIXES = {".c", ".h", ".py", ".sh", ".cmake", ".ioc", ".ld", ".mk", ".yml", ".yaml"}
SKIP_SOURCE_DIRS = {"Core", "Drivers", "build", "build-flow-final", "build-flow-sanitize", "build-flow-audit", "build-flow-coverage", "build-legacy-compat"}


def tracked_paths():
    output = subprocess.check_output(["git", "ls-files"], cwd=ROOT, text=True)
    return [Path(line) for line in output.splitlines() if line]


def main():
    errors = []
    paths = tracked_paths()
    for path in paths:
        if FORBIDDEN_PATH.search(path.as_posix()) and path not in ALLOWED_DOCUMENTATION:
            errors.append(f"forbidden path: {path}")
        if path in ALLOWED_DOCUMENTATION or path == ARCHITECTURE_CHECKER or (path.suffix not in SOURCE_SUFFIXES and path.name != "CMakeLists.txt"):
            continue
        if any(part in SKIP_SOURCE_DIRS for part in path.parts):
            continue
        try:
            text = (ROOT / path).read_text(encoding="utf-8", errors="ignore")
        except OSError as exc:
            errors.append(f"cannot read {path}: {exc}")
            continue
        for line_number, line in enumerate(text.splitlines(), 1):
            if FORBIDDEN_SYMBOL.search(line):
                errors.append(f"forbidden symbol in {path}:{line_number}: {line.strip()}")
    if (ROOT / ".gitmodules").exists():
        errors.append(".gitmodules must not exist in the standalone repository")
    if (ROOT / "third_party" / "CanOpenSTM32").exists():
        errors.append("CANopen submodule directory still exists")
    if (ROOT / "middleware" / "diagnostics").exists() or (ROOT / "middleware" / "canopen").exists():
        errors.append("legacy CANopen middleware directory still exists")
    if errors:
        print("standalone architecture check failed:", file=sys.stderr)
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"standalone architecture check passed for {len(paths)} tracked paths")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
