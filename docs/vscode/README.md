# VSCode IDE configuration

If you want to use VSCode to configure and launch this project, there's a ready-made `.vscode` sample folder here.

* [IDE configuration JSON files](#ide-configuration-json-files)
* [Using a specific settings.json for your operating system](#using-a-specific-settingsjson-for-your-operating-system)
* [VS Code Settings for Mac Windows and Linux extension](#vs-code-settings-for-mac-windows-and-linux-extension)

## IDE configuration JSON files

Copy this whole folder's content into a `.vscode` folder at the repo root:

```shell
cd game-engine
mkdir -p .vscode
cp docs/vscode/.vscode/* .vscode/
```

The root `.vscode` folder is gitignored — it's per-developer/per-machine local state, not tracked. This sample folder is the tracked source of truth; **any future `.vscode` change you want to keep should be copied back here**, not just left in your local, untracked copy.

Every debug configuration in `launch.json` has a `preLaunchTask` (`tasks.json`'s `cmake build (scarab)`) that runs `cmake --build build` before launching. This matters beyond just recompiling: this project's build also copies Lua scripts/configs/sprites/audio into `build/resources/` as part of that same build step (see `CLAUDE.md`'s Build section) — without the `preLaunchTask`, hitting Run/Debug just launches whatever binary is already sitting at `build/scarab` (or `build/debug/scarab.exe` on Windows) as-is, which can silently run against **stale copied resources** if you've edited any Lua/config/asset file since the last real build. Added 2026-08-17 after confirming this gap existed (no config in this folder ran a build before launching) while investigating an unrelated, still-unconfirmed live-bug report — the gap itself is real and worth closing regardless of whether it turns out to explain that specific report.

## Using a specific settings.json for your operating system

`settings.json` itself is kept OS-neutral. Three ready-made per-OS variants exist alongside it:

* `settings.linux.json` — Linux;
* `settings.macos.json` — macOS;
* `settings.windows.json` — Windows (points `cmake.configurePreset` at the `windows-vcpkg` preset in the repo's `CMakePresets.json`, which is where the actual vcpkg toolchain wiring lives — see `CLAUDE.md`'s Build section).

If you're not using the extension below, just copy the matching one over `settings.json` in your local `.vscode` folder:

```shell
cd game-engine/.vscode
cp settings.macos.json settings.json
```

## VS Code Settings for Mac Windows and Linux extension

Alternatively, install [VS Code Settings for Mac Windows and Linux](https://marketplace.visualstudio.com/items?itemName=franmastromarino.vs-code-settings-os) (already listed in `extensions.json`'s recommendations) and restart VSCode. It automatically swaps the matching `settings.<os>.json` into `settings.json` based on your detected host OS, and keeps doing so on every edit to the `.json` variant files.
