#!/usr/bin/env python3
"""Mirror docs/bridge/TASKS.md into GitHub issues, labels and milestones.

TASKS.md is the single source of truth; this script only reads it. Safe to re-run: existing
labels, milestones and issues (matched by the task id at the start of the title) are left alone.

Usage:
    python3 docs/bridge/create-issues.py --repo OWNER/REPO            # create
    python3 docs/bridge/create-issues.py --repo OWNER/REPO --dry-run  # print what would be created

Needs the GitHub CLI (`gh`) authenticated with access to the repo.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

TASKS = Path(__file__).with_name("TASKS.md")

MILESTONE_RE = re.compile(r"^## Milestone (\d) — (.+?)(?: \(gate:.*\))?$")
TASK_RE = re.compile(
    r"^- \[[ xX]\] \*\*(T\d+\.\d+)\*\* (.+?) — `([A-Za-z+]+)`, ([SML])(?: — depends on (.+?))?(?: — if .*)?$"
)

LABELS = {
    "bridge": ("0e8a16", "SkyrimNet x urSovngarde bridge work"),
    "tier-0": ("c5def5", "Milestone 0: setup"),
    "tier-1": ("c5def5", "Milestone 1: AI follows ownership"),
    "tier-2": ("c5def5", "Milestone 2: text both ways"),
    "tier-3": ("c5def5", "Milestone 3: voice"),
    "tier-4": ("c5def5", "Milestone 4: release"),
    "protocol": ("d93f0b", "Changes network messages: every client and the server must match"),
    "human-test": ("fbca04", "Needs a person with Skyrim VR rigs"),
    "ai": ("5319e7", "Implementable by an AI session with CI builds"),
    "ai+human": ("5319e7", "AI drafts, human verifies"),
    "size:S": ("ededed", "hours"),
    "size:M": ("ededed", "about a day"),
    "size:L": ("ededed", "days"),
}


def gh(*args: str, dry: bool = False, capture: bool = True) -> str:
    cmd = ["gh", *args]
    if dry:
        print("DRY:", " ".join(repr(a) if " " in a else a for a in cmd))
        return ""
    res = subprocess.run(cmd, check=False, capture_output=capture, text=True)
    if res.returncode != 0:
        sys.stderr.write(res.stderr or res.stdout)
        raise SystemExit(f"gh failed: {' '.join(cmd[:3])}")
    return res.stdout


def parse_tasks() -> tuple[dict[str, str], list[dict]]:
    milestones: dict[str, str] = {}
    tasks: list[dict] = []
    current_ms = None
    current: dict | None = None
    for raw in TASKS.read_text(encoding="utf-8").splitlines():
        m = MILESTONE_RE.match(raw)
        if m:
            current_ms = f"M{m.group(1)} {m.group(2).strip()}"
            milestones[m.group(1)] = current_ms
            current = None
            continue
        t = TASK_RE.match(raw)
        if t:
            tid, title, who, size, deps = t.groups()
            protocol = "[protocol]" in title
            current = {
                "id": tid,
                "title": title.replace("`[protocol]`", "").replace("[protocol]", "").strip(),
                "who": who,
                "size": size,
                "deps": [d.strip() for d in deps.split(",")] if deps else [],
                "protocol": protocol,
                "tier": tid[1],
                "milestone": current_ms,
                "body": [],
            }
            tasks.append(current)
            continue
        if current is not None and raw.startswith("  - "):
            current["body"].append(raw[4:].strip())
        elif current is not None and raw.startswith("    ") and current["body"]:
            # wrapped continuation of the previous bullet
            current["body"][-1] += " " + raw.strip()
        elif raw.strip() == "":
            continue
        else:
            current = None
    return milestones, tasks


def issue_body(t: dict) -> str:
    lines = [
        f"**Who:** `{t['who']}` · **Size:** {t['size']}"
        + (f" · **Depends on:** {', '.join(t['deps'])}" if t["deps"] else ""),
        "",
    ]
    for b in t["body"]:
        lines.append(f"- {b}")
    lines += [
        "",
        "---",
        f"Source of truth: `docs/bridge/TASKS.md` ({t['id']}). Design: `docs/bridge/PLAN.md`. "
        "Rules: `CLAUDE.md`. Tick the task in TASKS.md in the PR that closes this issue.",
        "",
        "Claim this issue by assigning yourself and commenting which session/person has it before starting.",
    ]
    if t["protocol"]:
        lines.insert(
            2,
            "> **Protocol change.** Bump the protocol version in the same PR; every client and the "
            "server must run the same build.\n",
        )
    return "\n".join(lines)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", required=True, help="OWNER/REPO")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    dry = args.dry_run
    repo = args.repo

    milestones, tasks = parse_tasks()
    print(f"Parsed {len(tasks)} tasks across {len(milestones)} milestones from {TASKS.name}")

    # Labels
    existing_labels = set()
    if not dry:
        out = gh("label", "list", "--repo", repo, "--limit", "200", "--json", "name")
        existing_labels = {l["name"] for l in json.loads(out or "[]")}
    for name, (color, desc) in LABELS.items():
        if name in existing_labels:
            continue
        gh("label", "create", name, "--repo", repo, "--color", color, "--description", desc, dry=dry)

    # Milestones (REST; gh has no milestone create)
    ms_numbers: dict[str, int] = {}
    if not dry:
        out = gh("api", f"repos/{repo}/milestones?state=all&per_page=100")
        for m in json.loads(out or "[]"):
            ms_numbers[m["title"]] = m["number"]
    for title in milestones.values():
        if title in ms_numbers:
            continue
        out = gh("api", "-X", "POST", f"repos/{repo}/milestones", "-f", f"title={title}", dry=dry)
        if out:
            ms_numbers[title] = json.loads(out)["number"]

    # Issues
    existing_titles: list[str] = []
    if not dry:
        out = gh("issue", "list", "--repo", repo, "--state", "all", "--limit", "500", "--json", "title")
        existing_titles = [i["title"] for i in json.loads(out or "[]")]
    created = 0
    for t in tasks:
        full_title = f"{t['id']} {t['title']}"
        if any(x.startswith(t["id"] + " ") for x in existing_titles):
            print(f"skip (exists): {full_title}")
            continue
        labels = ["bridge", f"tier-{t['tier']}", f"size:{t['size']}"]
        labels.append({"AI": "ai", "Human": "human-test", "AI+Human": "ai+human"}.get(t["who"], "ai"))
        if t["protocol"]:
            labels.append("protocol")
        cmd = [
            "issue", "create", "--repo", repo,
            "--title", full_title,
            "--body", issue_body(t),
            "--label", ",".join(labels),
        ]
        if t["milestone"]:
            cmd += ["--milestone", t["milestone"]]
        gh(*cmd, dry=dry)
        created += 1
        print(f"created: {full_title}")
    print(f"Done. {created} issue(s) created.")


if __name__ == "__main__":
    main()
