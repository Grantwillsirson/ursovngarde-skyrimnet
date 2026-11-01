# Task list

One task = one GitHub issue = one branch = one PR. IDs are stable; the issue title starts with the ID.
**Who:** `AI` = implementable by an AI session with CI builds; `Human` = needs a person with a Skyrim VR
rig and SkyrimNet; `AI+Human` = AI drafts, human verifies. **Size:** S (hours), M (a day), L (days).

Tick a task only in the PR that completes it. A task that touches the network protocol says
`[protocol]` and bumps the protocol version in the same PR.

## Milestone 0 — Setup (gate: T0.3 answered before Tier 2 starts)

- [ ] **T0.1** Repo hygiene and green baseline — `AI`, S
  - Add `skyrimnet-bridge` to `.github/workflows/windows.yml` push/PR triggers.
  - Root `README-BRIDGE.md` pointing at `docs/bridge/`.
  - Confirm the unmodified fork builds green on CI; link the run in the PR.
  - Acceptance: CI green on `skyrimnet-bridge`; no source changes outside workflow + docs.
- [ ] **T0.2** CI: SkyrimNet API header at build time — `AI`, M — depends on T0.1
  - Workflow step: download the latest SkyrimNet release zip from `MinLL/SkyrimNet-GamePlugin`
    releases, extract `CppAPI/PublicAPI.h` into `external/skyrimnet/` (git-ignored). Pin the release
    tag in a single variable.
  - xmake: define `SKYRIMNET_API_AVAILABLE` and add the include path when the header exists; build
    must pass with and without it.
  - Acceptance: two CI runs green (with header, with the step disabled); `.gitignore` covers
    `external/skyrimnet/`; no SkyrimNet file committed.
- [ ] **T0.3** Baseline session: both mods, unpatched — `Human`, M
  - Two rigs, MGO 4.0 or equivalent, SkyrimNet configured on both with a cheap fast model, Piper TTS.
  - Answer open questions 1–4 and 6 from `CLAUDE.md`; attach SkyrimNet and urSovngarde logs.
  - Acceptance: `TESTLOG.md` entry with each question answered yes/no/observed.
- [ ] **T0.4** API notes from `PublicAPI.h` — `Human`, S
  - Write `docs/bridge/SKYRIMNET-API-NOTES.md`: every exported function with signature, the
    callback registration function, the list of event type names it accepts, the API version
    macro, and whether any event carries audio. Do **not** commit the header.
  - Acceptance: the notes are enough for T2.1 to be written without the header in hand.
- [x] **T0.5** Install guide for the patched build — `AI`, S
  - `docs/bridge/INSTALL.md`: manual install from a CI artifact, launcher auto-update off, same build
    on both clients and the server, SkyrimNet installed from official pages, GM on host only.
  - Acceptance: a tester follows it without asking questions.

## Milestone 1 — Tier 1: AI follows ownership

- [x] **T1.1** `SkyrimNetBridge` service skeleton — `AI`, S
  - New `Code/client/Services/Generic/SkyrimNetBridge.{h,cpp}`, registered in `World` like other
    services. `IsActive()` true only when `SkyrimNet.esp` is in the load order. Resolve the
    blacklist faction (`SkyrimNet.esp|0x12DB`) once; log at info level either way.
  - Acceptance: builds; log line `SkyrimNet bridge: active/inactive` appears; zero behaviour change.
- [ ] **T1.2** Blacklist remote copies — `AI`, M — depends on T1.1
  - On every path that creates or re-enables a remote actor (`CharacterService::OnActorAdded` with a
    `RemoteComponent`, spawn-request handlers, `WaitingFor3D` completion) call
    `SkyrimNetBridge::OnActorBecameRemote(actor)` → `AddToFaction`.
  - Acceptance: builds; code review confirms every remote-creation path is covered (list them in the PR).
