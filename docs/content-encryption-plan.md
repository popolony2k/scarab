# Content encryption plan

**Status: planning complete, implementation not started.** This document tracks a planned feature — encrypting a game's `.zip` bundle contents (Lua source *and* assets) so only that game's own privately-built `scarab` executable can decrypt it. Delete this file once the feature is fully implemented, tested, and documented elsewhere (see [Docs](#docs-checkpoint-6) below for where that permanent documentation belongs).

## Motivation

Today a game built on Scarab can already bundle its Lua source + assets into a single self-contained `.zip`, mounted at `APP_DIR` and run identically to a loose directory (`EngineHost::ResolveEntryScript`'s `.zip` branch) — already 100% functional, not something this plan changes. The gap: anyone can open that `.zip` with any standard tool and read the Lua source/assets in plaintext. This plan adds a layer so only two parties can ever see the real content: the game's own owner, and Scarab itself.

**Honesty check, a framing principle, not a blocker**: whatever scheme is built here is *deterrence*, not unbreakable DRM. The engine has to decrypt the content to run it, so a sufficiently motivated attacker can always extract the key from the running/compiled binary. Every major engine that does this (Unity, Godot, Unreal) frames it the same way — raises the bar well past "unzip with any tool," doesn't stop a dedicated reverse engineer. Build toward that realistic bar, not a promise crypto can't keep client-side.

## Core mechanism

**Shared-secret (symmetric) key**, not real public/private-key infrastructure — same key encrypts and decrypts.

This fits because **every game already privately compiles its own copy of `scarab`** — Scarab isn't a shared system runtime, `FetchContent` builds it fresh per-game (`project(scarab VERSION ...)`/`SCARAB_SUNLIGHT_VERSION` are already compiled-in string literals per game build, exactly the pattern a content key would use). So:

- **The owner** encrypts their content with their own key when packaging the `.zip`.
- **That same owner's own build of `scarab`** has the matching key compiled in, decrypts transparently at load time.
- A different company's build, or a generic public Scarab release, has a different (or no) key — the bundle is just noise to them.

## Scope: encrypt everything uniformly for v1

Lua source AND every asset type (json, fonts, audio, images) alike — no per-file-type policy. Encryption operates on raw bytes regardless of file type, so there's no extra complexity in covering everything, and it's the simplest thing to reason about ("this bundle is protected," full stop). Selective encryption (e.g. leave large audio assets in the clear for faster dev iteration) is a legitimate *later* refinement, not needed for v1.

## Tooling shape: `scarab` itself is the CLI

`scarab` becomes the packaging CLI (reusing the CLI11 parsing already in `main.cpp`), running a **Lua tool script** that calls new C++ crypto primitives exposed the same way every other subsystem is (`LuaCryptoApi`, matching `LuaSoundApi`/`LuaSpriteApi`/etc.'s own file-pair pattern). Architecturally consistent with "thin C++, everything else in Lua" — this engine's own core philosophy — rather than a separate Python/C++ toolchain.

## The `sunlight` cross-repo fork — folded into v1, not deferred

Every resource *Scarab itself* directly reads (Lua scripts via `LuaEngine::RunFile`, JSON via `LuaJsonApi::LoadJson`, `dofile` via `LuaFileSystemApi::DoFile`) goes through one path this repo owns. But **textures, sounds, and tilemaps are read by `sunlight`'s own code** (`RaylibEngine`'s hooked `SetLoadFileDataCallback`, `RayLibSound::Load`, `TileMapRenderer::LoadMap`), calling `sunlight`'s `IFileSystem::ReadFile` directly. Exactly the same repo-boundary shape as the multi-file BMFont loading bug fixed in sunlight v0.17.3.

**Decision: use the same cross-repo strategy that closed the BMFont bug, as part of v1 itself.** That strategy — a precise technical spec (exact file/line references, code snippets) handed to a sunlight-focused session, which implements + adds regression tests + ships a tagged release, followed by end-to-end verification and a `SCARAB_SUNLIGHT_VERSION` bump — worked cleanly and fast once already. Full-bundle protection (Lua source *and* textures/sounds/tilemaps) is v1's real target, not a reduced "logic-only" scope with assets left for later.

## Library choices, both verified directly (not assumed from memory)

### Zip writer: miniz

PhysFS (what every resource read goes through) can *read* archives but has no built-in zip-*writing* capability. **Decided: miniz**, giving one convenient `scarab`-driven encrypt+zip step end-to-end (rather than a separate manual zip step after encrypting to a mirrored directory).

License verified directly from the canonical `richgel999/miniz` GitHub repo's own `LICENSE` file: **MIT License**, copyright RAD Game Tools/Valve Software/Rich Geldreich. Compatible with Scarab's own zlib license — both permissive, neither copyleft — and this repo already depends on MIT-licensed code today (`nlohmann_json`), so this isn't even a novel situation. Follow-up when adopted: give it the same lightweight attribution treatment other bundled deps get (raylib/libtmx credited in root `README.md`; the OFL font's license file copied alongside it in `resources/fonts/`).

### Crypto library: libsodium, via `robinlinden/libsodium-cmake`

Verified directly against `doc.libsodium.org/installation`: builds both static and shared/dynamic by default, genuinely multi-platform (Linux, Windows via MSVC/MinGW, macOS Intel+ARM, plus more) — covers all 4 platforms Scarab's own CI matrix builds for.

**Real constraint**: libsodium's native build system is Autotools, not CMake — no official CMake support. This project avoids mixing Autotools (or any non-CMake build tool) in at all, CMake-only, no exceptions — this rules out an `ExternalProject_Add` that shells out to `./configure && make` under the hood, and rules out routing only Windows through `vcpkg` while Unix uses something else.

**Real precedent already in this codebase**: this repo already solves this exact class of problem for **Lua itself** — official Lua is Makefile-based, and `walterschell/Lua` (a community CMake-wrapper fork, source untouched) is the actual default dependency, pulled in via plain `FetchContent_Declare`. The alternative (`SCARAB_USE_OFFICIAL_LUA_FTP=ON`, `ExternalProject_Add` shelling out to `make`) is explicitly the lesser, opt-in-only, Windows-unsupported fallback (`option(SCARAB_USE_OFFICIAL_LUA_FTP ... OFF)`) — not the recommended path. Follow the same shape for libsodium.

**Candidate vetted: [`robinlinden/libsodium-cmake`](https://github.com/robinlinden/libsodium-cmake)** — a real, purpose-built CMake wrapper (vendors the actual unmodified official `jedisct1/libsodium` source as a git submodule, not a fork):
- License: ISC — permissive, same family as MIT/zlib, no compatibility concern
- Static/shared: respects the *standard* CMake `BUILD_SHARED_LIBS` variable directly — no custom toggle
- Usage: `FetchContent_Declare(Sodium GIT_REPOSITORY .../libsodium-cmake.git GIT_TAG <pinned-commit>)` + `FetchContent_MakeAvailable(Sodium)` + `target_link_libraries(... sodium)`
- Linux (x64+ARM64) + Windows (x86+x64) — CI-verified by the wrapper's own CI matrix, both static and shared builds tested on each
- macOS Intel — **verified live** on a real dev machine: cloned with `--recurse-submodules`, built both static (`libsodium.a`) and shared (`libsodium.dylib`), confirmed with a real functional test program (`sodium_init()` + `crypto_secretbox_easy`/`crypto_secretbox_open_easy` roundtrip) — not just "it compiles." libsodium `1.0.20`. Matches Scarab's own `macos-15-intel` CI runner exactly.
- macOS ARM64 (Apple Silicon) — **accepted as a known risk, not blocking.** No ARM Mac available to verify in isolation; real verification will happen naturally the first time libsodium is actually integrated and built through Scarab's own CI (which already includes `macos-latest`, confirmed Apple Silicon). If that run fails, it fails loudly in CI at integration time rather than being silently assumed.

## Checkpoints

### 0. Lock the libraries — ✅ done
- [x] Zip writer decided: miniz (license verified)
- [x] Crypto library decided: libsodium via `robinlinden/libsodium-cmake` (license + static/shared + Linux/Windows/macOS-Intel verified; macOS ARM64 accepted as a known risk)

### 1. `LuaCryptoApi` — ✅ done
- [x] New `src/lua/luacryptoapi.h`/`.cpp` — libsodium `crypto_secretbox_easy`/`crypto_secretbox_open_easy` (XSalsa20-Poly1305, authenticated), a random 24-byte nonce generated per call and prepended to the returned blob
- [x] `crypto_encrypt_data(data) -> encrypted` / `crypto_decrypt_data(encrypted) -> data` — no key argument on either; both use `SCARAB_CONTENT_KEY` internally only
- [x] Key baked in at compile time via a new `SCARAB_CONTENT_KEY` CMake cache string + `target_compile_definitions`, matching the existing `SCARAB_VERSION`/`SCARAB_SUNLIGHT_VERSION` pattern — never exposed to Lua as a readable value; empty by default, both primitives fail cleanly (`nil` + a clear stderr message) rather than falling back to a shared public placeholder key
- [x] `SCARAB_LIBSODIUM_SHARED` option added too (default OFF, static) — real bug found and fixed while wiring it: `raylib`'s own CMakeLists.txt caches `BUILD_SHARED_LIBS=ON` earlier in the same configure pass (to build itself shared), so a first version of this logic that only ever set `BUILD_SHARED_LIBS` when the option was ON silently inherited that cached `ON` even with the option OFF — confirmed live (`dyld: Library not loaded: @rpath/libsodium.dylib` despite the option defaulting off). Fixed by setting it explicitly both ways.
- [x] Verified for real, through the actual Lua binding (not just the isolated C test from checkpoint 0): a genuine `crypto_encrypt_data`/`crypto_decrypt_data` roundtrip, including a binary-safe embedded zero byte surviving intact (confirms `lua_tolstring`/`lua_pushlstring` are used correctly, not the null-terminated Lua C API forms), and a tamper test (flip one ciphertext byte) correctly triggering the AEAD authentication failure — `nil` returned, clean error logged, no crash
- [x] Registered in `LuaEngine::RegisterCalls` alongside `LuaJsonApi`/`LuaScriptingApi`/`LuaTimerApi` (no `ITileMap`/`SoundManager`/`SpritePool` dependency)
- [x] Tagged for the Lua API doc generator (`@luacategory{Crypto}`/`@luaname`/`@luagroup`/`@luadoc`/`@luaexample`/`@luaoutro`) — `scripts/_lua_api_shared.py`'s `SOURCE_TO_DOC` updated (`luacryptoapi.cpp` → `crypto.md`); full generator run confirmed clean
- [x] `CLAUDE.md`'s `src/lua/` architecture bullet updated to mention `LuaCryptoApi`
- [x] Regression-checked `hello-world`/`sprite` samples post-integration, both clean
- [x] Follow-up: new `samples/crypto` (encrypt → decrypt roundtrip + a deliberate tamper test showing the AEAD failure mode nil-returns rather than accepting silently-wrong data) — verified live against both a default (empty-key) build and a real-key build, `samples/README.md`'s category index updated

### 2. Packaging tool — ✅ done
- [x] A Lua script, run via `scarab` itself, encrypting + zipping (miniz) in one step: new `tools/pack.lua` + `tools/pack-config.example.json` + `tools/README.md`
- [x] New `src/lua/luapackapi.h`/`.cpp` (`Scarab::Engine::Lua::LuaPackApi`) — `pack_list_directory`/`pack_read_file`/`pack_write_file`/`pack_create_archive`/`pack_add_entry`/`pack_close_archive`, miniz-backed (`mz_zip_writer_*`, `#include "miniz.h"` — the umbrella header, not `miniz_zip.h` directly, which alone is missing `time_t`/`MZ_DEFAULT_COMPRESSION`); archive handles follow the same "0 is never a valid handle" convention as `sprite_acquire`, tracked via `static std::map<int, mz_zip_archive*>`
- [x] Deliberately the one `src/lua/` file whose primitives read/write real native OS paths, bypassing `SunLight::FileSystem` entirely — documented explicitly in both the class-level doc comment and `CLAUDE.md`, since packaging runs against a loose, not-yet-mounted source tree before any mounting concept applies
- [x] `miniz` added to the root `CMakeLists.txt` via `FetchContent` (pinned `3.1.2`), forced static, same save/restore `BUILD_SHARED_LIBS` pattern already used for libsodium (guards against the same raylib-leaks-`BUILD_SHARED_LIBS=ON` bug from checkpoint 1)
- [x] Registered in `LuaEngine::RegisterCalls` alongside `LuaCryptoApi`/`LuaJsonApi`/`LuaScriptingApi`/`LuaTimerApi`
- [x] Tagged for the Lua API doc generator (`@luacategory{Pack}`/`@luaname`/`@luagroup`/`@luaheading`/`@luadoc`/`@luaoutro`) — `scripts/_lua_api_shared.py`'s `SOURCE_TO_DOC` updated (`luapackapi.cpp` → `pack.md`); full generator run confirmed clean, `pack.md` renders correctly
- [x] `CLAUDE.md`'s `src/lua/` architecture bullet updated to mention `LuaPackApi`
- [x] Verified for real, end-to-end, with the actual `tools/pack.lua` script (not just an ad-hoc scratchpad stand-in) invoked the correct, documented way (`cd` to this repo's own root, `./build/scarab tools/pack.lua`, with a gitignored `pack-config.json` at the repo root whose `source_dir`/`output` fields are absolute native paths) — produced a genuine, standard `.zip` (`unzip -l` confirms real structure/filenames), each entry's content encrypted (byte counts grow by the 24-byte nonce + 16-byte MAC overhead: 36→76, 25→65, 17→57), for a 3-file test tree including one file with an embedded null byte
- [x] First documented USAGE (run from the *game's own* directory, passing an absolute path to `tools/pack.lua` elsewhere on disk) was wrong and confirmed to fail live with the same mount-restriction error already known from checkpoint 0/1 work (`Lua error: cannot open ...`) — `pack.lua`'s own entry-script path and `pack-config.json`'s location both still route through the mount-based filesystem even though the primitives they call don't; corrected to the same "run from this repo's own root" convention every `samples/` entry already uses, re-verified working after the fix
- [x] Follow-up: `scarab --pack <config.json>` CLI flag (`main.cpp`) — the actual, user-facing CLI form (vs. running `tools/pack.lua` directly), works from **any** directory, no `cd`/hand-authored repo-root `pack-config.json` needed. `main.cpp` reads `<config.json>` itself via a plain native file read (mirroring `ResolveEntryScript`'s own loose-`project.json` handling), resolves relative `source_dir`/`output` against the config file's own directory, and hands them to `tools/pack.lua` as the `SCARAB_PACK_SOURCE_DIR`/`SCARAB_PACK_OUTPUT` environment variables (read via Lua's own `os.getenv` — no new engine↔Lua plumbing needed); the entry script itself resolves via the same `APP_DIR` mount every archived game's own entry script already uses, since `tools/pack.lua` is now copied next to the built executable by `scarab_copy_binaries` (root `CMakeLists.txt`, unconditional on every platform — also means any downstream project consuming Scarab via `FetchContent` gets the packer for free). `path`/`--entry`/`--pack` are all mutually exclusive (CLI11 `excludes()`); exactly one of `path` or `--pack` is required, checked explicitly since CLI11 can't express "exactly one of these two, unconditionally" on its own. `tools/pack.lua` prefers the env vars when set, falling back to `load_json("pack-config.json")` for the original direct-invocation form — both forms verified live end-to-end (identical archive contents both ways), including from a directory with no relation to this repo at all, and every usage-error combination (`scarab` with no args, `path` + `--pack` together, `--entry` + `--pack` together) confirmed to fail cleanly before any window opens

### 3. Runtime decrypt hook — Scarab's own read paths — not started
- [ ] `LuaEngine::RunFile`, `LuaJsonApi::LoadJson`, `LuaFileSystemApi::DoFile`

### 4. Runtime decrypt hook — `sunlight`'s own read paths (cross-repo) — not started
- [ ] Precise spec covering `RaylibEngine`'s `SetLoadFileDataCallback`, `RayLibSound::Load`, `TileMapRenderer::LoadMap`'s own `ReadFile` calls
- [ ] Open design question to resolve with whoever implements the sunlight side (propose, don't assume): a generic runtime key/callback setter on `sunlight`'s side (e.g. `IFileSystem::SetDecryptionKey(...)`), with Scarab as the first caller populating it from its own compile-time secret — keeps "bake a secret in at compile time" a Scarab-only concern, since sunlight is a reusable library, not per-game-compiled the way Scarab is

### 5. Backward compatibility check — not started
- [ ] A plain, unencrypted `.zip` must keep working exactly as it does today — this whole feature stays opt-in, never a breaking change

### 6. Docs — not started
- [ ] How a consuming game generates/stores its own key safely, explicit "never commit a real key to version control" warning, demoed with a throwaway/example key rather than a real one anywhere in this repo's own tracked files

## Explicitly out of scope for now

- A minimal drag-and-drop GUI / LÖVE-style launcher window for `scarab` (drop a `.zip`/source folder to run it) + a menu-driven encrypt/decrypt browsing UI on top of that
- Multiple `.zip` archives per game
- Selective per-file-type encryption policy — only worth revisiting if the uniform-encryption default turns out to have a real, measured performance cost somewhere
