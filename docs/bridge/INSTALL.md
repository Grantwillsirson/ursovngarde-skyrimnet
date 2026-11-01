# Installing the patched build (urSovngarde + SkyrimNet bridge)

For testers who have never built the mod. Follow the steps in order on every PC. Where this guide says
"(unverified)", nobody has confirmed it on a real install yet: check it and tell us what you saw
(a comment on the issue or a `TESTLOG.md` entry).

## 1. What you are installing

- **The patched build** is urSovngarde (Skyrim VR co-op) built from this repository's `skyrimnet-bridge`
  branch or a PR branch. It replaces the stock urSovngarde programs. It does not contain SkyrimNet.
- **SkyrimNet** is a separate mod. You download it yourself from its official pages (step 2). It is never
  part of our download: its licence does not allow us to pass it on.

Two things must be true, or the test is worthless:

1. **Every player and the server run the same patched build.** Same zip, same version. The server compares a
   protocol id with each client at connect and refuses a client that does not match. Later bridge tasks add
   new network messages, so a stock client or stock server will not work with them.
2. **Every PC runs the same SkyrimNet version**, installed from SkyrimNet's own release page.

## 2. Prerequisites on each player PC

urSovngarde needs:

1. Skyrim VR **1.4.15**.
2. SKSE VR (from skse.silverlock.org, the Skyrim VR build).
3. VR Address Library for SKSEVR (on Nexus).
4. `uGridsToLoad=5` in `Documents\My Games\Skyrim VR\SkyrimPrefs.ini`. 5 is the default; the server refuses
   other values.
5. Recommended: VRIK (on Nexus). It is the body the other player sees.

SkyrimNet needs (from its README, Requirements section, Skyrim VR variants):

6. A CPU with **AVX2** (almost any PC CPU from the last 10 years).
7. The latest **Microsoft Visual C++ Redistributable** (x64), from Microsoft.
8. **powerofthree's Papyrus Extender**: https://www.nexusmods.com/skyrimspecialedition/mods/22854
   (take the file for Skyrim VR; whether that is a separate file on the page is (unverified)).
9. **PapyrusUtil**: https://www.nexusmods.com/skyrimspecialedition/mods/13048 (VR file: (unverified)).
10. **powerofthree's Tweaks VR**: https://www.nexusmods.com/skyrimspecialedition/mods/59510, with its
    `Load EditorIDs` fix left **on** (the default).
11. **Skyrim VR ESL Support**: https://www.nexusmods.com/skyrimspecialedition/mods/106712 (instead of BEES).
12. **Skyrim VR Refocused**: https://www.nexusmods.com/skyrimspecialedition/mods/32737 (or a similar mod that
    keeps the game window focused).
