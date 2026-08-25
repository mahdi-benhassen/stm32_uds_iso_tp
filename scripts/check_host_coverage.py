#!/usr/bin/env python3
"""Aggregate gcov JSON and enforce explicit host coverage thresholds.

The report is software-test evidence for the modules compiled by the host
coverage target. It is not a claim about target firmware or hardware coverage.
"""
from __future__ import annotations

import argparse
import gzip
import json
import sys
from pathlib import Path


def percent(covered: int, total: int) -> float:
    return 100.0 * covered / total if total else 100.0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("build_dir", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--min-line", type=float, default=80.0)
    parser.add_argument("--min-function", type=float, default=80.0)
    parser.add_argument("--min-branch", type=float, default=80.0)
    args = parser.parse_args()

    reports = sorted(args.build_dir.glob("*.gcov.json.gz"))
    if not reports:
        raise SystemExit(f"no gcov JSON reports found in {args.build_dir}")

    modules: list[dict[str, object]] = []
    totals = {"lines": [0, 0], "functions": [0, 0], "branches": [0, 0]}
    for report_path in reports:
        with gzip.open(report_path, "rt", encoding="utf-8") as stream:
            document = json.load(stream)
        for source in document.get("files", []):
            source_path = str(source.get("file", ""))
            if "/App/" not in source_path and "/middleware/" not in source_path:
                continue
            lines = source.get("lines", [])
            functions = source.get("functions", [])
            branches = [branch for line in lines for branch in line.get("branches", [])]
            line_total = len(lines)
            line_covered = sum(1 for line in lines if int(line.get("count", 0)) > 0)
            function_total = len(functions)
            function_covered = sum(1 for function in functions if int(function.get("execution_count", 0)) > 0)
            branch_total = len(branches)
            branch_covered = sum(1 for branch in branches if int(branch.get("count", 0)) > 0)
            totals["lines"][0] += line_covered
            totals["lines"][1] += line_total
            totals["functions"][0] += function_covered
            totals["functions"][1] += function_total
            totals["branches"][0] += branch_covered
            totals["branches"][1] += branch_total
            modules.append(
                {
                    "source": source_path,
                    "lines": {"covered": line_covered, "total": line_total, "percent": round(percent(line_covered, line_total), 2)},
                    "functions": {"covered": function_covered, "total": function_total, "percent": round(percent(function_covered, function_total), 2)},
                    "branches": {"covered": branch_covered, "total": branch_total, "percent": round(percent(branch_covered, branch_total), 2)},
                }
            )

    metrics = {
        "line": round(percent(*totals["lines"]), 2),
        "function": round(percent(*totals["functions"]), 2),
        "branch": round(percent(*totals["branches"]), 2),
    }
    thresholds = {"line": args.min_line, "function": args.min_function, "branch": args.min_branch}
    failures = [name for name, value in metrics.items() if value < thresholds[name]]
    result = {
        "schema": "stm32-canopen-host-coverage-v1",
        "scope": "gcov JSON for project-owned App/ and middleware/ host-test modules",
        "modules": modules,
        "totals": totals,
        "metrics_percent": metrics,
        "thresholds_percent": thresholds,
        "passed": not failures,
        "failures": failures,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))
    if failures:
        print(f"coverage threshold failure: {', '.join(failures)}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
