# SkyrimNet C++ API notes (human-written; the header itself is never committed)

Fill this in from `CppAPI/PublicAPI.h` in the SkyrimNet release zip (task T0.4). Describe, do not
paste: function names and signatures in your own words are fine; copying the file is not.

## Version

- SkyrimNet release used: `<tag>`
- API version macro/value: `<n>` (upstream urSovngarde's note mentions "public API v12")

## Registration

- How a plugin gets the API (exported function? SKSE messaging? `GetProcAddress` on `SkyrimNet.dll`?):
- Callback registration function (name, parameters, return):
- Threading contract stated in the header (which thread callbacks run on):

## Event types the callback accepts

| Event type string | Payload fields | Notes |
| --- | --- | --- |
| `dialogue` | speaker, dialogue, listener (per docs) | |
| `dialogue_player_stt` | | |
| `dialogue_player_text` | | |
| … | | |

## Functions relevant to the bridge

| Function | Signature (in words) | Use |
| --- | --- | --- |
| e.g. PublicRegisterDialogue… | | Tier 2.4 inject |
| e.g. PublicSendCustomDecisionToLLM | | not needed |

## Audio

- Any event or function that exposes rendered audio bytes, a file path, or a "line started/ended"
  signal? (This decides Tier 3 option (a).)

## Anything surprising

-
