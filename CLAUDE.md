# SkyrimNet × urSovngarde bridge — brief for AI sessions

Read this first. It is written for any Claude (or other agent) session working in this repo,
whoever is paying for the tokens. Humans: it is a fine summary for you too.

## What this repo is

A fork of [urSovngarde](https://github.com/OhLookItsDaOne/urSovngarde) (Skyrim VR co-op, a port of
Skyrim Together Reborn, GPLv3) that adds a **compatibility bridge** so that
[SkyrimNet](https://github.com/MinLL/SkyrimNet-GamePlugin) (AI NPC dialogue) works in co-op:

- every NPC has exactly **one** AI mind (the game that *owns* the NPC runs it),
- both players can **talk to** that NPC, and both **read and hear** its replies,
- both games' SkyrimNet memories receive the same lines,
- nothing changes for players who do not have SkyrimNet installed.

The goal, in the project owner's words: *both players interactable and in the same world, NPCs
remembering their scripted dialogue and conversation history, responding quickly and naturally in
the character's voice, with no activation step.*

All planning lives in `docs/bridge/`. Start with `docs/bridge/README.md`.

## Vocabulary (urSovngarde's, use it exactly)

| Term | Meaning |
| --- | --- |
| **owner** / **local** | The one client whose game simulates an actor. `ActorExtension::IsLocal()`. |
| **copy** / **remote** | The puppet of that actor in every other client. `IsRemote()`. Its `Actor::Process` is skipped (`Code/client/Games/Skyrim/Actor.cpp`, `HookActorProcess`): no AI runs on a copy, position and state arrive over the network. |
| **remote player** | The other player's avatar in this game: a real `Actor` with a `0xFF`-range form id. `IsRemotePlayer()`. |
| **server id** | Network-wide id of an entity (`Utils::GetServerId`). Form ids differ between clients; server ids do not. |
| **party leader** | Drives quests and scenes for the party. |

## Architecture in one paragraph

urSovngarde is a native DLL plus launcher, 119 engine detours (`TP_HOOK`), an EnTT world, and UDP
netcode through `urSovngardeServer.exe`. SkyrimNet is a closed-source SKSE plugin that reads game
memory on worker threads and exposes a Papyrus API (`SkyrimNetApi.psc`, MIT), a C++ API
(`CppAPI/PublicAPI.h`, ships in the SkyrimNet release zip, **not** in their repo) and a YAML
filter system. The bridge is C++ inside urSovngarde's client, plus a small Papyrus/ESP add-on, plus
(for voice) new network messages and possibly a Python TTS proxy. See `docs/bridge/PLAN.md`.

## Hard rules

1. **Never commit SkyrimNet files.** Not `SkyrimNet.dll`, not `SkyrimNet.esp`, not
   `CppAPI/PublicAPI.h`. SkyrimNet's license forbids redistribution. CI fetches the header from the
   official release at build time; developers drop it into a git-ignored path locally. The Papyrus
   API (`SkyrimNetApi.psc`) is MIT and may be copied.
2. **The bridge is a no-op when `SkyrimNet.esp` is not loaded.** Every bridge code path checks
   `SkyrimNetBridge::IsActive()` first. urSovngarde without SkyrimNet must behave exactly as upstream.
3. **No protocol change without an issue that says "protocol".** New or changed messages in
   `Code/encoding` / `Code/server` mean every client and the server must run the same build. Bump
   the protocol version in the same PR and say so in the PR title.
4. **Do not claim anything was tested in-game unless `docs/bridge/TESTLOG.md` has the entry.**
   Nobody can run Skyrim VR in a cloud session. Code is *written* here and *built* by GitHub Actions
   on Windows; it is *tested* by a human with two VR rigs. Say which of the three you did.
5. **Keep upstream mergeable.** `main` mirrors upstream `OhLookItsDaOne/urSovngarde`; all work goes
   on `skyrimnet-bridge` through PRs. Prefer new files (`Code/client/Services/Generic/SkyrimNetBridge*`)
   over edits inside upstream files; where an upstream file must change, keep the diff small and
   comment it `// SkyrimNet bridge:`.
6. **One issue, one branch, one PR.** Branch name `t<id>-<slug>` (e.g. `t2.3-playerspeech`). Claim an
   issue by assigning yourself and commenting which session/person has it, before starting.
7. **Decisions go in `docs/bridge/DECISIONS.md`**, not in chat. If you have to choose between
   designs, write the entry, then code.

## How to work on a task

1. `git fetch`, branch from `skyrimnet-bridge`.
2. Read the issue, then the files it names. The task list (`docs/bridge/TASKS.md`) has acceptance
   criteria and dependencies; respect the dependencies.
3. Build cannot run locally on Linux/macOS. Push the branch and open a PR: `.github/workflows/windows.yml`
   compiles on `windows-2022`. A red build is your problem to fix; read the log.
4. In the PR description: what changed, which rule-4 category (written / built / tested), what the
   human tester should check, and any decision you recorded.
5. Tick the task in `TASKS.md` in the same PR.

Build facts: Windows 10/11 x64, Visual Studio 2022, xmake ≥ 3.1, Node 20 + pnpm (for the CEF menu
only). `xmake config --plat=windows --arch=x64 --mode=release --yes && xmake -y`. Outputs in
`build/windows/x64/release`. Papyrus compiles with `papyrus-compiler`; the ESP from Spriggit source.

## Where the bridge touches urSovngarde (verified against upstream at 2026-10-30)

| Concern | File | Notes |
| --- | --- | --- |
| Remote/local/player flags | `Code/client/Games/ActorExtension.h` | `kRemote`, `kPlayer` bits. |
| Remote AI suppression | `Code/client/Games/Skyrim/Actor.cpp` `HookActorProcess` | Copies never run `Actor::Process` (except corpses/knocked). |
| Faction sync | `Code/client/Games/Skyrim/Actor.cpp` `Actor::SetFactions` (~l. 912) | Calls `RemoveFromAllFactions()` then applies owner's list — **wipes any faction we add locally**. Callers in `CharacterService.cpp` (~l. 1401, 1749, 3760). |
| Vanilla voice-line relay | `Actor.cpp` `HookSpeakSoundFunction` (~l. 1612) → `DialogueEvent` → `DialogueRequest` (file **name** only) | Receiver replays the same `.fuz` from its own data (`CharacterService.cpp` ~l. 2355). Useless for runtime TTS. |
| Subtitle relay | `CharacterService::OnSubtitleEvent` / `OnNotifySubtitle` (~l. 2367–2410), `SubtitleRequest` | Text-only relay that **exists today** — Tier 2 reuses it. |
| Scene double-voicing guard | `IsLineVoicedByLocalScene` in `CharacterService.cpp` | Reuse for relayed AI lines. |
| Remote player spawn | `CharacterService::OnActorAdded` (~l. 833), `SetPlayer(true)` | Avatar is a plain `Actor`. |
| Papyrus natives exposed | `Code/client/Games/PapyrusFunctions.cpp` | Only `IsRemotePlayer(Actor)` and `IsPlayer(Actor)`. No `IsRemote` for copies. |
| Package hooks | `Code/client/Games/References.cpp` | `AI_SYNC 0`: pass-through. Not the suppression point. |
| Launcher DLL blocklist | `Code/immersive_launcher/stubs/DllBlocklist.cpp` | SkyrimNet and its dependencies are **not** blocked. |
| Server player cap | `Code/server/GameServer.cpp` `uMaxPlayerCount` (default 8) | Nothing is two-player-specific. |

Maintainer's own sketch of this integration: upstream `VR_TODO.md` lines 1621–1640
("SkyrimNet with the mod"). Our plan is that sketch, extended.

## Where the bridge touches SkyrimNet (public surfaces only)

| Surface | Use |
| --- | --- |
| `SkyrimNet_ActorBlacklistFaction` (`SkyrimNet.esp` 0x12DB) | "No talk, memories or actions" for an actor. Tier 1 puts copies and the remote player in it. |
| `plugins/skyrimnet/base/filters/skyrimnet_filter_factions.yaml` | YAML actor filter; whitelist/blacklist by faction. Possible keyword-based extension. |
| `SkyrimNetApi.psc` (MIT) | `RegisterDialogue`, `RegisterDialogueToListener`, `PurgeDialogue`, `TriggerToggleGlobalAI`, `GetConfigBool`, `IsRunningVR`, Virtual NPC functions. |
| C++ API `PublicRegisterEventCallback(eventType, cb)` (per upstream maintainer's note; API v12) | Event types of interest: `dialogue` (speaker, dialogue, listener), `dialogue_player_stt`, `dialogue_player_text`. Runs on SkyrimNet's threads — **queue to the game thread**. |
| Event type list | SkyrimNet `docs/modding/WORKFLOW_TRIGGERS.md` table. |
| Audio | Rendered in-process, played through the game's audio manager. **No file, no API** today. See Tier 3 options in PLAN.md. |

## Open questions a human must answer with a real session (before Tier 2 code)

1. Does the game launch with both mods and reach the main menu?
2. With both players in Whiterun, does Player B's SkyrimNet treat Player A's avatar as an NPC?
3. When A talks to Hulda and B stands beside, what does B see and hear?
4. Does `SkyrimNetApi.GetEntityUUID()` return something sane for a `0xFF` remote-player actor?
5. What exactly does `CppAPI/PublicAPI.h` expose (function list, event names, any audio)?
6. Can SkyrimNet *hear* an actor without ever *speaking for* it (listen-only)?

Record answers in `docs/bridge/TESTLOG.md` and update `DECISIONS.md`.

## Roles (how the owner wants sessions used)

- **Planning, review, decisions:** a high-end model session (the owner's "overseer"). Reviews PRs,
  writes DECISIONS entries, keeps TASKS.md honest.
- **Implementation:** cheaper model sessions at medium effort, one issue each, producing PRs.
- **Testing:** humans with Skyrim VR rigs. Everything else is unverified until they say otherwise.

Each session is independent; GitHub (issues, PRs, these docs) is the only shared memory.