13. **MFG Fix NG**: https://www.nexusmods.com/skyrimspecialedition/mods/133568
14. For the in-game chat and dashboard, either **Prisma UI**
    (https://www.nexusmods.com/skyrimspecialedition/mods/148718) plus its requirement **Media Keys Fix SKSE**
    (https://www.nexusmods.com/skyrimspecialedition/mods/92948), or **Magelight UI** (on Nexus).
    Prisma UI in VR: (unverified).
15. Optional: UIExtensions (on Nexus) for SkyrimNet's input wheel and its in-game configuration menu.
16. **A language model.** For testing: an OpenRouter key (https://openrouter.ai) or any OpenAI-compatible
    endpoint, or a Fateless account. A local model works too but is slower on a PC that also runs VR.
17. **SkyrimNet itself**: the latest release from https://github.com/MinLL/SkyrimNet-GamePlugin/releases,
    installed with your mod manager, `SkyrimNet.esp` enabled.
18. **Piper voice models** (the default voice engine, runs on the CPU): a separate download, linked from
    SkyrimNet's README under "Install". Install them as their own mod.

Both players should run the same mod list. Different setups cause desyncs that have nothing to do with the
bridge.

## 3. Getting a build

There is no published release of the patched build. You get it from GitHub Actions on
https://github.com/Grantwillsirson/ursovngarde-skyrimnet.

**Which workflow makes a usable download.** Only one does it in the layout this guide uses:

- **"Build windows"** (`windows.yml`) runs on every PR and push. It only compiles and runs the tests. On a PR
  or push it uploads **nothing**. Do not look for a download there.
- **"Create GitHub Release"** (`release.yml`) can be run by hand: Actions tab, "Create GitHub Release",
  "Run workflow", pick the branch to test. Run by hand it publishes nothing; it uploads one artifact named
  **`urSovngarde-release`**. Someone with write access to the repo has to start it. Whether it appears in
  the fork's Actions list before its first run is (unverified); if it does not, ask the repo owner.
- "Playable Skyrim Together Build" (`windows-playable-build.yml`) also uploads a zip, but in upstream Skyrim
  Together's old folder layout (`SkyrimTogetherReborn\`). Do not use it with this guide.

**Downloading.** Open the finished "Create GitHub Release" run, scroll to **Artifacts**, download
`urSovngarde-release` (you must be signed in to GitHub). Unzip it. Inside are three zips, made by
`Tools/VR/make-release.ps1`, named after the build version (`<version>` is a git describe string, for
example a commit hash such as `a1b2c3d4` when the fork has no tags):

| Zip | Holds | Who needs it |
| --- | --- | --- |
| `urSovngarde-<version>.zip` | `urSovngarde\` (the program that starts the game: `urSovngarde.exe`, `TPProcess.exe`, `EarlyLoad.dll`, the menu, `setup-connect.bat`, `update.bat`, `collect-logs.bat`), `Server\` (`urSovngardeServer.exe`, `STServer.dll`, `config\STServer.ini`, `host-server.bat`), `urSovngarde mod\` (the game files, `SkyrimTogether.esp`), the READMEs and licence files | Everyone, first install |
| `urSovngarde-<version>-update.zip` | `urSovngarde.exe` and `.pdb` only | Players updating an existing install |
| `urSovngarde-<version>-server.zip` | The `Server` folder only | The host, updating the server |

Every PC, the server included, uses zips from **the same run**. Write the version down; you will compare it
in step 7.

## 4. Installing the client

### 4a. Keep the stock launcher away from the patched files

There are two different things called "launcher":

- `urSovngarde.exe` in the `urSovngarde` folder. It starts Skyrim VR and loads the mod. It has **no update
  code**: the update source in this repository (`Code/immersive_launcher/update/Update.cpp`) is an empty file,
  and nothing in the client checks for new versions. Using it is safe.
- **The urSovngarde launcher app** (installs, updates, hosts with an invite code). It is a separate program
  that is not in this repository (`VR_TODO.md` line 15: it lives in its own repo). We cannot see how it
  updates, and we found no setting to stop it. Whether it has one is (unverified).

So, on every PC that runs the patched build:

1. Do **not** use the urSovngarde launcher app to install, update or play. Close it if it is running, and stop
   it from starting with Windows if it does that.
2. Install by hand (4b, 4c or 4d) and start the game with `urSovngarde.exe` (or a mod-manager executable
   pointing at it).
3. If the launcher app was used on this PC before, it may still overwrite the folders it manages when it next
   runs. Install the patched build into a **new** folder and point your mod manager at that (unverified
   whether the app touches folders it did not create).

### 4b. No mod manager (from `README-manual.md`)

1. Open `urSovngarde mod` and copy everything in it into `Skyrim VR\Data`, next to `Skyrim.esm`. Enable
   `SkyrimTogether.esp` in the plugin list.
2. Put the `urSovngarde` folder anywhere, for example inside `Skyrim VR`. Not inside `Data`.
3. Start the game with `urSovngarde.exe` from that folder, **not** with the SKSE loader: it starts the game and
   loads SKSE (and so SKSE plugins such as SkyrimNet) itself. The first start asks where Skyrim VR is.

### 4c. Mod Organizer 2 or a Wabbajack list (from `README-mod-manager.md`)

1. Install the folder `urSovngarde mod` as a mod (drag it onto the mod list, or zip it and use "Install a new
   mod"). Tick it, and tick `SkyrimTogether.esp` in the plugin list.
2. Copy the `urSovngarde` folder into the list's `tools\` folder (or anywhere).
3. Add an executable in MO2: title `urSovngarde`, binary `urSovngarde.exe` from that folder. Start the game
   with it from now on, not with SKSE.

### 4d. Vortex (from `README-vortex.md`)

1. Zip the folder `urSovngarde mod`, then in Vortex: Mods, Install From File, pick the zip, enable it. In
   Plugins enable `SkyrimTogether.esp`. Deploy.
2. Put the `urSovngarde` folder anywhere, for example next to `SkyrimVR.exe`. Not inside `Data`.
3. Start the game with `urSovngarde.exe` (double-click, or Add Tool on the Vortex dashboard). Never start the
   SKSE loader directly.
4. A Purge in Vortex removes the mod from `Data`; deploy again before playing.

### 4e. All setups: set the server address

1. Run `setup-connect.bat` once in the `urSovngarde` folder and type the host's address (`IP:10578`), and the
   password if the host set one. The host types `127.0.0.1:10578`. It is saved to
   `%LOCALAPPDATA%\SkyrimTogetherVR\connect.txt`.
2. In game, load a save and press **F6** to connect (the game shows "press F6 to connect to ..."). F6 again
   disconnects.

### 4f. Updating to a newer patched build

Close the game and drag the new `-update.zip` onto `update.bat` in the `urSovngarde` folder. By hand: replace
`urSovngarde.exe` and `urSovngarde.pdb`. If the release notes or PR say the network messages or game files
changed, use the full zip instead and update the server too.

## 5. Running the server (host only, from `README-host.md`)

The server must be `urSovngardeServer.exe` and `STServer.dll` from **the same run** as the clients. A stock
server refuses patched clients whose network messages differ, and the other way round.

**Own server (use this for bridge tests).**

1. Put the `Server` folder from the zip anywhere on the host PC. It does not need the game.
2. Run `host-server.bat` before playing and leave the window open. It starts the server on UDP **10578**
   (`uPort` in `Server\config\STServer.ini`) and prints the address to give the others.
3. Let the others reach it: forward UDP 10578 on the router to this PC (and allow it in Windows Firewall), or
   put everyone on a virtual LAN (Radmin VPN, ZeroTier, Tailscale) and give them that address.
4. Optional password: set `sPassword=` in `Server\config\STServer.ini` and tell the others.
5. The host connects to `127.0.0.1:10578`.

Leave the other settings in `STServer.ini` as shipped (`uMaxPlayerCount=8`, PvP off, difficulty 2) unless a
test says otherwise. The server rewrites the file at its first start.

**Host with an invite code.** This is a feature of the urSovngarde launcher app and its relay. It is not
supported for the patched build: whether the app would start our server, or replace it with the stock one,
is (unverified). Use your own server.

## 6. SkyrimNet settings for co-op

1. **Same SkyrimNet version on all PCs.** Check the version in its dashboard or the release you downloaded.
2. **Game Master on the host only.** The Game Master is SkyrimNet's agent that starts conversations by itself.
   With it on in two games, both start scenes for the same NPCs. Turn it **ON on the host's PC** and **OFF on
   every other PC**. Where to switch it:
   - SkyrimNet's in-game "SkyrimNet Configuration Utility" menu has "Enable Game Master Agent" (needs
     UIExtensions; how to open that menu is (unverified)).
   - In the dashboard: the setting is `gamemaster.agentEnabled` in the `Game` config; its exact page in the
     dashboard is (unverified).
   The bridge does not check this for you (decision D9 in `DECISIONS.md`): it is a rule you follow by hand.

   **Game Master: the rule.** In co-op the **host** is the **party leader** (the player who created the
   party in urSovngarde and drives quests and scenes). Only the party leader's PC runs the Game Master:
   - Party leader: Game Master **ON** (or off, if you want no invented events at all).
   - Every other player: Game Master **OFF**, before you join the party.
   - If leadership changes (the leader leaves, or hands leadership over), the new leader may turn it ON
     and the old leader turns it OFF.
   - Why: the Game Master invents world events and spawns things. urSovngarde syncs spawns, so with two
     Game Masters both players get both sets of events. Nothing breaks, but events double up.
   - Check it before every session: SkyrimNet keeps the setting between sessions, so a PC that ever
     played solo with the Game Master on still has it on.
3. **Model for testing:** pick a fast, cheap model so replies come quickly. The setup wizard offers presets
   (Budget, Balanced, Performance); Budget or Balanced is fine for tests. Use the same preset on all PCs.
4. **Voice for testing:** Piper (CPU, default), with the Piper voice models installed (step 2.18).
5. **The dashboard** is at http://localhost:8080 in a browser on the same PC, while the game is running. The
   first time, follow its setup wizard.

## 7. First-launch checklist

1. Each PC: game reaches the main menu with both mods loaded (SkyrimNet and `SkyrimTogether.esp` both in the
   load order). (unverified that the two load together without trouble: that is test T0.3, question 1.)
2. `urSovngarde\logs\tp_client.log` first lines say `Skyrim Together client, build <version>`: the same
   version on every PC, matching the zip names.
3. http://localhost:8080 opens on each PC; the SkyrimNet version is the same on each.
4. Game Master: on for the host, off for the others.
5. Host started `host-server.bat` from the same build; its window shows the server running.
6. Each player loads a save and presses F6; the host connects first.
7. Both players see each other in the same place (start somewhere simple, such as Whiterun).
8. One player speaks to an NPC; the NPC answers through SkyrimNet in that player's game.
9. Note what the other player sees and hears; write it in `TESTLOG.md`.
10. After the session: run `collect-logs.bat` in the `urSovngarde` folder on each PC, and copy `SkyrimNet.log`
    (below). Attach both.

**Where the logs are.**

| Log | Path |
| --- | --- |
| urSovngarde client | `urSovngarde\logs\tp_client.log` (rotated: `tp_client*.log`), next to `urSovngarde.exe` |
| urSovngarde menu (CEF) | `urSovngarde\logs\cef_debug.log` |
| urSovngarde crash dumps | `urSovngarde\logs\crash_*.dmp`; SKSE crash logs in `Documents\My Games\Skyrim VR\SKSE\crash-*.log` |
| urSovngarde server | `Server\logs\STServerOut.log` (the server writes `logs\` in its working folder; `host-server.bat` sets that to `Server`) |
| All of the above, zipped | `collect-logs.bat` in the `urSovngarde` folder puts a zip on your Desktop (it does not include SkyrimNet's log) |
| SkyrimNet | `Documents\My Games\Skyrim VR\SKSE\SkyrimNet.log` (SkyrimNet documents the SE path `Documents/My Games/Skyrim Special Edition/SKSE/SkyrimNet.log`; the VR folder is (unverified)). There is also a `SKSE\Plugins\SkyrimNet\logs\` folder inside the SkyrimNet mod (unverified what it holds). |
| Papyrus | `Documents\My Games\Skyrim VR\Logs\Script\Papyrus.0.log`, if Papyrus logging is on in `SkyrimVR.ini` (unverified for this setup) |

## 8. Uninstall, or go back to stock urSovngarde

To go back to stock urSovngarde:

1. Close the game and the server.
2. Get the official urSovngarde release from wherever you got stock urSovngarde (its GitHub releases page or
   Nexus page) and install it the same way as in step 4, replacing the `urSovngarde` folder and the
   `urSovngarde mod` files. Quick way: drag the official `-update.zip` onto `update.bat`; that replaces
   only `urSovngarde.exe` and `.pdb`, which is enough while the game files are unchanged. Use the full zip if
   unsure.
3. The host replaces `urSovngardeServer.exe` and `STServer.dll` with the official ones (or the official
   `-server.zip`). Everyone must be back on stock, the server included, or connects are refused.
4. You may use the urSovngarde launcher app again from here on.

To remove urSovngarde entirely: delete the `urSovngarde` and `Server` folders, and remove the `urSovngarde mod`
files from `Data` (or disable/remove the mod in MO2 or Vortex). Saves record `SkyrimTogether.esp`; loading one
without it shows the usual missing-plugin warning.

SkyrimNet is unaffected by any of this: uninstall it through your mod manager like any other mod if you want.

**Saves made during co-op with SkyrimNet.** The bridge puts every NPC copy (and the other player's avatar)
in `SkyrimNet_ActorBlacklistFaction`, and the game saves that like any other faction. If you load such a save
*without* the bridge (plain SKSE loader, or stock urSovngarde), NPCs that were copies when it was saved may be
silently ignored by SkyrimNet. Fix: load the save once through this build's `urSovngarde.exe` (the bridge
repairs local NPCs on sight), or remove the blacklist per NPC with SkyrimNet's wheel menu
(`skynet_WheelMenu.psc`).
