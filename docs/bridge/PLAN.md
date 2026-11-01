# Design: SkyrimNet inside urSovngarde co-op

Status: approved for implementation, 2026-11-01. Changes to this document go through a PR with a
`DECISIONS.md` entry.

## 1. The rule everything follows

**AI follows ownership.** urSovngarde already decides, every moment, which client simulates each
NPC (the *owner*; everyone else holds a *copy* whose AI loop never runs). The bridge makes the owner's
SkyrimNet the NPC's only mind, and makes every other player a participant in that mind's
conversation instead of the owner of a second one.

Consequences:

- An NPC is blacklisted for SkyrimNet in every client except its owner (Tier 1).
- A player's speech to an NPC travels to the NPC's owner and is registered there as a line spoken
  by that player's avatar, which is a real actor in the owner's game (Tier 2).
- The owner's reply travels back as text (Tier 2) and audio (Tier 3) to everyone in range.
- Every relayed line is also registered in the non-owners' SkyrimNet memories, so memories stay
  aligned (Tier 2).
- Ownership moves with proximity, so when players separate, each keeps talking to the NPCs around
  them; when they are together, one game owns the NPC in front of them.

Why not "host runs all the AI": NPC actions (follow, walk over, give item) execute only in the
owner's game, and the host does not own NPCs standing next to the guest three towns away.
Ownership-bound AI keeps actions working everywhere and degrades gracefully.

## 2. Data flow (Player B talks to Hulda, owned by Player A)

```
B's game                       urSovngarde server/relay            A's game (owns Hulda)
--------                       -----------------------            ---------------------
B speaks; local SkyrimNet
STT -> text ("dialogue_player_stt"
callback)                 ----> PlayerSpeech{text, B, Hulda} ---->  route to Hulda's owner
                                                                     RegisterDialogueToListener(
                                                                       B's avatar, Hulda, text)
                                                                     Hulda's SkyrimNet replies:
                                                                     LLM text + TTS audio
                                                                     (A hears it locally)
subtitle shown; audio played  <---- SubtitleRequest (exists) <-----  "dialogue" callback -> text
on B's copy via SpeakSound;   <---- NpcAudio chunks (new)   <-----  audio bytes (Tier 3 source)
RegisterDialogue(Hulda copy,
text) into B's memory
```

A player talking to an NPC their own game owns is the ordinary solo path; only the outgoing
subtitle/audio is relayed.

## 3. Tiers

### Tier 0 — setup (no gameplay change)

- CI builds the fork; `skyrimnet-bridge` added to workflow triggers.
- CI fetches SkyrimNet's release zip and extracts `CppAPI/PublicAPI.h` to a git-ignored include
  directory. The bridge compiles only when the header is present (`SKYRIMNET_API_AVAILABLE`), so a
  build without it still succeeds and the bridge is simply absent.
- Baseline human test with both mods, unpatched. Answers the open questions in `CLAUDE.md`.
- Install guide for the patched build: manual install, launcher auto-update off, both players and
  the server on the same build.

### Tier 1 — AI follows ownership (no protocol change)

Component: `Code/client/Services/Generic/SkyrimNetBridge.{h,cpp}` (new), registered like other
services in `World`.

