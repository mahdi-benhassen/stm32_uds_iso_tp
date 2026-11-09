#!/usr/bin/env python3
"""
GitHub Milestone Management Tool for STM32 UDS & ISO-TP Repository

This utility provides automated management, reporting, and validation of
GitHub Milestones, issues, and release changelogs according to automotive
software engineering best practices.

Usage examples:
    # List all repository milestones and progress
    python tools/github_milestone_manager.py list

    # Display detailed breakdown of a milestone
    python tools/github_milestone_manager.py report --milestone 1

    # Generate release notes / changelog from a milestone
    python tools/github_milestone_manager.py changelog --milestone 1

    # Verify that a Pull Request is assigned to an active milestone (for CI)
    python tools/github_milestone_manager.py check-pr --pr 152

    # Assign an issue to a milestone
    python tools/github_milestone_manager.py assign --milestone 3 --issue 153
"""

import argparse
import os
import sys
import json
import urllib.request
import urllib.error
from typing import Dict, List, Optional, Any

DEFAULT_REPO = "mahdi-benhassen/stm32_uds_iso_tp"


def get_token() -> Optional[str]:
    """Retrieve GitHub token from environment or fallback."""
    return (
        os.environ.get("GITHUB_TOKEN")
        or os.environ.get("GH_TOKEN")
        or os.environ.get("GITHUB_API_KEY")
    )


def api_request(
    endpoint: str,
    method: str = "GET",
    data: Optional[Dict[str, Any]] = None,
    token: Optional[str] = None,
    repo: str = DEFAULT_REPO,
) -> Any:
    """Send an authenticated request to the GitHub REST API v3."""
    url = f"https://api.github.com/repos/{repo}/{endpoint.lstrip('/')}"
    headers = {
        "Accept": "application/vnd.github.v3+json",
        "User-Agent": "stm32-uds-milestone-manager",
    }
    auth_token = token or get_token()
    if auth_token:
        headers["Authorization"] = f"token {auth_token}"

    encoded_data = None
    if data is not None:
        encoded_data = json.dumps(data).encode("utf-8")
        headers["Content-Type"] = "application/json"

    req = urllib.request.Request(url, data=encoded_data, headers=headers, method=method)

    try:
        with urllib.request.urlopen(req) as resp:
            content = resp.read().decode("utf-8")
            return json.loads(content) if content else None
    except urllib.error.HTTPError as e:
        err_body = e.read().decode("utf-8", errors="replace")
        print(f"API Error ({e.code}): {err_body}", file=sys.stderr)
        raise


def cmd_list(args: argparse.Namespace) -> None:
    """List all milestones with status, open/closed issues, and completion percentage."""
    milestones = api_request("milestones?state=all", token=args.token, repo=args.repo)
    if not milestones:
        print("No milestones found in repository.")
        return

    print("\n" + "=" * 80)
    print(f" GitHub Milestones for {args.repo}")
    print("=" * 80)

    for m in milestones:
        num = m["number"]
        title = m["title"]
        state = m["state"].upper()
        open_cnt = m["open_issues"]
        closed_cnt = m["closed_issues"]
        total = open_cnt + closed_cnt
        percent = (closed_cnt / total * 100.0) if total > 0 else 0.0
        due_str = m["due_on"][:10] if m.get("due_on") else "No deadline"

        bar_len = 24
        filled = int(round(bar_len * (percent / 100.0)))
        bar = "#" * filled + "-" * (bar_len - filled)

        status_indicator = "[CLOSED]" if state == "CLOSED" else "[OPEN]  "
        print(f"\n{status_indicator} Milestone #{num}: {title}")
        print(f"  Due Date:    {due_str}")
        print(f"  Progress:    [{bar}] {percent:5.1f}% ({closed_cnt}/{total} closed)")
        print(f"  URL:         {m['html_url']}")

    print("\n" + "=" * 80 + "\n")


def cmd_report(args: argparse.Namespace) -> None:
    """Generate a detailed report of issues assigned to a specific milestone."""
    milestone = api_request(f"milestones/{args.milestone}", token=args.token, repo=args.repo)
    issues = api_request(
        f"issues?milestone={args.milestone}&state=all&per_page=100",
        token=args.token,
        repo=args.repo,
    )

    print("\n" + "#" * 80)
    print(f" Milestone Report: #{milestone['number']} - {milestone['title']}")
    print(f" State: {milestone['state'].upper()} | Due: {milestone.get('due_on', 'N/A')}")
    print("#" * 80 + "\n")

    if milestone.get("description"):
        print("Objective & Scope:")
        print(milestone["description"])
        print("\n" + "-" * 80)

    prs = [i for i in issues if "pull_request" in i]
    iss = [i for i in issues if "pull_request" not in i]

    print(f"\nPull Requests ({len(prs)}):")
    for p in prs:
        status = "[x]" if p["state"] == "closed" else "[ ]"
        print(f"  {status} #{p['number']}: {p['title']}")

    print(f"\nIssues ({len(iss)}):")
    for i in iss:
        status = "[x]" if i["state"] == "closed" else "[ ]"
        print(f"  {status} #{i['number']}: {i['title']}")
    print("\n")


