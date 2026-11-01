# Decision log

Newest first. One entry per decision: context, decision, consequences, status. Reversing a decision
is a new entry that links the old one.

## D8 — Tier 1 keeps the blacklist from one per-frame hook, not from every ownership call site (2026-11-01)

Context: `CharacterService.cpp` flips actors between remote and local in twelve places
(`SetRemote(true/false)`), and `Actor::SetFactions` wipes every local faction on each sync.
Instrumenting each site is brittle against upstream merges. `HookActorProcess`
(`Code/client/Games/Skyrim/Actor.cpp`) already runs for every actor every frame and already reads
`IsRemote()`.
Decision: the bridge reconciles desired vs actual blacklist state in that hook, before the remote
early-return: remote (copy or remote player) and not yet blacklisted → `SetFactionRank(blacklist, 0)`;
local and blacklisted → `SetFactionRank(blacklist, -1)`. "Blacklisted" is a new bit in
`ActorExtension` (`kSkyrimNetBlacklisted`), so the per-frame cost is a flag compare, not a faction
scan. `Actor::SetFactions` clears that bit after `RemoveFromAllFactions()`, so a sync wipe is repaired
on the next frame. `Actor::GetFactions()` strips the blacklist faction before returning, so it is
never sent to another client. Ownership transfer therefore needs no instrumentation; the transition
is detected in the hook (bit vs `IsRemote()`), which is also where the optional dialogue purge on
loss of ownership is triggered.
Consequences: T1.2, T1.3, T1.4 and T1.5 are delivered as one PR; upstream edits are three marked
blocks (`HookActorProcess`, `SetFactions`, `GetFactions`) plus one enum bit. Known cost: a copy is
exposed to SkyrimNet for at most one frame after each faction sync.

## D7 — Session roles: overseer plans and reviews, cheaper sessions implement, humans test (2026-11-01)

Context: the owner wants to spend tokens efficiently and let a friend contribute with their own.
Decision: planning, PR review and decisions by a high-end model session; implementation by
medium-effort sessions, one issue per session, delivered as PRs; nothing counts as tested until a
human logs it in `TESTLOG.md`. GitHub is the only shared memory between sessions.
Consequences: `CLAUDE.md` is the hand-off document; keep it current.

## D6 — Branch model (2026-11-01)

Decision: `main` mirrors upstream `OhLookItsDaOne/urSovngarde` and is never edited directly;
`skyrimnet-bridge` is the integration branch; tasks branch from it as `t<id>-<slug>`; upstream is
merged into `main` then into `skyrimnet-bridge` about weekly (upstream commits daily).
Consequences: keep bridge code in new files where possible to make merges cheap.

## D5 — Tier 3 audio source: open (2026-11-01)

Context: SkyrimNet renders TTS in-process and plays it through the game's audio manager; no file,
no API. Options: (a) ask Fateless.Ai for an audio-rendered callback; (b) route SkyrimNet's TTS
through a network endpoint behind a proxy we control; (c) hook the engine's audio submission.
Decision: deferred to T3.1. Pursue (a) and (b) in parallel; (c) only as last resort.
Status: open.

## D4 — Text relay reuses `SubtitleRequest`; player speech is a new message (2026-11-01)

Context: urSovngarde already relays subtitle text by server id and shows it with
`SubtitleManager::ShowSubtitle`. There is no message for player speech.
Decision: Tier 2 NPC lines ride the existing subtitle message with a hide timer; player speech gets
`PlayerSpeech` (protocol bump).
Consequences: Tier 1 and the NPC-lines half of Tier 2 need no protocol change.

## D3 — AI follows NPC ownership, not a single host AI (2026-11-01)

Context: two designs: (i) one SkyrimNet (the host's) runs every NPC; (ii) each NPC's AI runs in
whichever client owns it under urSovngarde's rules.
Decision: (ii). NPC actions only execute in the owner's game, and the host does not own NPCs near
a distant guest; (ii) keeps actions working everywhere and degrades gracefully. Memory is kept
aligned by registering relayed lines in every client.
Consequences: the other player's speech must be routed to the NPC's owner; ownership changes must
move the AI with them (purge on loss).

## D2 — The bridge lives in a urSovngarde fork; SkyrimNet is never redistributed or modified (2026-11-01)

Context: urSovngarde is GPLv3 with full source; SkyrimNet's DLL is proprietary and its license
forbids redistribution except via official pages. Only urSovngarde knows which actors are copies
and only it can send network messages.
Decision: all bridge code is in this fork under GPLv3. SkyrimNet is used only through its public
surfaces (MIT Papyrus API, the C++ API header fetched from the official release at build time, the
faction/YAML filters). No SkyrimNet file is committed.
Consequences: users install SkyrimNet themselves; CI needs a fetch step (T0.2); `PublicAPI.h`
knowledge is captured as human-written notes (T0.4).

## D1 — Keep SkyrimNet + urSovngarde; do not switch to Mantella/CHIM or write a new mod (2026-11-01)

Context: requirements — same world, both players talk to and hear the same NPCs, NPC memory of
scripted and freeform dialogue, fast natural replies in the character's voice, no activation step.
Decision: SkyrimNet meets four of five today (not co-op); Mantella and CHIM fail "no activation"
by architecture and run external servers; no AI mod avoids the audio-relay problem because it is a
property of co-op. urSovngarde is the only Skyrim VR co-op. A from-scratch co-op or AI mod is a
multi-month project with no advantage.
Consequences: the whole project is the bridge described in `PLAN.md`.
