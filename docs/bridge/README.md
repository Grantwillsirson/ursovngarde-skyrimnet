# SkyrimNet × urSovngarde bridge — planning index

| Document | What it is | Who updates it |
| --- | --- | --- |
| [`../../CLAUDE.md`](../../CLAUDE.md) | Brief for any AI session: vocabulary, rules, code map, open questions | Overseer session |
| [`FEASIBILITY.md`](FEASIBILITY.md) | Why this combination, what breaks, what the alternatives cost | Overseer; frozen once building starts |
| [`PLAN.md`](PLAN.md) | The design: tiers, hook points, messages, data flow, risks | Overseer; implementers propose changes via PR |
| [`TASKS.md`](TASKS.md) | The task list with acceptance criteria and dependencies; mirrored to GitHub Issues | Everyone (tick in the PR that closes the task) |
| [`DECISIONS.md`](DECISIONS.md) | Decision log, newest first | Whoever decides; overseer confirms |
| [`TESTLOG.md`](TESTLOG.md) | Every real-session result, newest first | Human testers only |
| [`SKYRIMNET-API-NOTES.md`](SKYRIMNET-API-NOTES.md) | Human-written notes on what `PublicAPI.h` exposes (the file itself is never committed) | Human with a SkyrimNet install |
| [`create-issues.py`](create-issues.py) | Creates labels, milestones and one issue per task from TASKS.md (`python3 docs/bridge/create-issues.py --repo OWNER/REPO`; re-runnable) | Whoever has repo access, after new tasks are added |

Status: **planning complete, no bridge code written.** Baseline test (T0.3) is the gate for Tier 1.

Upstream: `OhLookItsDaOne/urSovngarde`, tracked on `main`. Work branch: `skyrimnet-bridge`.