| Step | Where | What |
| --- | --- | --- |
| 1.1 Detect | service init | `IsActive()` = `SkyrimNet.esp` in load order (ModSystem). Resolve faction form `SkyrimNet.esp|0x12DB` once. |
| 1.2 Blacklist copies | `CharacterService` spawn path for remote entities (`OnActorAdded` with `RemoteComponent`; `OnAssignCharacter`/spawn-request handlers) | `AddToFaction(blacklist)` when `GetExtension()->IsRemote()`. |
| 1.3 Ownership change | wherever `RemoteComponent` ↔ `LocalComponent` is swapped (ownership transfer handlers in `CharacterService`) | becoming owner → `RemoveFromFaction`; losing ownership → `AddToFaction` and `SkyrimNetApi.PurgeDialogue(true)` for that actor's queued lines if the API allows per-actor purge, else global deferred purge. |
| 1.4 Survive faction sync | `Actor::SetFactions` (`Actor.cpp` ~912) | after the owner's list is applied, re-add the blacklist if `IsRemote()`. |
| 1.5 Never send it | wherever `FactionsContent` is built for outgoing updates | strip the blacklist faction so an owner never receives it from a copy. |
| 1.6 Remote player | `OnActorAdded` when `IsRemotePlayer()` | `AddToFaction(blacklist)`: the other player's avatar is never an AI NPC. (Tier 2 revisits this: the avatar must be *heard* by the owner's SkyrimNet, see §5.) |
| 1.7 Game Master guard | service init, via `SkyrimNetApi.GetConfigBool` if the key is discoverable | warn in the HUD if GM is on and this client is not the host. |

Fallback that needs no C++ rebuild: a tiny ESP + quest script that polls nearby actors every 2 s and
blacklists any for which urSovngarde's Papyrus native `IsRemotePlayer()` is true. Ships as a separate
micro-mod (`GameFiles/SkyrimNetBridge/`). Covers step 1.6 only.

Known risk: faction churn. Each `SetFactions` briefly exposes the copy between
`RemoveFromAllFactions()` and our re-add. If SkyrimNet samples at that instant it may start a reply.
Mitigation if observed: stamp copies with a keyword as well and extend SkyrimNet's YAML actor filter
to honour it (keywords are not in the synced faction list).

### Tier 2 — text both ways (one new message)

| Step | What |
| --- | --- |
| 2.1 API load | `GetModuleHandleW(L"SkyrimNet.dll")` + `GetProcAddress` for the public API entry points; check API version ≥ the one `PublicAPI.h` was taken from; all callbacks marshal to the game thread via urSovngarde's task queue. |
| 2.2 NPC line out | callback on `dialogue` → if speaker `IsLocal()` → `SubtitleRequest{serverId, text}` (existing message). Receiver: `OnNotifySubtitle` already shows it; add a hide timer (no voice line ends an AI subtitle) and skip when `IsLineVoicedByLocalScene`. |
| 2.3 Player speech out | callback on `dialogue_player_stt` / `dialogue_player_text` → **new** `PlayerSpeech{text, speakerServerId, targetServerId?}`; target = SkyrimNet's chosen listener if the event carries one, else the crosshair/nearest NPC. Server routes to the target's owner; optionally also to players in range as a subtitle so friends can read what you said. **Protocol bump.** |
| 2.4 Inject at owner | on `PlayerSpeech` → `SkyrimNetApi.RegisterDialogueToListener(avatarActor, npcActor, text)` (call the Papyrus native from C++ through the VM, or the C++ API equivalent if one exists). |
| 2.5 Memory alignment | on a relayed NPC line, non-owner calls `RegisterDialogue(copyActor, text)`. **Unverified:** whether this makes the copy's SkyrimNet answer. If it does, use `RegisterPersistentEvent` or a memory-only path instead. |
| 2.6 Listen-only avatar | see §5. |

### Tier 3 — voice (new message + an audio source)

Audio source options, decided in T3.1:

| Option | How | Cost |
| --- | --- | --- |
| (a) SkyrimNet audio callback | Feature request to Fateless.Ai: an event carrying (speaker, text, audio bytes or path) when a line is rendered. | Cleanest; outside our control. |
| (b) TTS proxy | Point SkyrimNet's TTS at a network endpoint (XTTS, Zonos, Chatterbox, or any OpenAI-compatible `/v1/audio/speech`) through a small proxy we ship. The proxy tees every (text, voice, audio) to the local client over localhost; the `dialogue` callback supplies the speaker. | No closed-source dependency. Loses in-process Piper (needs a GPU TTS server, local or rented). |
| (c) Audio-manager hook | Detour the engine's sound-buffer submission and capture SkyrimNet's playback. | Fragile across SkyrimNet updates; last resort. |

