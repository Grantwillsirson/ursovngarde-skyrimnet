# SkyrimNet C++ API header in CI

SkyrimNet's licence forbids redistributing its files, so `CppAPI/PublicAPI.h` is never committed. Instead:

- **CI** (`.github/workflows/windows.yml`, step "Fetch SkyrimNet API header") reads the release from `MinLL/SkyrimNet-GamePlugin` named by the `SKYRIMNET_RELEASE` env variable at the top of the workflow (`latest` for now; **pin a tag** such as `vbeta29` for reproducible builds). It lists the release assets through the REST API with the runner's `GITHUB_TOKEN`, tries the SkyrimNet `.zip`/`.7z` assets (skipping Piper/voice/model packs; devkit/sdk/api archives first, since `vbeta29` ships `skyrimnet-beta29.zip` without headers and `skyrimnet-devkit-beta29.zip`), finds `PublicAPI.h` wherever it sits in the archive (a `CppAPI/` path is preferred), and extracts only that file to `external/skyrimnet/PublicAPI.h`. The step summary records the release, archive path and SHA-256. It fails loudly, listing what it saw, if there are no assets or no header.
- **Skipping it:** run "Build windows" by hand (Actions → Run workflow) with `fetch_skyrimnet_header` unchecked to prove the build passes without the header.
- **xmake** (`Code/client/xmake.lua`): when `external/skyrimnet/PublicAPI.h` exists, the client targets get `SKYRIMNET_API_AVAILABLE` and that include dir; either way xmake prints one `SkyrimNet bridge:` line per client target.
- **Locally:** download SkyrimNet from its official page yourself, copy `PublicAPI.h` out of its `CppAPI` folder into `external/skyrimnet/` (git-ignored), and re-run `xmake config`. Never `git add -f` it.