- [ ] **T1.3** Ownership transfer in both directions — `AI`, M — depends on T1.2
  - Hook the `RemoteComponent` ↔ `LocalComponent` swap: becoming owner → remove faction; losing
    ownership → add faction and request a deferred purge of that actor's queued SkyrimNet dialogue.
  - Acceptance: builds; PR lists the transfer paths touched; purge call compiles against the Papyrus
    native or is stubbed with a TODO if no per-actor purge exists.
- [ ] **T1.4** Survive faction sync and never send it — `AI`, S — depends on T1.2
  - `Actor::SetFactions`: after applying the owner's list, re-add the blacklist when `IsRemote()`.
  - Outgoing `FactionsContent`: strip the blacklist faction.
  - Acceptance: builds; unit-style test of the strip step if the encoding tests allow; diff in
    upstream files ≤ 15 lines, each marked `// SkyrimNet bridge:`.
- [ ] **T1.5** Remote player avatar blacklisted — `AI`, S — depends on T1.1
  - `OnActorAdded` with `IsRemotePlayer()` → add faction. Also re-add after `SetFactions` on players.
  - Acceptance: builds; behaviour documented in PLAN §3 step 1.6.
- [ ] **T1.6** Papyrus fallback micro-mod — `AI`, S
  - `GameFiles/SkyrimNetBridge/`: Spriggit source for `SkyrimNetBridge.esp` with one quest and a
    script that every 2 s scans actors within 4096 units and blacklists any for which
    `IsRemotePlayer()` is true (urSovngarde's native) and `SkyrimNet.esp` is loaded. Compiles with
    the MIT `SkyrimNetApi.psc` and urSovngarde's Papyrus headers.
  - Acceptance: `.psc` compiles in CI (add a papyrus-compiler step) and the ESP builds from Spriggit.
- [ ] **T1.7** Game Master on host only — `AI`, S — depends on T1.1
  - If `SkyrimNetApi.GetConfigBool` can read the GM toggle, warn via HUD message when GM is on and
    this client is not the host. If not discoverable, document the manual rule in INSTALL.md.
  - Acceptance: builds; message text reviewed.
- [ ] **T1.8** Tier 1 session — `Human`, M — depends on T1.2–T1.5, T0.5
  - Verify: each player can only converse with NPCs their game owns; the other player's avatar never
    speaks on its own; ownership handoff when walking apart/together; no crash in 30 min.
  - Acceptance: `TESTLOG.md` entry; issues filed for anything observed.

## Milestone 2 — Tier 2: text both ways

- [ ] **T2.1** Load SkyrimNet C++ API at runtime — `AI`, M — depends on T0.2, T0.4
  - `GetModuleHandleW(L"SkyrimNet.dll")` + `GetProcAddress`; verify API version; register callbacks;
    every callback enqueues to the game thread (urSovngarde's task/runner queue) and never touches
    game objects on SkyrimNet's thread.
  - Acceptance: builds with and without the header; callback registration logged; a `dialogue` event
    logs speaker form id and text at debug level.
- [ ] **T2.2** NPC lines out as subtitles — `AI`, M — depends on T2.1
  - `dialogue` callback → if speaker is local → `SubtitleRequest`. Receiver: hide timer (reading-time
    heuristic, ~2.5 s + 60 ms/char, capped), skip when `IsLineVoicedByLocalScene`.
  - Acceptance: builds; timer logic unit-tested; PR shows the exact reuse of `OnNotifySubtitle`.
- [ ] **T2.3** `PlayerSpeech` message `[protocol]` — `AI`, L — depends on T2.1
  - Encoding in `Code/encoding` with round-trip test; server handler routes to the target's owner
    and, when a config flag is set, to players in range; client sender on
    `dialogue_player_stt`/`dialogue_player_text`; target = event listener if present else crosshair
    or nearest NPC within 500 units.
  - Acceptance: tests green; protocol version bumped; PR title contains `[protocol]`.
- [ ] **T2.4** Inject speech at the owner — `AI`, M — depends on T2.3
  - On `NotifyPlayerSpeech`: resolve avatar and NPC by server id; call
    `SkyrimNetApi.RegisterDialogueToListener(avatar, npc, text)` through the Papyrus VM (see how
    `PapyrusService` resolves natives) or the C++ API equivalent if T0.4 found one.
  - Acceptance: builds; call path documented; failure modes logged (unknown ids, API missing).
- [ ] **T2.5** Memory alignment on non-owners — `AI`, M — depends on T2.2
  - On a relayed NPC line, call `RegisterDialogue(copy, text)` (or the memory-only alternative if
    T0.3/T1.8 showed it triggers a reply). Config flag to disable.
  - Acceptance: builds; flag documented; TESTLOG task added for verification.
- [ ] **T2.6** Listen-only avatar spike — `AI+Human`, M — depends on T2.4
  - Try the three candidates in PLAN §5, in order; record the outcome in `DECISIONS.md`; implement
    the winner.
  - Acceptance: a DECISIONS entry and a PR, or a documented blocker with the question posted to
    SkyrimNet's tracker.
- [ ] **T2.7** Scene suppression for relayed AI lines — `AI`, S — depends on T2.2
  - Acceptance: builds; reuses `IsLineVoicedByLocalScene`; no duplicate subtitle in a shared scene
    per T2.8.
- [ ] **T2.8** Tier 2 session — `Human`, M — depends on T2.2–T2.7
  - Verify: B reads A's NPC replies; A's NPC answers B by name when B speaks; memories on both sides
    contain the exchange (check SkyrimNet dashboard); no runaway replies.
  - Acceptance: `TESTLOG.md` entry.

## Milestone 3 — Tier 3: voice

- [ ] **T3.1** Audio source decision — `AI+Human`, S — depends on T0.4
  - Draft the feature request to Fateless.Ai (audio-rendered callback + listen-only flag) and post
    it; in parallel evaluate the TTS-proxy route with the TTS engines the testers can run.
    Record the choice in `DECISIONS.md`.
  - Acceptance: DECISIONS entry naming (a), (b) or (c) and why.
- [ ] **T3.2** TTS proxy — `AI`, M — if (b)
  - `Tools/bridge/tts_proxy/`: Python, OpenAI-compatible `/v1/audio/speech` passthrough to a
    configured upstream (XTTS / Zonos / Chatterbox / any), tees `(request text, voice, audio)` to
    `localhost:<port>` for the client; config file; README.
  - Acceptance: unit tests with a fake upstream; latency overhead measured < 20 ms in the test.
- [ ] **T3.3** `NpcAudio` message `[protocol]` — `AI`, L — depends on T2.3
  - Opus encode/decode dependency via xmake; message encoding + tests; server relays to players in
    range; sender side consumes the audio source chosen in T3.1.
  - Acceptance: tests green; protocol bumped; bandwidth measured in the test (< 5 KB/s per stream).
- [ ] **T3.4** Receiver playback — `AI`, M — depends on T3.3
  - Reassemble chunks → temp `.wav` under `Data/Sound/Voice/SkyrimNetBridge/` → `Actor::SpeakSound`
    on the copy; MFG Fix NG lip sync when present; delete temp files on completion/exit.
  - Acceptance: builds; file lifecycle documented; stop message handled.
- [ ] **T3.5** Pairing, ordering, interruption — `AI`, M — depends on T3.4
  - Pair audio to subtitle by `lineId`; play in order; on owner-side purge or ownership loss send
    `stop`; receiver stops playback.
  - Acceptance: builds; sequence diagram in the PR.
- [ ] **T3.6** Full session, 2 then 3 players — `Human`, L — depends on T3.4, T3.5
  - Acceptance: `TESTLOG.md` entries for 2 and 3 players; latency numbers (owner vs non-owner).

## Milestone 4 — Release

- [ ] **T4.1** Packaging — `AI`, S — depends on T3.6
  - `Tools/VR/make-release.ps1` variant producing a bridge build; INSTALL.md final.
- [ ] **T4.2** Upstream contribution — `Human`, S
  - Issue/PR on `OhLookItsDaOne/urSovngarde` referencing `VR_TODO.md` 1621–1640, with TESTLOG
    results.