Transport: `NpcAudio{speakerServerId, lineId, seq, opusChunk}` relayed to players in range; Opus at
~24 kbps ≈ 3 KB/s per speaking NPC. Receiver writes a temp `.wav` under a bridge-owned voice path and
plays it on the copy with `Actor::SpeakSound(path)` (existing code path, see `CharacterService.cpp`
~2355), lip sync through MFG Fix NG where available. Pair audio to subtitle by `lineId`.
Interrupt: on ownership loss or `PurgeDialogue` at the owner, send a stop for that `lineId`.

Latency for the non-owner: owner's generation time + one relay hop each way (30–150 ms) + Opus
(<50 ms). The owner hears the line a fraction of a second before everyone else.

### Tier 4 — release

Packaging via `Tools/VR/make-release.ps1`, install docs, and an upstream issue/PR to
`OhLookItsDaOne/urSovngarde` referencing their `VR_TODO.md` entry.

## 4. Messages (protocol)

| Message | Direction | Fields | Status |
| --- | --- | --- | --- |
| `SubtitleRequest` / `NotifySubtitle` | client → server → clients | serverId, text, topicFormId, sceneId, scenePhase | exists |
| `PlayerSpeech` / `NotifyPlayerSpeech` | client → server → owner (+ in-range) | speakerServerId, targetServerId (0 = none), text, lineId | **new, Tier 2** |
| `NpcAudio` / `NotifyNpcAudio` | owner → server → in-range clients | speakerServerId, lineId, seq, flags(last/stop), opus bytes | **new, Tier 3** |

Encoding lives in `Code/encoding`; add round-trip tests in `Code/tests` like the existing messages.

## 5. The listen-only avatar problem

Tier 1 blacklists the remote player's avatar so it is never an AI NPC. Tier 2 needs the owner's
SkyrimNet to *hear* lines attributed to that avatar and let NPCs reply to it by name. The blacklist
is "no talk, memories or actions" — probably too broad. Candidates, to be settled by a spike (T2.6):

1. A SkyrimNet YAML filter or config that marks an actor as human-driven (never generated for, but
   a valid speaker). Ask on SkyrimNet's Discord / open an issue; this may already exist.
2. Keep the avatar blacklisted and register the line through `RegisterDialogueToListener` anyway —
   test whether the blacklist blocks *being heard* or only *speaking*.
3. A Virtual NPC named after the remote player, lines injected with `RegisterDialogueByUUID`.
   Risk: the virtual NPC itself starts talking.

## 6. Things verified not to conflict (so nobody re-investigates)

- No `libcef.dll` collision: urSovngarde initialises CEF in-process; Prisma UI (SkyrimNet's UI) is
  Ultralight-based.
- No shared engine detours found (SkyrimNet is described as reading memory, not hooking; its DLL is
  closed so this is inferred).
- urSovngarde maps the game at a fixed base; SkyrimNet uses Address Library (base-relative).
- No ESP record overlap. urSovngarde translates form ids across load orders (`GetServerModId`).
- Ports: SkyrimNet 8080/8889 TCP local; urSovngarde UDP 10578.
- Launcher DLL blocklist does not include SkyrimNet or its dependencies.

## 7. Risks, ranked

1. Faction churn (Tier 1) — see mitigation above.
2. Ownership flapping when players crowd one NPC — pin ownership for a conversation's duration
   (party-leader rule already does this for scenes).
3. Scene double-voicing — reuse `IsLineVoicedByLocalScene`.
4. Unknown SkyrimNet hooks — only a launch can rule it out (T0.3).
5. Two Game Masters — config guard (1.7).
6. `RegisterDialogue` on the non-owner triggering a reply (2.5) — test early.