def cmd_changelog(args: argparse.Namespace) -> None:
    """Generate a markdown release changelog from all issues in a milestone."""
    milestone = api_request(f"milestones/{args.milestone}", token=args.token, repo=args.repo)
    issues = api_request(
        f"issues?milestone={args.milestone}&state=all&per_page=100",
        token=args.token,
        repo=args.repo,
    )

    features = []
    fixes = []
    architecture = []
    others = []

    for item in issues:
        title = item["title"]
        num = item["number"]
        url = item["html_url"]
        entry = f"- {title} ([#{num}]({url}))"

        lower_title = title.lower()
        if lower_title.startswith("feat") or "add" in lower_title:
            features.append(entry)
        elif lower_title.startswith("fix") or "bug" in lower_title or "error" in lower_title:
            fixes.append(entry)
        elif "fsm" in lower_title or "abstract" in lower_title or "audit" in lower_title or "bootloader" in lower_title:
            architecture.append(entry)
        else:
            others.append(entry)

    print(f"## Release Notes: {milestone['title']}\n")
    if milestone.get("due_on"):
        print(f"**Target Release Date:** {milestone['due_on'][:10]}\n")

    if features:
        print("### New Features & Services")
        for f in features:
            print(f)
        print()

    if fixes:
        print("### Bug Fixes & Stability")
        for fx in fixes:
            print(fx)
        print()

    if architecture:
        print("### Architecture, Bootloader & HAL")
        for a in architecture:
            print(a)
        print()

    if others:
        print("### Other Changes & Diagnostics")
        for o in others:
            print(o)
        print()


def cmd_check_pr(args: argparse.Namespace) -> None:
    """Check if a Pull Request is assigned to a milestone (CI gate)."""
    pr = api_request(f"issues/{args.pr}", token=args.token, repo=args.repo)
    milestone = pr.get("milestone")
    if not milestone:
        print(f"FAILED: Pull Request #{args.pr} is NOT assigned to any GitHub Milestone!", file=sys.stderr)
        print("Please assign an active milestone before merging.", file=sys.stderr)
        sys.exit(1)

    print(f"PASSED: Pull Request #{args.pr} is assigned to Milestone #{milestone['number']} ({milestone['title']}).")


def cmd_assign(args: argparse.Namespace) -> None:
    """Assign an issue or PR to a milestone."""
    payload = {"milestone": args.milestone}
    updated = api_request(
        f"issues/{args.issue}",
        method="PATCH",
        data=payload,
        token=args.token,
        repo=args.repo,
    )
    print(f"Successfully assigned issue #{args.issue} to Milestone #{args.milestone} ({updated.get('milestone', {}).get('title', '')})")


def main():
    common_parser = argparse.ArgumentParser(add_help=False)
    common_parser.add_argument("--repo", default=DEFAULT_REPO, help="GitHub repository (owner/repo)")
    common_parser.add_argument("--token", default=None, help="GitHub API Personal Access Token")

    parser = argparse.ArgumentParser(description="STM32 UDS ISO-TP GitHub Milestone Manager", parents=[common_parser])
    subparsers = parser.add_subparsers(dest="command", required=True)

    # list
    subparsers.add_parser("list", help="List all milestones", parents=[common_parser])

    # report
    p_report = subparsers.add_parser("report", help="Report details of a milestone", parents=[common_parser])
    p_report.add_argument("--milestone", type=int, required=True, help="Milestone number")

    # changelog
    p_changelog = subparsers.add_parser("changelog", help="Generate changelog for a milestone", parents=[common_parser])
    p_changelog.add_argument("--milestone", type=int, required=True, help="Milestone number")

    # check-pr
    p_check_pr = subparsers.add_parser("check-pr", help="Verify PR milestone assignment", parents=[common_parser])
    p_check_pr.add_argument("--pr", type=int, required=True, help="Pull Request number")

    # assign
    p_assign = subparsers.add_parser("assign", help="Assign issue/PR to milestone", parents=[common_parser])
    p_assign.add_argument("--issue", type=int, required=True, help="Issue or PR number")
    p_assign.add_argument("--milestone", type=int, required=True, help="Milestone number")

    args = parser.parse_args()

    commands = {
        "list": cmd_list,
        "report": cmd_report,
        "changelog": cmd_changelog,
        "check-pr": cmd_check_pr,
        "assign": cmd_assign,
    }

    cmd_fn = commands.get(args.command)
    if cmd_fn:
        cmd_fn(args)


if __name__ == "__main__":
    main()
