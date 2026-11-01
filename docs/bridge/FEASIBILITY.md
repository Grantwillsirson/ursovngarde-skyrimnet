# Feasibility summary (2026-11-01)

Condensed from the full report. Frozen once building starts; design detail is in `PLAN.md`.

## Verdict

Feasible. The two mods do not fight at the engine level; what breaks is gameplay coherence, because
each player's game runs a private SkyrimNet. urSovngarde's maintainer sketched this exact
integration in `VR_TODO.md` (lines 1621–1640, 2026-10-23, after a Mad God's Overhaul player asked)
and marked it "not started, low priority". The sketch holds against the code.

## Requirements vs status

| Requirement | Today | Closed by |
| --- | --- | --- |
| Both players in one world | Met by urSovngarde (only Skyrim VR co-op that exists) | — |
| Both talk to and hear the same NPCs | Not met: private SkyrimNet per game | Tiers 1–3 |
| NPC memory of scripted and freeform dialogue | Met solo; split in co-op | Tier 2 memory alignment |
| Fast, natural, character's voice | Met by SkyrimNet (in-process, streaming TTS, per-voice-type Piper models or cloning) | — (+0.1–0.4 s for non-owners) |
| No activation step | Met by SkyrimNet; failed by Mantella/CHIM | — |

## What breaks today, from the source

| # | Symptom | Root cause | Severity |
| --- | --- | --- | --- |
| 1 | Other player never hears/sees AI lines | SkyrimNet plays TTS via the audio manager; urSovngarde's only voice relay (`HookSpeakSoundFunction`) forwards a file name the other PC does not have | Critical |
| 2 | Other player's avatar treated as an NPC | Avatar is a plain `Actor` (0xFF form id); SkyrimNet auto-bios any actor | High |
| 3 | AI-driven NPC behaviour silently dropped on copies | `HookActorProcess` skips AI on remote actors; packages set but never evaluated | High |
| 4 | Divergent memories, doubled replies | Two SkyrimNet databases, two LLM sessions | High |
| 5 | Two Game Masters | Per-client feature | Medium (config) |
| 6 | Local blacklist faction wiped | `Actor::SetFactions` does `RemoveFromAllFactions()` on every sync | Medium |
| 7 | Input/overlay interference | Both hook input; overlays on different surfaces | Low, untested |

Ruled out: CEF collision (Prisma UI is Ultralight), shared detours (none found; SkyrimNet reads
memory), address-base sensitivity (Address Library is base-relative), ESP overlap, port overlap,
launcher DLL blocklist.

## Alternatives considered

| Option | Why not |
| --- | --- |
| Mantella | Explicit conversation sessions (activation); external Python server; its openness only eases the audio relay |
| CHIM | Same architecture inside WSL2; SKSE plugin source not confirmed public |
| New AI mod | Months to reach SkyrimNet's level; same relay afterwards |
| New co-op mod | urSovngarde embodies ~a decade of Tilted Online work; nothing else runs on VR |
| Fallout 4 VR | No VR co-op exists (Fallout Together stalled), no always-on AI mod exists |
| ESO / Fallout 76 private server | No server emulator exists; NPC logic is server-side; no VR; ToS |

## Effort (estimates, not measurements)

- Tier 1 + Tier 2 + speech-in: ~2 weeks of C++ for someone comfortable in Tilted-style code, then a
  test weekend on two rigs.
- Voice: 1–2 more weeks if SkyrimNet (or a TTS proxy) hands over the audio; an engine audio hook is
  the fragile fallback.
- Expect 3–6 build → test → fix rounds; nobody has run both mods together before.

## Sources

- `MinLL/SkyrimNet-GamePlugin`: README, LICENSE, `Source/Scripts/SkyrimNetApi.psc`,
  `plugins/skyrimnet/base/filters/skyrimnet_filter_factions.yaml`, `docs/modding/WORKFLOW_TRIGGERS.md`.
- `OhLookItsDaOne/urSovngarde`: `VR_TODO.md`, `KNOWN_ISSUES.md` §4–5, `Code/client/Games/Skyrim/Actor.cpp`,
  `Code/client/Games/ActorExtension.h`, `Code/client/Games/PapyrusFunctions.cpp`,
  `Code/client/Services/Generic/CharacterService.cpp`, `Code/client/Games/References.cpp`,
  `Code/client/Services/Generic/OverlayService.cpp`, `Code/immersive_launcher/stubs/DllBlocklist.cpp`,
  `Code/server/GameServer.cpp`.
- Mad God's Overhaul 4.0 docs (SkyrimNet section); PrismaUI-SKSE/framework (Ultralight).
