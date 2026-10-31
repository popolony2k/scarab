# Scarab master plan

This is the working plan for integrating sunlight's `feat/master_plan`. Each item has a
status, a scope, and the test items that must pass before it is closed. Items are closed
one at a time, in phase order, unless the owner decides otherwise.

Like [`plans/CARAVELLIUS_BREAKAGES.md`](CARAVELLIUS_BREAKAGES.md), this file is integration
material only: it lives on `master_plan` and is removed before `master_plan` goes to
`main`. `master_plan` merges to `main` only on the owner's own explicit command, after
Scarab's own work here is integrated with Caravellius - not automatically once this plan's
own items are closed, and not before Caravellius has been told what changed.

Every change follows this project's own rules in [`CLAUDE.md`](../CLAUDE.md): tests first
(a regression test must fail on the old code, where applicable), the full existing check
suite passing (samples, the renderer/view API smoke test, the tileset archive smoke test,
the headless smoke test), an AddressSanitizer run for anything touching lifetimes once
Phase R lands, and a PR per item (or per consolidated set of mechanical items).

This plan follows sunlight's own [`plans/MASTER_PLAN.md`](../../sunlight/plans/MASTER_PLAN.md)
in shape and in working rules - see sunlight's handoff, read from
[`doc/SCARAB_HANDOFF.md`](../../sunlight/doc/SCARAB_HANDOFF.md) in a local checkout of
sunlight (`../sunlight` from this repo's root), and its peer-impact list,
[`doc/BREAKAGES.md`](../../sunlight/doc/BREAKAGES.md) (49 entries). sunlight's own commit
pinned for this work: `772851ce7120c0d4f25dbf72c8d15f7e54bcd1d5` (the head of
`feat/master_plan` when the handoff was sent), re-pinned in P4 to
`a63fec6811ba28fb46a94015d05b852393016692` (sunlight's fix for semi-transparent drawing), in
P5 to `bfce5983370fa0661c18701164a10f7e18f1e7a1` (sunlight's blend modes), in C6 to
`9c8c6bf421a1147d1e8c099e698dfbe7c1885af2` (fonts by handle), in V6 to
`f651accc2291a2ea9020dad3bfc2f1245e76858f` (I4, `..` in an embedded tileset image path), in
P6 to `fa953f38cb22508247b528c8234ad2bb0f4293e8` (I5, CollisionManager layer-id checks), and in
V9 to `a6ac4f6bea9973795c1668dd8eabfc67defc03c1` (I6, UnloadMap takes sprites' colliders out), and in
V11 to `4189199f7b3af75e590792f85d51051916f4a206` (I7, tile collisions and tile picking in map space), and in
V12 to `4274ecbf6cdd71c3625254f68ae14e0ed6631acb` (I8, layer tint at runtime, group tint and opacity), and
in P7 to `8842ccc831a0dc7d10f85a93fc9f978f63015b09` (I9, layer opacity as a 0..1 float, exact round trips),
and in P8 to `ef1499d811e87c520a3669c9754cf4e6893ff4a8` (I10, sprite opacity and tint), and in V13 to
`56a73cd4de2868fe30e124e79340ea19d71feae7` (I11, tile animation by time and the frame query, with the 0 ms fix).

## Phase P: pin & plumbing

| # | Item | Status |
|---|------|--------|
| P0 | Write and get this document approved | DONE |
| P1 | Split `SCARAB_SUNLIGHT_VERSION`'s single hardcoded-`v`-prefix mechanism into a fetch-ref variable and a display-version variable; decide `--version`'s format for a commit-pinned build | DONE |
| P2 | Pin `GIT_TAG` to `772851ce7120c0d4f25dbf72c8d15f7e54bcd1d5`; make the code compile against it (the `SunLight::Base` namespace move, BREAKAGES entry 14; `BaseCommand`'s new header, entry 5) | DONE |
| P3 | Every existing Scarab check green against the pin, before any further BREAKAGES item is touched | DONE |
| P4 | Re-pin to `a63fec6811ba28fb46a94015d05b852393016692` (sunlight's I1 fix: semi-transparent drawing no longer darker; sunlight BREAKAGES entry 50) | DONE |
| P5 | Re-pin to `bfce5983370fa0661c18701164a10f7e18f1e7a1` (sunlight's I2: blend modes; sunlight BREAKAGES entry 51) | DONE |
| P6 | Re-pin to `fa953f38cb22508247b528c8234ad2bb0f4293e8` (sunlight's I5: CollisionManager checks every layer id, `Clear()`/`RemoveAll()` work; sunlight BREAKAGES entry 54) | DONE |
| P7 | Re-pin to `8842ccc831a0dc7d10f85a93fc9f978f63015b09` (sunlight's I9: layer opacity is a 0..1 float with exact round trips, `SetScreenFade` clamped; sunlight BREAKAGES entry 58) and move Lua's layer opacity to 0..1 | DONE |
| P8 | Re-pin to `ef1499d811e87c520a3669c9754cf4e6893ff4a8` (sunlight's I10: sprite opacity and tint; sunlight BREAKAGES entry 59) and expose them to Lua (`sprite_set/get_opacity`, `sprite_set/get_tint`) | DONE |

**P0 (DONE).** This document and [`plans/CARAVELLIUS_BREAKAGES.md`](CARAVELLIUS_BREAKAGES.md)
(this plan's own peer-impact log, mirroring sunlight's `BREAKAGES.md` one level down the
chain), reviewed and approved by the owner before the `master_plan` branch held anything
else.

**P1 (DONE).** `GIT_TAG v${SCARAB_SUNLIGHT_VERSION}` (`CMakeLists.txt`) and
`"sunlight v" SCARAB_SUNLIGHT_VERSION` (`main.cpp`'s `--version` string) both hardcoded a
literal `v` in front of the variable, so it couldn't hold a commit hash as-is - confirmed by
reading both call sites, not assumed. `SCARAB_SUNLIGHT_VERSION` is now two variables,
`SCARAB_SUNLIGHT_REF` (what `GIT_TAG` fetches) and `SCARAB_SUNLIGHT_DISPLAY_VERSION` (what
`--version` prints after `"sunlight "`), each used **verbatim** at its one call site - no
`v` hardcoded anywhere else, so either variable can hold any ref shape (a tag, a bare commit
hash, a `branch@shorthash`) without the other needing to change. **Decided while building
it**, not exactly as first scoped: rather than conflating the mechanism change with the
actual hash pin (P2), P1 keeps both variables pointing at today's real release
(`SCARAB_SUNLIGHT_REF "v0.38.0"`, `SCARAB_SUNLIGHT_DISPLAY_VERSION "v0.38.0"`) and proves the
refactor is behavior-neutral; P2 is where they actually flip to the commit hash, together
with the compile fixes that requires. `CLAUDE.md`'s own Build-section description of this
mechanism is updated in the same change (agreed with the owner - the doc describes the exact
mechanism being changed, not a general sweep for Phase T).
- Test items: `scarab --version` is byte-for-byte identical to before
  (`scarab v0.1.25 (sunlight v0.38.0, Lua 5.4.7)`), confirmed by a direct string comparison,
  not eyeballed; `cmake -B build -S .` configures clean against `v0.38.0` through the new
  `GIT_TAG ${SCARAB_SUNLIGHT_REF}`; a headless sample (`samples/hello-world`) still runs.
- `--version`'s format for the commit-pinned build (P2's own concern, decided here since the
  mechanism now exists to support it): `scarab v<version> (sunlight <branch>@<short hash>,
  <Lua release>)`, eg. `scarab v0.1.25 (sunlight feat/master_plan@772851c, Lua 5.4.7)` -
  matches the handoff brief's own suggested shape.

**P2 (DONE).** `stDimension2D`/`stSize2D`/`stCoordinate2D` move from `SunLight::TileMap` to
`SunLight::Base`, no aliases kept (BREAKAGES entry 14) - **12 sites, not the 11 a first grep
found**: `main.cpp`, `src/lua/luaspriteapi.cpp`, `src/lua/luarendererapi.cpp`,
`src/lua/luaviewapi.cpp`, `src/lua/luacameraapi.cpp` each had the type fully qualified
(`SunLight :: TileMap :: stDimension2D` etc., 11 sites, renamed by a scripted substitution);
`src/lua/luatilemapapi.cpp` had a 12th, unqualified `stCoordinate2D` relying on its own
`using namespace SunLight :: TileMap;` - **found only by actually grepping that one file
again after the others compiled clean**, not assumed from BREAKAGES' own count, and fixed by
qualifying that one site explicitly (`SunLight :: Base :: stCoordinate2D`) rather than adding
a second `using namespace` line. `BaseCommand`'s new header (entry 5): no direct include of
the old command-types path anywhere in Scarab, confirmed by grep - nothing to do.
- **Found while building it, not in P2's original scope**: `src/lua/luarendererapi.cpp`
  reads and writes `config.bUseDefaultKeyHandler` directly, a flat compile error of its own
  once `RendererConfig` loses that field (BREAKAGES entry 26) - P2 could not compile without
  resolving this too. **Owner's decision (2026-10-21): implement D1 in full here**, rather
  than a minimal no-op stash, since D1's design was already settled and small. See D1 below
  for what was built; it shipped in this same PR.
- Test items: `cmake -B build -S .` configures and builds clean against the real, network-
  fetched commit (not just the local `../sunlight` checkout used for fast iteration) -
  confirmed separately, `git -C build/_deps/sunlight-src rev-parse HEAD` reads exactly
  `772851ce7120c0d4f25dbf72c8d15f7e54bcd1d5`; `scarab --version` prints
  `scarab v0.1.25 (sunlight feat/master_plan@772851c, Lua 5.4.7)`, matching P1's own decided
  format exactly.

**P3 (DONE, folded into the same PR as P2 - its own scope was already satisfied by P2's
verification).** `scripts/renderer_api_smoke.py` (all 9 scenarios, including `create`'s own
`use_default_key_handler` option round-trip), `scripts/tileset_archive_smoke.py`, the
headless smoke project, and all 15 samples headless - all green against the real pin.
Additionally (not in P3's original scope, but the natural check for what P2 found): a
scratch headless script confirmed `renderer_create{ use_default_key_handler = true }`
creates the renderer without crashing *before* any map is loaded and that
`renderer_get_config`'s readback matches, then `tilemap_load_map` still succeeds afterward -
closing D1's own open question (`Install` is safe to call before a map is loaded). Not
checked: the bindings' actual functional effect (do the arrow keys really pan the camera) -
nothing exercises real input headless, and this was never checked in a real window even
before this change (`use_default_key_handler=true` was never used by any Scarab sample,
confirmed earlier). Available as a quick real-window check if wanted; not blocking.

**P4 (DONE, 2026-10-22, added during V1).** Found by the owner in V1's real-window
check of `samples/camera`: `test.tmx`'s semi-transparent layers (`background_clouds` 0.69,
`background` 0.65) drew darker than in Tiled. A sunlight bug, present in v0.38.0 too. sunlight
draws the frame into a render target and then blends it over a window cleared to black, and
raylib's `BLEND_ALPHA` blended the target's own alpha channel as well (one 0.69 layer over the
opaque sky left it at 0.79; both layers at about 0.70). Reported to the sunlight session on the
owner's request. Fixed there on `feat/master_plan` only (sunlight #214, "I1"): in
`RaylibEngine::BeginRenderTarget`, the alpha channel now blends with (ONE,
ONE_MINUS_SRC_ALPHA), and colour blending is unchanged. sunlight measured a pixel on the GPU at
167 173 175 before and 242 250 253 after, which matches Tiled. The hash is recorded in sunlight's
`doc/SCARAB_HANDOFF.md`, section 9 "sunlight commits to pin". sunlight BREAKAGES entry 50:
opaque drawing is unchanged, and everything semi-transparent now comes out lighter, as
authored.
- Pin: `SCARAB_SUNLIGHT_REF` `a63fec6811ba28fb46a94015d05b852393016692`,
  `SCARAB_SUNLIGHT_DISPLAY_VERSION` `feat/master_plan@a63fec6`. Verified in the local sunlight
  checkout: the commit is on `origin/feat/master_plan` and descends directly from `772851c`
  (2 commits, one `src` file changed). No Scarab code change was needed.
- Verified with a real fetch (not `FETCHCONTENT_SOURCE_DIR_SUNLIGHT`): the fetched checkout is
  exactly `a63fec6`, and `scarab --version` prints
  `scarab v0.1.25 (sunlight feat/master_plan@a63fec6, Lua 5.4.7)`. The camera boundary,
  renderer API, tileset archive (plain + encrypted) and headless smoke tests (incl. the
  frame-budget exit 3) pass, the Lua API docs generator passes, and all 15 samples run headless.
- `plans/CARAVELLIUS_BREAKAGES.md` entry 2.
- The real check is by eye only, since nothing headless can see colour. The owner ran
  `samples/camera` in a real window (screenshot, 2026-10-22 02:50): `background_clouds` and
  `background` now show light blue and white like Tiled, not grey.

**P5 (DONE, 2026-10-22).** sunlight added blend modes on `feat/master_plan` (sunlight #216,
"I2", on top of I1), at the owner's request, and sent the new pin. The owner asked for Scarab's
Lua side as a new item, B1 (below). This item is only the re-pin. In sunlight:
- Four modes, `SunLight::Base::BlendMode`: `BLEND_MODE_NORMAL` (the default), `_ADD`,
  `_MULTIPLY` and `_SUBTRACT`.
- A scoped mode for a game's own drawing, `IDrawSurface::BeginBlendMode`/`EndBlendMode`.
  The calls do not nest.
- A per-sprite mode, `Sprite::SetBlendMode`/`ClearBlendMode`/`HasBlendMode`/`GetBlendMode`.
- Every frame starts in normal and the screen fade always runs in normal. A frame that uses no
  mode makes no blend call at all.
- sunlight BREAKAGES entry 51: only classes that implement `IEngine`/`IDrawSurface` must
  change.
- Pin: `SCARAB_SUNLIGHT_REF` `bfce5983370fa0661c18701164a10f7e18f1e7a1`,
  `SCARAB_SUNLIGHT_DISPLAY_VERSION` `feat/master_plan@bfce598`. Verified in the local sunlight
  checkout: it is on `origin/feat/master_plan` and descends directly from `a63fec6` (2 commits,
  11 `src` files changed).
- No Scarab code change. Scarab implements neither interface, and no file in `src/`
  includes `raylib.h` or names `BlendMode`, so raylib's own global `BlendMode` (the same trap as
  `KeyboardKey`) cannot bite yet. B1 must write `SunLight :: Base :: BlendMode` in full.
- Verified with a real fetch: the fetched checkout is exactly `bfce598`, the build has zero
  warnings, and `--version` prints
  `scarab v0.1.25 (sunlight feat/master_plan@bfce598, Lua 5.4.7)`. The camera boundary,
  renderer API, tileset archive (plain + encrypted) and headless smoke tests (incl. exit 3 on
  the frame budget) pass, the Lua API docs generator passes, and all 15 samples run headless.
- No CARAVELLIUS_BREAKAGES entry: nothing changes for a game that uses no blend mode, and
  Caravellius cannot use one before B1 exists. No real-window check either, for the same
  reason.

**P6 (DONE, 2026-10-24).** Re-pin to sunlight's I5, the fix for the CollisionManager bug
found in D3 and reported on the owner's request (sunlight PR #223, BREAKAGES entry 54). Checking
the report, sunlight found two more of the same class and fixed all four:
`AddColliderToColliderRule`'s duplicate search indexed the layer array with both ids before any
check; its bounds check tested the first id twice (`(1, 300)` and `(1, -1)` were accepted, and
the next `Update()` crashed); `Clear()` destroyed every layer's list, so the next add/remove/update
dereferenced null; `RemoveAll()`'s default `-1` ("every layer") cleared nothing. Every method
taking a layer id now checks `0 <= id < MAX_COLLIDER_LAYERS` first. Nothing to do for Scarab or
Caravellius: valid ids behave as before, D3 already refuses ids outside `0..254`, and neither
calls `Clear()` or `RemoveAll()`. Scarab keeps its own range checks (the owner's question,
answered 2026-10-24): they give the precise `"argument 'x' must be between 0 and 254"` instead of
a plain refusal that would read like "already declared", and use sunlight's public
`MAX_COLLIDER_LAYERS`; only their comments changed.
- `SCARAB_SUNLIGHT_REF` `fa953f38cb22508247b528c8234ad2bb0f4293e8`,
  `SCARAB_SUNLIGHT_DISPLAY_VERSION` `feat/master_plan@fa953f3`. Verified: on
  `origin/feat/master_plan`, two commits after `f651acc` (the I4 pin record, docs only, and the
  fix; 2 files in `src/collision/`, no API change).
- Checks: real fetch exact (`build/_deps/sunlight-src` at `fa953f3`), clean build, `--version`
  shows `feat/master_plan@fa953f3`; every smoke test (error reporting, renderer API, camera
  boundary, blend API, tilemap layer, input constants, font API, headless incl. the budget run,
  tileset archive plain and encrypted), the docs generator and all 18 samples pass; Caravellius'
  acceptance PASS and its reference state fingerprint (`hash=33897632`).
- Local note: the local build's content key was lost when `build/` was deleted for the clean
  fetch; the owner chose to reuse Caravellius' local key for Scarab's local build (CI uses its own
  secret, unaffected).

**P7 (DONE, 2026-10-30).** sunlight I9 (PR #231, BREAKAGES 58, BREAKING), found by sunlight
after V12 and sent on its owner's go-ahead: `stLayer::nOpacity` (an int 0..255) is replaced by
`fOpacity` (a float 0..1, as Tiled, libtmx and raylib store it). With the int, an unchanged
`GetLayer` -> `SetLayer` moved an authored 0.5 to 128/255 = 0.502, and Scarab's `tilemap_set_layer`
does that round trip on every call, even one that only changes visibility. `GetLayer` now reads the
authored value exactly; `SetLayer` clamps to 0..1 and writes the opacity and parallax only when they
changed. `SetScreenFade` clamps to 0..1 (Scarab's `screen_fade` already refuses anything else, D3).
- **Decision (the owner): option (a)** - Lua's layer opacity moves to a number 0..1 too, in
  `tilemap_get_layer`/`tilemap_set_layer` (and `_by_name`), rather than keeping 0..255 with a
  conversion that would bring the 0.502 drift back. Strict 0..1 on write (outside it, or not a
  number: `nil, "message"`, the layer unchanged), like `screen_fade` and `sound_set_volume`. It
  reverses the unit V3 chose; V3's exactness is kept, now without rounding.
- **Reads the shortest decimal:** sunlight's float holds an authored 0.69 as 0.6899999761...; Scarab
  pushes the shortest decimal that converts back to the same float, so a script reads `0.69` as Tiled
  shows it and writing it back gives the identical float (mutation-checked: pushing the raw float makes
  the test read 0.68999999761581).
- **Re-pin:** `SCARAB_SUNLIGHT_REF` `8842ccc831a0dc7d10f85a93fc9f978f63015b09`,
  `SCARAB_SUNLIGHT_DISPLAY_VERSION` `feat/master_plan@8842ccc`; verified on `origin/feat/master_plan`,
  two commits after `4274ecb` (the I8 pin record and the fix); reconfigured in place.
- **Tests:** `scripts/tilemap_layer_smoke` (V3's) rewritten for 0..1: the authored 0.69/0.65/1 read
  exactly, three write-backs in a row change nothing, 0/0.25/0.3/0.5/0.65/0.69/1 round-trip exactly by
  id and by name (0.5 is the value the int moved), 1.5/-0.1/255/"0.5" refused with the layer unchanged,
  the other fields round-trip with it. `error_reporting_smoke`: an opacity of 255 refused.
- **Samples:** `samples/tilemap`'s Up/Down now step the clouds layer by 0.125 (exact in binary) between
  0 and 1, and the HUD shows "of 1" (its HUD moved to Caravellius 8x8 on three short lines, at the
  owner's request: the built-in font was hard to read); `samples/shapes`' `G` steps the group through
  1 / 0.25 / 0.6. The owner's window check (2026-10-30): readable HUD, the clouds layer at exactly 0.25.
- **Docs:** `tilemap_get_layer` / `tilemap_set_layer` (`@luadoc`, example now `0.5`).
- **Caravellius:** its `core/presentation.lua` reads a layer's opacity and passes it straight back to
  hide the layer, so it keeps working (acceptance and fingerprint unchanged). Breakage entry 17, and
  entry 4 (V3's 0..255) now points at it.

**P8 (DONE, 2026-10-30).** sunlight I10 (PR #233, BREAKAGES 59, nothing breaks), sent by sunlight on
its owner's go-ahead: `Sprite::SetOpacity`/`GetOpacity` (0..1, clamped, NaN ignored - the unit of a
layer's `fOpacity`) and `SetTint`/`GetTint` (opaque white = none; RGB multiplies into the frame, alpha
kept but unused). `Draw()` applies both to the frame it draws, for that draw only, so a canvas shared
by two sprites is safe; `Unload()` keeps them; a sprite does not take its layer's opacity or tint.
`Shape::MultiplyTint` moved to `SunLight::Base::MultiplyTint` (Scarab never used it). sunlight's code
style now forward-declares only to break an include cycle; Scarab's `src/` has no forward declaration,
so nothing to change.
- **Lua (covered fully, the owner's rule):** `sprite_set_opacity(h, 0..1)` / `sprite_get_opacity(h)`
  (strict on write - outside 0..1, NaN or not a number: `nil, "message"`, unchanged; read back as the
  shortest decimal, like P7's layer opacity) and `sprite_set_tint(h, r, g, b[, a])` /
  `sprite_get_tint(h) -> r, g, b, a` (0..255, `a` optional = 255, the shape of
  `tilemap_set_layer_tint`). `ShortestDecimal` moved from `luatilemapapi.cpp` to `LuaEngineUtil` so
  both opacities share it.
- **Recycled slots:** `SpritePool::Release` resets opacity to 1 and tint to white (`__SPRITE_OPAQUE`,
  `__SPRITE_NO_TINT`), like the world-space flag and blend mode, since sunlight keeps them across
  `Unload()`.
- **Re-pin:** `SCARAB_SUNLIGHT_REF` `ef1499d811e87c520a3669c9754cf4e6893ff4a8`,
  `SCARAB_SUNLIGHT_DISPLAY_VERSION` `feat/master_plan@ef1499d`; verified in `../sunlight`, two commits
  after `8842ccc` (the I9 pin record and I10); reconfigured in place.
- **Tests:** new `scripts/sprite_tint_smoke` (ci.yml, all 4 platforms): a fresh sprite's defaults,
  0/0.1/0.25/0.3/0.5/0.69/0.75/1 round-trip exactly, tint with and without alpha, write-backs, every
  rejection (1.5, 255, -0.1, NaN, "0.5", missing; channels 256/-1/3.5/missing, alpha 300/"x") leaving
  the values as they were, bad/string/released handles, a recycled slot starting opaque and untinted,
  no renderer created by any of it, two sprites on the same texture keeping their own values,
  reconfiguring the texture and switching the sequence keeping them, and 5 frames of drawing.
  `error_reporting_smoke` gained one check per kind of failure. Proven: fails on the published v0.1.25
  (`sprite_get_opacity` is nil); mutation-checked twice (without the `Release` reset the recycled sprite
  kept 0.5 and 1,2,3,4; pushing the raw float read `0.30000001192093`).
- **Sample:** `samples/sprite` - `O` cycles Sunny's opacity 1 / 0.5 / 0.25 / 0, `T` its tint none / red
  hit flash / blue team; a second Sunny on the same texture stays untouched; HUD on Caravellius 8x8
  plates (the owner: the built-in font was unreadable). The owner's window check (2026-10-30): faded
  at 0.25 with the grass showing through, red and blue-team tints, the twin unchanged.
- **Docs:** new "Opacity and tint" group in the sprite reference.
- **Caravellius:** calls none of them; acceptance PASS, fingerprint `hash=33897632`. Breakage entry 18
  (informational).

## Phase D: design items

| # | Item | Status |
|---|------|--------|
| D1 | `use_default_key_handler` (BREAKAGES entry 26) - keep the Lua shape, back it with `DefaultControllerBindings::Install` | DONE |
| D2 | `renderer_create{ preferred_zoom }` - a new, optional option for the zoom `zoom_reset` returns to (Scarab only, no sunlight change) | DONE |
| D3 | Consistent error reporting: every Lua function that can fail returns `nil, "message"` instead of silently ignoring bad input | DONE |

**D1 (DONE, 2026-10-21 - implemented inside P2, not as its own separate PR, since P2 could
not compile without it; see P2's own write-up above).** `RendererConfig::bUseDefaultKeyHandler`
and the classic 5-argument `TileMapRenderer` constructor are both removed in sunlight. Option
B, as agreed: `renderer_create`'s/`renderer_get_config`'s `use_default_key_handler` keeps its
exact name and meaning. Implementation:
- `Scarab::Engine::IRendererProvider::CreateRenderer` gains a second parameter,
  `bool bUseDefaultKeyHandler` - a plain bool, not a new method naming
  `DefaultControllerBindings` anywhere in this interface, so `IRendererProvider` stays exactly
  as Lua-agnostic *and* sunlight-input-agnostic as its own doc comment already claimed (it
  names no sunlight input type, only one bool). A new `GetUseDefaultKeyHandler()` getter
  mirrors `GetEffectiveConfig()`'s own "read back what creation was actually given" shape, for
  `renderer_get_config` to use.
- `Scarab::Host::RendererProvider` (the one, sole implementer) stores what it was given in a
  new `m_bUseDefaultKeyHandler` member, and calls
  `SunLight :: Input :: DefaultControllerBindings :: Install( *m_pRenderer, m_pRenderer -> GetControllerManager() )`
  right after `Start()` inside `Create()`, exactly as planned - confirmed only one implementer
  of `IRendererProvider` and one caller of `CreateRenderer` exist, so no other code needed
  updating for the interface change.
- `src/lua/luarendererapi.cpp`: `ParseOptions` gains a matching output parameter (no longer
  writes into `RendererConfig` at all, since the field is gone there); `RendererCreate`
  threads the parsed bool through to `CreateRenderer`; `RendererGetConfig` reads
  `pProvider -> GetUseDefaultKeyHandler()` instead of a `RendererConfig` field.
- `main.cpp`'s `MakeDefaultRendererConfig` no longer sets a now-nonexistent field (the
  default renderer's own call site, `RendererProvider::GetOrCreateDefault`, never passes
  `bUseDefaultKeyHandler=true` to `Create()`, so this is still a no-op behaviourally, exactly
  as before).
- The open question from this item's own design ("confirm `Install` is safe to call before
  any map is loaded") is answered: yes, confirmed with a scratch headless script - see P3's
  own write-up.

**D2 (DONE, 2026-10-22, reviewed and approved by the owner - added 2026-10-22 during V1, done right after V1 by the owner's decision, out of
phase order).** Found by the owner in V1's real-window check of `samples/camera`: Home reset
the zoom to `1.0`, not to the `3.8125` the sample starts at. Not a bug. `zoom_reset` returns
to the view's *preferred* zoom, which is a separate setting from the current zoom (documented
on `view_set_zoom`/`view_zoom_reset`), and renderer creation sets only the current zoom, so the
preferred one stays at sunlight's own default, `1.0`. `samples/camera` itself was fixed inside
V1 with `view_set_preferred_zoom(0, viewport_get_zoom_factor())` (option (c), the owner's
choice). Option (a), making the creation zoom the preferred zoom too, was rejected: it would
silently break that documented contract. D2 makes the preferred zoom something a game can set
at creation instead:
- Lua: `renderer_create{ preferred_zoom = <factor> }`, strict like every other option,
  validated exactly like `zoom` (`LuaRendererApi::ValidateZoomFactor`). Left out, the
  behavior is today's (preferred zoom `1.0`), so no Caravellius breakage.
  `renderer_get_config` returns `preferred_zoom` live (`nPreferredZoomPos`).
- C++: sunlight's `RendererConfig` has no preferred-zoom field, so the value is applied right
  after creation with sunlight's existing `Viewport::SetPreferredZoom`, threaded through
  `IRendererProvider::CreateRenderer`/`RendererProvider::Create` the same way D1 threads
  `use_default_key_handler` - grouped with it into one small Scarab-side struct rather than a
  further parameter. A real `RendererConfig::nPreferredZoomPos` in sunlight is possible later
  (the owner's call); the Lua shape would not change.
- Tests: `scripts/renderer_api_smoke.py` - `defaults` (`preferred_zoom == 1.0`), `create` (a
  non-default value read back, and `zoom_reset()` lands on it), `errors` (a bad factor, a
  wrong type). Docs: the `renderer_create` options table and `renderer_get_config`'s doc.

**Built (2026-10-22):**
- `Scarab::Engine::stRendererCreateExtras` (`src/engine/irendererprovider.h`) holds
  `bUseDefaultKeyHandler` (D1) and `std::optional<unsigned> nPreferredZoomPos`.
  `IRendererProvider::CreateRenderer` and `RendererProvider::Create` take it in place of D1's
  bool. `Create` applies the preferred zoom right after `Start()` with
  `GetViewport().SetPreferredZoom`. The lazy default renderer passes no extras, so it keeps
  sunlight's `1.0`.
- `renderer_create{ preferred_zoom }` is parsed like `zoom` (`ValidateZoomFactor`, errors
  prefixed `option 'preferred_zoom': `). `renderer_get_config` returns `preferred_zoom` live
  from `nPreferredZoomPos`. `FactorOfPosition` moved from `luaviewapi.cpp` to
  `LuaRendererApi`, so both files share it.
- Found while building it: sunlight keeps the upper zoom limit as an exclusive bound internally
  (`SetMaxZoom` stores `max + 1`), so `SetPreferredZoom( ZOOM_POS_MAX )` is accepted. The new
  `preferredzoom` scenario pins it with `ZOOM_FACTOR_MAX`.
- Tests (`scripts/renderer_api_smoke.py`): `defaults` (`preferred_zoom == 1.0`), `create`
  (`2.5`, distinct from `zoom = 2.0625`), `errors` (a string, `3.8` with the nearest valid
  factor named, `20` outside the scale), `lazy` (the implicit default renderer keeps `1.0`), and a
  new `preferredzoom` scenario (`zoom_reset` lands on `preferred_zoom`, `ZOOM_FACTOR_MAX` is
  accepted, and a later `view_set_preferred_zoom` shows up live in `renderer_get_config` and is
  followed by `zoom_reset`). Mutation check: with the `SetPreferredZoom` call removed, `create`
  and `preferredzoom` fail with the exact reasons, and they pass again once it is restored.
- All other checks pass: camera boundary, tileset archive (plain + encrypted), headless smoke,
  the Lua API docs generator (the renderer page shows the new row, section and example), and
  all 15 samples headless.
- No CARAVELLIUS_BREAKAGES entry: the option is opt-in, and leaving it out keeps today's
  behavior exactly. No real-window check is needed: nothing is drawn differently, and the
  headless scenarios cover all of it.

**D3 (DONE, added 2026-10-23 at the owner's request during B1; pulled forward by the owner
on 2026-10-24, right after V6, then back to the natural order with V7).**
Consistent error reporting. Only the newer `renderer_*`/`view_*` functions (and B1's) report a
failure as `nil, "message"`. The other 116 registered Lua functions across 14 `src/lua/*api.cpp`
files report nothing. For example, 13 `sprite_*` functions silently do nothing with a bad
handle, and some getters return a "safe" value instead (`sprite_get_world_space` returns
`false`). Scope:
- Audit each of those functions and classify what bad input means for it: an invalid sprite
  handle, an unknown sound/layer/timer id, a wrong argument type, or nothing that can fail.
- Every function that can fail returns `nil, "message"` (using `LuaEngineUtil::PushError` from
  B1), never silently, and never as a Lua error.
- Extend the headless smoke tests, prove each new error path by mutation, and update every
  `@luadoc`.
- A CARAVELLIUS_BREAKAGES entry. It's mostly harmless, since `nil, msg` never raises, but a
  script that reads a getter's old "safe" value sees `nil` instead.

**D3 decisions (the owner, 2026-10-24):** (1) setters that returned nothing return `true` or
`nil, "message"`, like `view_*`; (2) functions that returned `false` (or `0`) return
`nil, "message"`; (3) a wrong argument type is `nil, "argument 'x' must be ..."`, with shared
helpers in `LuaEngineUtil`; (4) a fractional number where an integer is expected is REJECTED
(`3.0` accepted), not rounded down; (5) exceptions: `dofile` keeps raising like standard Lua, and
functions with nothing to report (`input_is_*`, `app_get_*`, `screen_get_*`) only gain the
argument checks; (6) one PR, reviewed "by average".

**D3 as built:**
- **Shared readers** in `LuaEngineUtil`: `ReadIntegerArg`, `ReadIntArg`, `ReadIntegerArgInRange`,
  `ReadOptionalIntegerArg`, `ReadNumberArg`, `ReadNumberArgInRange`, `ReadBooleanArg`,
  `ReadOptionalBooleanArg`, `ReadStringArg`, `CheckFunctionArg`, `ReadConstantArg` (checked
  against the same `stNamedConstant` table that registers the group's globals, so a constant group
  can't drift from its check), plus `FormatNumber`. `luaviewapi.cpp`, `luarendererapi.cpp` and
  `luatextapi.cpp` dropped their private copies of `PushError`/`PushTrue`/`FormatNumber`/
  `ReadIntegerArg`/`ReadBooleanArg`/`ReadOptionalBoolean` for them. Messages start
  `"argument '<name>'"`. Arguments are checked before the first `GetTileMap`/`GetDrawSurface`/
  `EnsureRenderer`, so a refused call never creates the default renderer (tested).
- **The 3 old getters** (`GetSoundManager`, `GetSpritePool`, `GetScriptProcessor`) now pop what
  they push.
- **Every module converted** (about 130 functions in 13 files): sprite and pool, the `sp_*`
  queue, sound and songs, tilemap, camera and zoom, collision, app, input, timers, json, crypto,
  pack, and `draw_text`/`measure_text`. Notable rules: a queued command's parameter is `0..65535`
  (it is kept in 16 bits and used to wrap silently); `sprite_acquire` fails with `nil, "message"`
  for an unknown or full pool (it returned `0`); a sound id is tracked as loaded on the Scarab side
  (`SoundManager` can't be asked), so an id never loaded is `"sound N is not loaded"` and a second
  `sound_load` of an id is refused (sunlight silently kept the first sound); `screen_fade`,
  `sound_set_volume` and `sprite_set_collision_inset` take `0..1`; colours `0..255`; layer ids for
  sprites and collision rules `0..254` (sunlight's fixed `MAX_COLLIDER_LAYERS` array);
  `sprite_add_to_layer` with no map loaded still accepts any id (V8); `draw_text`/`measure_text`
  accept a number as the text, as Lua converts one wherever it expects a string; the `KEY_*`,
  `GAMEPAD_BUTTON_*`, `GAMEPAD_AXIS_*`, `MAP_ALIGNMENT_*`, `FULLSCREEN_STRATEGY_*` and
  `TEXTURE_ANIMATION_MODE_*` arguments must be one of their constants (a misspelt constant name
  is `nil` in Lua and is now reported); tilemap layer opacity keeps V3's clamping. `tools/pack.lua`
  reads the new results. `set_timer`'s background callback now prints and pops its Lua error.
- **Found on the way: a sunlight bug** (reported to sunlight on the owner's request, fixed there
  as I5 and pinned in P6 - see below): in
  `CollisionManager::AddColliderToColliderRule` (`collisionmanager.cpp`), the `find_if` lambda
  indexes `m_ColliderLayerList[id]` (a `std::array` of `MAX_COLLIDER_LAYERS` = 255) BEFORE any
  bounds check, and the check that follows tests the FIRST id twice and never the second, so
  `collision_add_rule(1, 300)` read and stored past the array's end. Scarab now range-checks both
  ids first.
- **Old pin (published v0.1.25 macOS x64):** the new test can't pass there; the old binary
  segfaults on several of its calls (an unknown `MAP_ALIGNMENT_*` value, `tilemap_get_tile` far
  outside the map, `load_json(nil)`), all of which now return `nil, "message"`.
- **Regression test:** `scripts/error_reporting_smoke.py` (+ `scripts/error_reporting_smoke/`,
  ci.yml on all 4 platforms): one check per kind of failure of every module, that no check raises
  a Lua error, that successes return `true`, and that refused calls create no renderer.
  Mutation-checked: fractions truncated instead of refused (10 failures), a double `sound_load`
  accepted (2), setters returning nothing (28), `nil` accepted as a boolean (4).
- **Existing tests updated** for the new results: `renderer_api_smoke` (`sprite_*_world_space` on
  an invalid handle), `tilemap_layer_smoke` (an unknown layer), `font_api_smoke` (`draw_text`
  returns `true`).
- **Checked against Caravellius** (local checkout `22bbaaf`, a temporary trace printing every
  `nil, "message"`, not committed): its headless acceptance passes (`ACCEPT RESULT PASS`) and its
  state fingerprint is the reference (`hash=33897632 maxScore=3000`); the only error it meets is
  `sprite_remove_from_layer` on a sprite not on the layer, which returned `false` before. A grep
  found the real break: 23 `if handle == 0 then` pool-exhausted checks in 18 files (entry 9).
- `plans/CARAVELLIUS_BREAKAGES.md` entry 9; `CLAUDE.md` gained the rule for new primitives.

## Phase V: visual/behavioral re-verification

| # | Item | Status |
|---|------|--------|
| V1 | `MoveCamera*` boundary behavior (BREAKAGES entries 24, 46) - 8 Lua wrappers (`camera_move_*`, `view_move_camera_*`) | DONE |
| V2 | Pixel-snap/scroll-boundary rendering fixes at non-integer zoom (entry 35) | DONE |
| V3 | `nOpacity` 0..255 round-trip (entry 41) | DONE |
| V4 | `KEY_MENU` value change, 82 to 5 (entry 2) | DONE |
| V5 | Tile/text/point objects now drawn where they were invisible before (entries 8, 11, 17) | DONE |
| V6 | Rotated shape/tile/text objects (entries 18-20) | DONE |
| V7 | Shape/text object opacity now respects layer opacity (entry 36) | DONE |
| V8a | Scarab drew every sprite a second time each frame (`SpritePool::UpdateAll`), over the whole frame: layer visibility, layer order and view layer masks never applied to sprites (found in V8) | DONE |
| V8 | `AddSprite` before a map loads is now actually drawn, not silently invisible (entry 39) | DONE |
| V9 | `Sprite::Unload()` leaves no active sequence (entry 47); re-pin to `a6ac4f6` (I6, entry 55) | DONE |
| V10 | `LoadMap` over a loaded map no longer leaks (entry 40) | DONE |
| V11 | The collision lookup no longer reads a wrong tile past the map's far edge (entry 4, A6); tile collisions and lookups in map space, re-pin to `4189199` (I7, entry 56); Scarab's tile handler fixed | DONE |
| V12 | A tile object resized away from its own tile's size takes its object layer's tint (entry 45); layer tint at runtime + group tint/opacity, re-pin to `4274ecb` (I8, entry 57) | DONE |
| V13 | Tiled tile animations run by time, each frame once per cycle (entries 44, 60; moved here from O4); re-pin to `56a73cd4de2868fe30e124e79340ea19d71feae7` (I11) and `tilemap_get_tile_frame` / `tilemap_get_object_tile_frame` | DONE |

**V1 (DONE, 2026-10-22 - V1d confirmed by the owner in a real window).** Not just a rename: the scroll clamp genuinely changed (the last step can now be
partial, reaching the map's true edge where it used to stop a step short;
`MoveCameraDown`/`Right` can now return exactly to origin, where a `< 0` vs `<= 0` bug used
to block the single step that would). ~~Scope: extend the existing alignment/scroll probe
(built for the sunlight v0.29.0 viewport semantics work) to cover this.~~ Corrected when the
item started: that probe was never committed to this repo, so there is nothing to extend.
The "32 call sites" in the first draft was also wrong: Scarab has 8 Lua wrappers
(`camera_move_up/down/left/right` in `luacameraapi.cpp`, `view_move_camera_*` in
`luaviewapi.cpp`) and no other caller.

Sub-tasks, as agreed with the owner:
- **V1a** - a committed headless regression test, run by `ci.yml` on all 4 platforms and
  kept after `master_plan` merges to `main` (it pins sunlight's camera contract, not a
  one-time migration): `scripts/camera_boundary_smoke.py` + `scripts/camera_boundary_smoke/`.
  It uses `resources/tilemap/test.tmx` (640x640) and a 7 px step that divides none of the
  distances. It checks the default view at Scarab's default zoom (3.8125, 1240x900), through
  both `camera_move_*` and `view_move_camera_*(0)`, and an extra view at zoom 1.0 (300x200).
  For each axis: the camera stops exactly on `-ceil(view size / zoom - map size)` (sunlight's
  `Camera::FarCameraLimit`, negated as Lua sees it). The travel is not a whole number of
  steps. It takes `ceil(distance / step)` moves each way and comes back to exactly `0`.
- **V1b** - fix the Lua docs of `camera_move_*` and `view_move_camera_*`: the step was said
  to be "configured on the C++ renderer, not from Lua", the direction was not stated, and
  nothing said the position may not be a multiple of the step.
- **V1c** - the first entry in `plans/CARAVELLIUS_BREAKAGES.md`.
- **V1d** - a real-window check of `samples/camera` by the owner.

**Found while building it:**
- The test was shown failing on the old pin first, as a one-time proof: the published Scarab
  v0.1.25 macOS x64 release binary (sunlight v0.38.0) fails every check on both APIs and both
  views. The old behavior was **not** "a step short of the far edge" as the first draft (and
  sunlight's BREAKAGES 46 wording) suggested. v0.38.0's far limit had a fudge term
  (`+ tile_height`, `+ tile_width / round(zoom)`) and was measured against the viewport's
  far edge including its offset, so the camera went past the map's edge, in whole steps:
  315/427 instead of 314/403 at zoom 3.8125, and 371/469 instead of 340/440 at zoom 1.0.
  Down/right stopped one step short of the origin (at 7, not 0), which is BREAKAGES 24.
  The new pin passes every check.
- The camera position Lua reads is the map point at the view's top-left, the negation of
  sunlight's internal camera (`Camera::GetPosition` returns `-m_Position`). So up/left
  make `camera_get_position()` grow, towards the far edge.
- Confirmed with a scratch headless script: the default view's scroll step stays at
  Scarab's default 1 px after a map loads (only extra views default to the tile size).
- From the owner's real-window check of `samples/camera` (V1d), three sample changes made
  inside V1. The text is now dark, so it can be read over the light sky. sunlight's shared
  zoom keys were added next to the sample's own: `+`/`-` zoom, `=` resets the zoom, on the main
  keyboard and the keypad (sunlight's `samples/shared/zoomkeys.h`). And Home/`=` now reset to
  the sample's starting zoom (`view_set_preferred_zoom`, see D2 for why that is needed).
- Also from that check, still open: semi-transparent layers (`test.tmx`'s
  `background_clouds`, 0.69, and `background`, 0.65) draw darker than in Tiled. It's a sunlight
  bug, present in v0.38.0 too: the frame is drawn into a render texture whose own alpha drops
  under every semi-transparent tile, and `TileMapRenderer::Run` then blends that texture onto a
  black screen (about 70% brightness in those areas). Not tracked in sunlight's plan; how it
  goes to sunlight is the owner's decision.

**V2.** Three real rendering bugs sunlight found and fixed at non-integer zoom - a
rasterization seam between adjacent tiles, a scroll-boundary gap of tens of pixels, and a
`std::round` tie-breaking bug at screen positions straddling x/y = 0. Scope: **this needs
checking at Scarab's own actual default (zoom position 60, factor 3.8125), not only at zoom
1.0** - every game using Scarab's unmodified defaults runs at exactly the kind of
non-integer zoom where sunlight's own three bugs were found live (`samples/sprite`, at its
own default zoom).

**V2 (DONE, 2026-10-23 - every real-window test passed and the change was approved by the owner).** Sub-tasks, as agreed with the owner:
- **V2a, scope check:** no Scarab C++ code does its own pixel snapping. The only
  `std::round` in `src/` validates zoom factors (`LuaRendererApi::ValidateZoomFactor`), and
  Scarab draws no tiles itself (`draw_text`/`draw_filled_rectangle` are screen space,
  unzoomed). Samples at a non-integer zoom: `camera`, `sprite`, `collision` and `tilemap` at
  the default 3.8125, the `views` minimap at 0.375 (`view_fit_to_map`, 240/640; its main
  view is 1.0 and its close-up 2.0, both whole), and `renderer` after one zoom step (0.0625
  each).
- **Bug 2 (the scroll limit) was in effect verified by V1:** `scripts/camera_boundary_smoke.py`
  showed the old pin scrolling past the far edge, and it pins the exact edge on all 4
  platforms. Nothing more is needed here.
- **No headless test for bugs 1 and 3:** they are about which screen pixel a tile lands on.
  The null backend draws nothing, and no Lua function reads pixels back, so no Scarab test
  can see a seam. sunlight covers this on its side with its own tests and GPU measurements;
  Scarab's check is by eye.
- **V2b, the owner's real-window check (2026-10-23), every sample launched one at a time,
  all passed:**
  1. `samples/camera`: slow scrolls, the edges, and zooms 3.81, 5.06, 5.38 and 16.00
     (screenshots), with no seams. Home goes back to 3.81 (V1's preferred-zoom fix).
  2. `samples/sprite` at 3.8125: no jitter and no seams. The camera did not follow Sunny,
     so Sunny left the view at the border (found by the owner). The sample was changed in
     V2 to follow it (below). Screenshots confirm: mid-scroll camera 20,313 with no seams,
     the right edge exactly at camera x **314** (V1's pinned edge), and camera 313 when
     centring Sunny at y 416 (⌊416 + 16 − 118.03⌋).
  3. `samples/views`: the minimap at 0.375 had no seams, with Sunny moving across it,
     and both with the clouds layer hidden (the default: that layer also holds the grass
     islands, `tileset_cloud_soil`, so they look missing until `L`) and shown (screenshot).
  4. `samples/renderer`: zoom steps above and below 1.0 while panning, with no seams.
- **Sample changes made inside V2, from the owner's check:**
  - `samples/sprite`: the camera follows Sunny. Sunny is in world space
    (`sprite_set_world_space`) and is kept on the map. `follow` centres the camera with
    `camera_set_position`, clamped to the map's edges, because `camera_set_position` doesn't
    clamp. Dark text, as in `samples/camera`, plus a camera-position line. The README was
    updated.
  - `samples/renderer`: its zoom keys now match `samples/camera`'s (asked for by the owner),
    the zoom keys every sunlight/Scarab sample shares. Page Up/Page Down or `+`/`-` (main
    keyboard and keypad) zoom, `=` resets the zoom, and Home resets the zoom and the camera.
    These replace `Z`/`X`/`R`; Space still asks for the invalid zoom.
  - `samples/views` and `samples/renderer`: each text line is drawn on a dark translucent
    plate (`draw_text_plate`, sized with `measure_text`). The text crosses both white clouds
    and black areas, where neither light nor dark text alone reads.
- **V2c:** `plans/CARAVELLIUS_BREAKAGES.md` entry 3.

**V3.** `src/lua/luatilemapapi.cpp`'s `GetLayer`/`SetLayer` pass `nOpacity` straight to
Lua - self-correcting now that both directions are 0..255 (an opaque layer used to read
back as 256), but its own `@luaexample` doc comment ("fade a layer out") names a specific
number that should be checked still reads right.

**V3 (DONE, 2026-10-23 - the owner's window check passed and the change was approved).** Scarab passes the value straight through
(`tilemap_get_layer`/`tilemap_set_layer` and their `_by_name` forms), so no code changed. This
one *is* testable headless, because it is numbers, not pixels. Sub-tasks, as agreed with the
owner:
- **V3a:** `scripts/tilemap_layer_smoke.py` + `scripts/tilemap_layer_smoke/`, in `ci.yml` on
  all 4 platforms, on `resources/tilemap/test.tmx`. An opaque layer reads 255, and the
  authored 0.69/0.65 layers read 176/166 (`lround(x * 255)`). By id and by name agree.
  Writing a layer's own values back changes nothing. 0, 1, 128, 254 and 255 round-trip exactly,
  by id and by name. Writing 255 keeps the layer visible. 300 and -5 clamp to 255 and 0. The
  other fields round-trip with the opacity. An unknown layer gives `nil`/`false` (today's
  behavior; D3 may change it). **Shown failing on the old pin first** (the published Scarab
  v0.1.25, sunlight v0.38.0), with 15 failures: opaque 256, 0.69/0.65 both 255, write-back
  256 -> 254, 0/1/128/254 read back 510/509/382/256, and 300/-5 read back 210/515 (no
  clamping). One check passed on the old pin: "writing 255 keeps it visible". Old sunlight
  stored 0.0 for 255 (an invisible layer), but its read formula turned 0.0 back into 255, so
  that bug can't be seen from Lua. The check's comment says so; the other checks catch the
  old pin.
- **V3b:** the docs. `tilemap_get_layer` documents the 0..255 range, 255 = opaque and the
  rounding (0.69 reads 176). `tilemap_set_layer` gained a `@luadoc` covering the range, the
  clamping, `false` for an unknown layer, and that it writes all four values, so read first,
  with hiding a layer also hiding its sprites. The example now fades to 128 while keeping
  the other values.
- **V3c, the owner's real-window check:** `samples/tilemap` changes the *clouds* layer's
  opacity, with the value read back on screen. At first `O` cycled 255/128/0 (screenshots
  confirm each step matches its number, and the round trip restores it exactly). The owner
  then asked for ↑/↓ in steps of 15 (which divides 255, so both ends are exact). Screenshots
  at 15, 120, 135, 195 and 255 show a smooth fade that matches the numbers. The sample's text
  is on dark plates, like `views`/`renderer`, and its README has a controls table.
- **V3d:** `plans/CARAVELLIUS_BREAKAGES.md` entry 4.

**V4.** `src/lua/luainputapi.cpp` exposes `KEY_MENU` to Lua *by value* - the value itself
(82 to 5) is what a script sees, not just an internal rename. Scope: check whether anything
stored the old number (a saved binding, a hard-coded 82).

**V4 (DONE, 2026-10-23 - approved by the owner, merged once CI was green).** Found when the item started: the old 82 was **also `KEY_R`**.
sunlight v0.38.0's `iinputhandler.h` had `KEY_R = 82` and `KEY_MENU = 82`, and its backend
passed the number straight to raylib (`::IsKeyDown( key )`). So in Lua `KEY_MENU == KEY_R`,
and `input_is_key_down( KEY_MENU )` fired on R. Now `KEY_MENU` is 5, raylib's own (Android's
menu button), and sunlight translates by name. No Scarab code changed: the constants table
takes sunlight's values. Nothing in `samples/`, `scripts/`, `docs/` or `resources/` uses
`KEY_MENU`. The only `82`s are y positions in three samples. Scarab has no saved bindings
(there is no Lua action map yet; that's C1). Sub-tasks, as agreed with the owner:
- **V4a:** `scripts/input_constants_smoke.py` + `scripts/input_constants_smoke/`, in
  `ci.yml` on all 4 platforms. The Python driver reads the expected names straight from
  `luainputapi.cpp`'s arrays (110 `KEY_*`, 18 `GAMEPAD_BUTTON_*`, 6 `GAMEPAD_AXIS_*`), so the
  list can't drift. The Lua side checks `KEY_MENU == 5`, `KEY_BACK == 4`, `KEY_R == 82`,
  `KEY_MENU ~= KEY_R`, that every constant exists as an integer, and that **no two in a
  group share a value**. That last check catches the whole class of bug, and the owner chose
  to cover the gamepad groups too. It passes on the new pin, so none of the 134 has a
  legitimate duplicate. **Coverage, added at the owner's request:** the driver also reads
  sunlight's own enums (`KeyboardKey`, `GamepadButton`, `GamepadAxis`, `ControllerType` in the
  fetched checkout's `src/input/iinputhandler.h`, `build/_deps/sunlight-src` or
  `--sunlight-src DIR`) and fails if Scarab's matching array misses one or holds one sunlight
  doesn't have. So a key sunlight adds later and Scarab forgets fails CI. A missing checkout
  fails too, never skips. Today all four match one to one (110/18/6/5), and the
  `CONTROLLER_*` group joined the value checks. Checked by mutation: with `KEY_MENU` removed
  from `s_aKeyboardKeys`, it fails naming `KEY_MENU`. sunlight's other two input enums,
  `InputDispatchMode` and `ActionBindingType`, are deliberately not exposed: they belong to the
  controller manager/action map, which is C1, an open decision. **Shown failing on the old pin first** (the published Scarab v0.1.25):
  "KEY_MENU is 82" and "KEY_MENU and KEY_R are the same number (82)", from both the direct
  check and the no-duplicates check.
- **V4b:** the "Keyboard keys" constants doc now says the values are raylib's own, and that
  `KEY_BACK`/`KEY_MENU`/`KEY_VOLUME_UP`/`KEY_VOLUME_DOWN` are Android buttons a desktop
  keyboard never sends.
- **V4c:** `plans/CARAVELLIUS_BREAKAGES.md` entry 5.
- No real-window check: the change is purely constant values, which the test pins directly.

**V13 (DONE, 2026-10-30; added 2026-10-23, moved here from O4).** O4 said "no animated tiles in any
Scarab map", and so did sunlight's BREAKAGES entry 44. That was wrong for Scarab: `resources/tilemap/test.tmx`'s
tilesets have animated tiles (`tileset_house.tsx` 1, `tileset_house_var1.tsx` 2,
`tileset_monke_variants.tsx` 4), and every sample that loads it shows them. Caravellius has none (checked
by sunlight and here: no `<animation>` anywhere in `../caravellius/resources`). The old plan said "Lua
cannot read a tile's current frame, so no headless test can see it"; under the owner's "cover every item
fully" rule (V12) that was the gap to close.
- **sunlight I11 (asked for by Scarab, PR #235, BREAKAGES 60):** the frame an animated tile shows was a lazy,
  draw-driven state per tile (`__stTileAnimInfo`), so a tile not drawn (off screen, hidden layer, view mask)
  PAUSED and resumed one frame on, and each frame's deadline was set from the draw that noticed it was due,
  so a duration that was not a whole number of frames was held until the next draw (40 ms at 60 fps held
  50 ms) and the cycle stretched. Now the animation is a pure function of time, as Tiled animates: the frame
  shown is (now - LoadMap time) modulo the cycle, walked through the durations. No pause, no drift, no
  state; every cell and tile object of a tile in step. New read-only queries `ITileMap::GetTileFrame( layer,
  cell, gid )` and `GetObjectTileFrame( name, gid )`.
- **A sunlight bug found by this item's test (fixed as PR #237):** an animation whose durations are all 0 ms
  showed its LAST frame (the walk stepped past every zero-length frame, `0 >= 0`), though I11 said the first.
  Reported on the owner's go-ahead; now the first frame, drawn and queried.
- **Re-pin:** `SCARAB_SUNLIGHT_REF` `56a73cd4de2868fe30e124e79340ea19d71feae7`,
  `SCARAB_SUNLIGHT_DISPLAY_VERSION` `feat/master_plan@56a73cd` (via `06009d4`, I11 before the fix); verified
  on `origin/feat/master_plan`; reconfigured in place.
- **Lua:** `tilemap_get_tile_frame(row, col, layerId) -> gid` and `tilemap_get_object_tile_frame(name) -> gid`
  (flip bits removed). sunlight answers only false for every refusal, so Scarab checks the reasons first and
  reports each: no map, no such layer, not a tile layer, a cell off the map, an empty cell; no such object,
  not a tile object. The `@luadoc` ("Animated tiles") explains the time-based animation and the change.
- **Test map:** `resources/tilemap/animation/` (`animation.tmx`, `tileset_anim.tsx`, README) - one tile over
  gids 1 / 6 / 11 for 100 / 50 / 300 ms (50 ms is not a whole number of 60 fps frames), in two cells, flipped,
  far to the right, on a hidden layer and as two tile objects, next to a static tile and an all-0 ms animation.
  (A comment placed before the `.tsx`'s root element made libtmx fail with "missing 'name' attribute in the
  'tileset' element"; it now sits inside the element.)
- **Tests:** new `scripts/tile_animation_smoke` (ci.yml, all 4 platforms). The map loads at a virtual time
  that is not a whole millisecond; then every frame for 300 frames (11 cycles) every copy must equal EXACTLY
  the gid the virtual clock predicts (`llround` ms, as sunlight's `VirtualClock`), also while the layer is
  hidden, masked out of the default view and scrolled off screen; a second query gives the same answer; the
  static tile and the 0 ms cycle never change; every gid is seen; every refusal. `error_reporting_smoke`
  gained one check per kind. Proven: it failed on `06009d4` (the 0 ms cycle read gid 7, the last frame), and
  with sunlight's `FrameGidOf` mutated in the fetched source - frame 0 held twice per cycle (gid 11 at 450 ms
  where 1 was due) and a frame skipped (gid 11 at 100 ms where 6 was due) - then restored.
- **Sample:** `samples/shapes` key 6 shows the map at real speed; the HUD reads the frame back. The owner's
  window check (2026-10-30): every copy and both tile objects change face together (gids 1, 6 - the 50 ms
  flash - and 11 caught), the static and 0 ms tiles stay. Labels respaced after the first look.
- **Caravellius:** no animated tiles, nothing reads `tmx_tile::user_data`; acceptance PASS, fingerprint
  `hash=33897632`. Breakage entry 19.

**V5 (DONE, 2026-10-24 - checked by the owner in a real window and approved; fully planned 2026-10-23, open questions settled 2026-10-24, started 2026-10-24 after C6).** sunlight entries 8, 11
and 17: **text**, **tile** (`OT_TILE`, drawn at their own size or stretched into their box,
clipped to the viewport, animated) and **point** objects (a filled square, `point_size`
property, default one map unit, scaled by zoom) are now drawn. Up to v0.38.0 all three were
invisible, so a map that used one as hidden data or a marker would now show it. The row used
to list entry 1 too, but that is the non-orthogonal refusal, already O1, so it was dropped.

**Map audit (2026-10-23, measured):**

| Map | Object layers | Objects | Text / tile / point |
|---|---|---|---|
| Scarab `resources/tilemap/test.tmx` (Scarab's only map) | 0 | 0 | none |
| Caravellius `resources/tilemap/corsair/corsair.tmx` | 7 | 3 | none: all 3 are rectangle triggers (`SubBossIntervalStart`, `SubBossIntervalEnd`, `CephalonPosition`) |
| Caravellius `resources/tilemap/cover/cover.tmx` | 6 | 0 | none |

The Caravellius figures come from the local checkout `../caravellius` (commit `22bbaaf`,
2026-09-23), read-only. So nothing changes on screen in either project. Scarab's only object
function, `tilemap_get_object_by_name`, reads an object's position and size, not how it is
drawn, so it is unaffected.

**Side finding, O3, for Caravellius:** O3 (shape outlines scale with zoom) is "not
applicable" for Scarab, which has no shape objects. But Caravellius' 3 trigger markers are
rectangles (`OT_SQUARE`) on an object layer with no `visible="0"`. sunlight drew rectangles
before (v0.38.0 has `case OT_SQUARE`) and still does, so they are probably drawn in the game,
now with zoom-scaled outlines. Whether they are actually visible (colour, opacity, where the
camera goes) can't be told from the map alone. That goes in V5c below.

**Sub-tasks (agreed with the owner 2026-10-23):**
- **V5a:** record the audit above (done in this text).
- **V5b, a new `samples/objects` sample**, asked for by the owner so V5 has something to see
  by eye. It is built from sunlight's own object samples, read from the local sunlight repo at
  the pinned commit (`git -C ../sunlight show 9c8c6bf:samples/...`), from the vendored
  `build/_deps/sunlight-src`, or by asking the sunlight session:
  - `samples/objects/resources/map/objects.tmx` (60x40, 32 px): 6 **tile** objects, one row
    per case (own size, stretched to a 64 px box, cut by the left viewport edge, animated,
    flip bits ignored, cut by the bottom edge), each with a numbered **text** object label.
    It uses `tileset_house.tsx` + `images/house.png`, the same house tileset Scarab already
    ships in `resources/tilemap/`. A green rectangle object frames the viewport. sunlight's
    `docs/README.md` for that sample lists what to look for per case.
  - **point** objects: only `samples/shapes/resources/map/thick.tmx` has any (3, with 11 text
    objects and 9 shapes). `samples/text/resources/map/text.tmx` has 17 text objects.
  - Shape (`rotation.tmx`, `shapes.tmx`, `stress.tmx`) and opacity maps (`opacity.tmx`,
    13 text + 7 tile objects) exist too. They cover V6 (rotation) and V7 (opacity), so pick
    the maps with those items in mind: one Scarab map per item, or one shared objects map.
  - The three open questions, settled with the owner (2026-10-24):
    1. **Font:** text objects draw only with a registered font, otherwise `??????`, so the
       owner promoted C6 (fonts by handle, incl. `register_font`), done right before V5. The
       sample loads `resources/fonts/caravellius8x8.fnt` once with `load_font`, registers it
       as "Sans", and can use the same handle for its HUD text.
    2. **Location:** a new `resources/tilemap/objects/` with its own README and provenance
       (from sunlight's `samples/objects` at `9c8c6bf`), reusing the house tileset already in
       `resources/tilemap/`.
    3. **Zoom:** start at 1.0 (sunlight's sample does too), so all six cases and labels fit,
       with the shared zoom keys to look closer. Adjust after the owner's eye test.
  - Controls follow the shared scheme (arrows pan, Page Up/Down or `+`/`-` zoom, `=` reset
    zoom, Home reset), with the text on dark plates. The owner checks it in a real window.
    Add it to `samples/README.md` and to the VSCode template `launch.json` picker.
- **V5c:** `plans/CARAVELLIUS_BREAKAGES.md` entry: text, tile and point objects in a map are
  now drawn (Caravellius' maps have none today, so any marker added later should be a
  rectangle or sit on a hidden layer), and its 3 rectangle trigger markers, probably already
  drawn, now have zoom-scaled outlines (O3). Check they are hidden or harmless.
- **V5d:** the plan write-up, CI green, then DONE.

**Built (2026-10-24):**
- **Point objects decided (the owner):** V5 keeps to `objects.tmx`, tile and text objects, so
  it stays close to a sample sunlight has tested. Point objects (only in sunlight's
  `thick.tmx`) come with V6/V7's shape maps.
- `resources/tilemap/objects/objects.tmx`: sunlight's `samples/objects` map at `9c8c6bf`. The
  only change is its tileset path, `../tileset_house.tsx`, because sunlight's copy of the house
  tileset and its image are **byte-identical** to Scarab's in `resources/tilemap/`. A comment in
  the map and the folder's new README say so. Checked headless: the map loads (60x40, 32 px),
  `tilemap_get_object_by_name( "viewport" )` gives (100, 100, 1000, 800), and "Sans" registers.
- `samples/objects` (+ `docs/README.md`, listed in `samples/README.md` and in both VSCode
  `launch.json` pickers): the layout of sunlight's own sample (render 1260x920, viewport
  1000x800 at (40, 40), zoom 1.0 with `preferred_zoom` 1.0, map `TOP_LEFT`, camera at (100, 100),
  scroll step 4). `load_font` loads Caravellius 8x8 once, `register_font( "Sans", ... )` gives it
  to the map's text objects, and the same handle draws the HUD. Shared controls (arrows/WASD pan
  in the same direction as `samples/camera`; Page Up/Down or `+`/`-` zoom; `=`; Home resets zoom
  and camera).
- **The owner's real-window check (2026-10-24, screenshots at zoom ~0.75, 1.0, 1.44, 1.69,
  panned):** every label is drawn from the map's text objects in Caravellius 8x8, with no
  `??????`, and the title in its own yellow. Case 1 is at own size, 2 stretched, 3 cut by the left
  edge (whole once panned left), 5 identical to 1, 6 cut by the bottom edge. Case 4 animates: its
  tile showed a different frame in each screenshot. Long labels are clipped cleanly at the
  viewport's right edge, and everything scales together when zooming. Expected, not a bug: the
  green frame's top and left lines lie on the viewport's clip border at the start (the frame is
  the viewport's own edges), and its right and bottom lines move inside when zoomed out. The
  first HUD's second line overflowed the window; it was split into two shorter lines, re-checked.
- No headless test: drawing can't be seen headless, and nothing in Lua reads an object's drawn
  pixels. The map loading and font registering are covered by the checks above and by C6's font
  test.
- V5c: `plans/CARAVELLIUS_BREAKAGES.md` entry 7.

**V6 (DONE, 2026-10-24).** sunlight entries 18-20: rectangles, polylines, polygons
(D1a), ellipses (D1b), tile objects and text objects (D1d) with a Tiled rotation are drawn turned
clockwise about their stored point; before, the rotation was ignored. The new `IEngine`/`IFont`
methods only concern implementers, and Scarab implements neither.
- **Audit:** no map in either project has a rotated object, a tinted object layer or a
  per-object opacity (Scarab's `test.tmx` and `objects/objects.tmx`; Caravellius' `corsair.tmx`
  and `cover.tmx`), so nothing changes on screen. That also answers the map side of V7 and V12.
- **Decisions (the owner):** one `samples/shapes` with keys `1`/`2`/`3` switching maps at runtime
  (which also exercises V10, `LoadMap` over a loaded map); no `stress.tmx` (a benchmark, and
  Scarab has no stress tests); V7's map is decided when V7 starts.
- **Found while building it: a sunlight bug.** `rotation.tmx`'s embedded tileset image, pointed at
  Scarab's byte-identical `../../sprites/sunny_idle_down.png`, made `tilemap_load_map` refuse the
  map ("an error occured in the delegated image loading function"). The same path without `..`
  loaded. The cause, in sunlight's `TextureLoaderCallback` (`mapdata.cpp`): `..` was collapsed
  only for external tilesets' images, and PhysFS rejects `..`. Reported to sunlight on the
  owner's request. The owner chose to wait for the fix rather than copy the image (V6 on hold).
  sunlight fixed it (I4, PR #221, BREAKAGES 53; every image path now goes through
  `ExternalResources::JoinVirtualPath`, with tests incl. from a scrambled `.zip`).
- **Re-pin (inside V6, the owner's choice):** `SCARAB_SUNLIGHT_REF`
  `f651accc2291a2ea9020dad3bfc2f1245e76858f`, `SCARAB_SUNLIGHT_DISPLAY_VERSION`
  `feat/master_plan@f651acc`. Verified: on `origin/feat/master_plan`, descends from `9c8c6bf`
  (3 commits: the handoff record, `CODE_STYLE.md`'s forward-declaration rules, the fix; 1 `src`
  file). Real fetch exact, clean build, `--version` shows `feat/master_plan@f651acc`, every
  smoke test, the docs generator and all 17 samples pass, and `rotation.tmx` now loads.
- **Built:** `resources/tilemap/shapes/` (`shapes.tmx`, `rotation.tmx`, `thick.tmx` from
  sunlight's `samples/shapes` at `9c8c6bf`; only `rotation.tmx`'s image path changed; README
  with provenance, linked from `resources/tilemap/README.md`). `samples/shapes`: sunlight's
  layout (viewport 1000x800 at (40, 40), zoom 1.0, camera at the origin, scroll step 4),
  Caravellius 8x8 loaded once and registered as "Sans" (it survives every map switch, since the
  registration is the renderer's), shared controls, a three-line HUD that fits. Listed in
  `samples/README.md` and both VSCode pickers.
- **The owner's real-window check (2026-10-24, screenshots):** map 1's shapes are clipped
  cleanly at the viewport, also at zoom 1.75 and 0.38. In map 2, every object is turned about
  its pivot: rectangle, polyline and polygon at 90 and 45, the ellipse at 45, Sunny's tile objects
  at 90 and 45, "turned text" vertical, "text 45" diagonal. In map 3, line widths 1/2/3/5 are
  distinct, the point objects are filled squares, and both scale at zoom 1.25 and 1.50.
  Switching maps was clean, with no `??????`.
- **The point objects V5 deferred** are covered by map 3.
- `plans/CARAVELLIUS_BREAKAGES.md` entry 8.

**V7 (DONE, 2026-10-24).** sunlight entry 36 (L2): a Tiled object's own `opacity` custom
property (0 to 1) overrides its layer's opacity, and shape and text objects follow layer opacity at
all (before, only tile objects did; shapes and text were always drawn at their colour's own alpha).
- **Audit:** no object layer below opacity 1.0 and no object `opacity` property in any Scarab map
  or in Caravellius' `corsair.tmx`/`cover.tmx`, so nothing changes on screen for either.
- **Decisions (the owner):** the map goes to `resources/tilemap/objects/opacity.tmx` (next to
  `objects.tmx`, same `../tileset_house.tsx` path); it is key `4` of `samples/shapes`, the
  extensible map switcher V6 built; no automated test (a headless run can't see opacity; that
  the map loads is checked), the owner's window check instead.
- **Built:** `opacity.tmx` from sunlight's `samples/opacity` at `f651acc` (tileset and image
  byte-identical to Scarab's; only the tileset path changed, noted in the map and the objects
  README). `samples/shapes` gained per-map starting cameras (map 4 starts at (100, 100), where its
  frame is, as in sunlight's sample; `Home` returns to the current map's start). Docs: the objects
  README, `resources/tilemap/README.md`, the shapes sample README and `samples/README.md`.
- **The owner's real-window check (2026-10-24, screenshot):** tile objects, rectangle outlines and
  text each fade 1.0 / 0.75 / 0.5 / 0.25 with the 0.0 one invisible; the two row-4 tiles (no
  property, invalid `5`) look the same at the layer's 0.4; labels in Caravellius 8x8, no `??????`.
- `plans/CARAVELLIUS_BREAKAGES.md` entry 10.

**V8a (DONE, 2026-10-27).** Found while building V8's `samples/mapless`: its layer keys
changed the view's layer mask (`view_is_layer_shown` read it back) but nothing disappeared. sunlight
was right (`DrawPassContent` checks the active view's mask, also for mapless layers); Scarab was the
cause. `EngineHost::RunStageStateHandler` called `SpritePool::UpdateAll()` every frame: a second
`Sprite::Update()` (`Advance()` + `Draw()`) of every live pool sprite, AFTER sunlight's layer pass
had already advanced and drawn each one, on top of the whole frame and with no layer or mask check.
It predates the Scarab/Caravellius split. So hiding a layer (`tilemap_set_layer(id, false, ...)`,
an authored `visible="0"`) never hid its sprites, a view's layer mask never hid them in that view,
and every sprite was drawn above the map layers meant to cover it.
- **Measured before deciding (the owner's request):** with the call removed, `samples/mapless`' layer
  keys really hide sprites (owner's window check); animation pace unchanged (a temporary probe, never
  committed, counted a 4-frame `AUTOMATIC_RIGHT_LEFT` sprite's tile changes over 600 headless
  frames: 101 with and without the call, with and without a map - each step waits for its delay);
  Caravellius' acceptance PASS and its state fingerprint the reference `hash=33897632` (5 runs); and
  a sprite never added to a layer is invisible even WITH the call (owner's window check of a probe),
  so removing it hides nothing that is visible today.
- **Fix:** the call and `SpritePool::UpdateAll()` are removed; sunlight's layer pass is the one place
  a sprite is advanced and drawn. A comment at the call site says why.
- **The owner's window checks (2026-10-27, screenshots):** `samples/sprite`: Sunny (layer 4,
  `clouds`) now goes under the `houses` roof and the `monke` layer above him; `samples/mapless`: keys
  `1`/`2`/`3` hide and show each sprite, all off leaves only the HUD; `samples/views`: Sunny once in
  the main view, the minimap and the close-up, `L` toggles the minimap's clouds only, `C` the close-up.
- No automated test: it is purely drawing, which a headless run can't see.
- `CLAUDE.md`'s layer-visibility gotcha corrected (it was only true for drawing since this fix);
  `plans/CARAVELLIUS_BREAKAGES.md` entry 11.

**V8 (DONE, 2026-10-27).** sunlight entry 39 (M2): a sprite added with `AddSprite` while
no map is loaded is now drawn (before: accepted, collider updated, never drawn); with no map, the
layer ids draw in ascending order and a view's layer mask hides them by id; `Stop()`/the destructor
now also release sprites registered with no map (they leaked). With a map loaded nothing changes.
- **Audit:** every Scarab sample and test script loads its map before adding a sprite. Caravellius:
  a temporary trace (never committed) printing every `sprite_add_to_layer` with no map loaded counted
  ZERO over its acceptance and state fingerprint runs (both still the reference); the same trace
  fired on a 5-line script that adds a sprite before any map, so the zero is real. Not covered:
  code reached only by player input.
- **Decisions (the owner):** a new `samples/mapless`; the runtime audit as above; a breakage entry
  either way.
- **Built:** `samples/mapless` (no map: shadow, Sunny and glow on layer ids 1-3, Sunny moved with the
  arrows, `1`/`2`/`3` toggle each layer through `view_show_layer`, the shared zoom keys, a four-line
  HUD on plates), its README, `samples/README.md` and both VSCode pickers. Docs: `CLAUDE.md`'s
  "sprite setup before the map loads" gotcha (a sprite added early is now drawn over an empty
  background), `samples/sprite`'s header comment, and `sprite_add_to_layer`'s `@luadoc`.
- **Found while building it:** the layer keys changed the mask but hid nothing - Scarab drew every
  sprite a second time (V8a, fixed and merged first).
- **The owner's real-window check (2026-10-27, screenshots, after V8a):** all three sprites draw with
  no map, Sunny over the shadow and under the glow, each key hides its sprite and the HUD matches,
  all off leaves only the HUD, zoom scales everything.
- `plans/CARAVELLIUS_BREAKAGES.md` entry 12.

**V9 (DONE, 2026-10-29).** sunlight entry 47: `Sprite::Unload()` leaves no active sequence
(before, the active sequence stayed marked valid over freed memory), and `UnloadMap()`/`Stop()`
unload every sprite registered on a layer. Loading a map over a loaded one does not.
- **Audit:** neither Scarab's samples and scripts nor Caravellius call `tilemap_unload_map` (the only
  call is D3's error test, with no map loaded). A reconfigure after an unload works: `Sprite::Unload`
  empties the sequences, so `sprite_configure_texture` takes its "add" path, and
  `sprite_set_active_sequence` returns `nil, "sequence N is not configured"` until then.
- **Found: a sunlight bug.** `UnloadMap` unloaded and unregistered the sprites but left their
  colliders in the `CollisionManager`. Measured on the `fa953f3` pin (a temporary headless script, two
  overlapping sprites on layers 2 and 3 of `test.tmx`, a rule between them, collision calls per 60
  frames): map loaded 60; map unloaded **60** (unloaded sprites kept colliding); map loaded again and
  sprites re-added **240** (each collider registered twice). The published v0.1.25 segfaulted
  (exit 139) on the frame after the unload. Reported to sunlight on the owner's request; fixed there
  as I6 (PR #225, BREAKAGES 55): `UnloadSprites()` removes each unloaded sprite's own collider, for
  `UnloadMap`, `Stop()` and the destructor; a collider the game added itself and the collision rules
  are kept.
- **Re-pin (inside V9, the owner's choice):** `SCARAB_SUNLIGHT_REF`
  `a6ac4f6bea9973795c1668dd8eabfc67defc03c1`, `SCARAB_SUNLIGHT_DISPLAY_VERSION`
  `feat/master_plan@a6ac4f6`. Verified on `origin/feat/master_plan`, two commits after `fa953f3`
  (the I5 pin record, docs only, and the fix; 3 files in `src/`, no public API change).
  Reconfigured in place (not a clean `build/`), fetch exact.
- **Checks:** clean build, `--version` shows `feat/master_plan@a6ac4f6`; every smoke test (incl. the
  new one), the headless test and its budget run, the tileset archive test (plain and encrypted),
  the docs generator and all 19 samples pass; Caravellius' acceptance PASS and its reference state
  fingerprint (`hash=33897632`, 3 runs).
- **Regression test:** `scripts/unload_map_smoke.py` (+ `scripts/unload_map_smoke/`, ci.yml on all 4
  platforms): the same scenario, expecting exactly 60 / 0 / 60, `-1` as the active sequence after the
  unload and `set_active_sequence` refused until reconfigured. Passes at `a6ac4f6`; the measurement
  above is what it rejects at `fa953f3`; the published v0.1.25 crashes on it (exit -11).
- **Docs:** `tilemap_unload_map`'s `@luadoc` (every sprite on a layer is unloaded and taken out of
  collisions; set it up again after the next map; the rules stay; a map loaded over a map unloads
  nothing). No window check: collisions and sequence state are counted headless.
- `plans/CARAVELLIUS_BREAKAGES.md` entry 13.

**V10 (DONE, 2026-10-29).** sunlight entry 40 (T2): `LoadMap` over a loaded map frees the
old map (tile textures, Resource Manager, tile animations); before, it overwrote the map pointer and
leaked the whole previous map on every switch. Sprites stay registered across it, as before. The new
map is parsed first, so a map that fails to parse leaves the loaded map loaded (before, the old map
was lost and no map was left).
- **Measured (headless, peak memory, `test.tmx` loaded over itself; no GPU, so the CPU side only):**
  current pin 10 loads 11 MB, 200 loads 11 MB; published v0.1.25 10 MB and 16 MB. In the final test:
  20 vs 1000 loads, +0.0-0.1 MB at the pin, **+27.9 MB** on v0.1.25 (about 30 KB per load).
- **Audit:** Caravellius switches stages exactly this way (`core/bootstrap.lua`, no unload), so it
  leaked every previous stage and is fixed with nothing to change; its failure path
  (`if not tilemap_load_map(...)` -> print, `return false`) works with `nil` and now leaves the
  previous stage's map loaded. `samples/shapes` switches maps on keys 1-4 the same way.
- **Decisions (the owner):** test the leak in CI too, option (a) - a Python peak-memory comparison on
  Linux/macOS (Windows has no peak-memory reading in Python's standard library; the functional
  checks still run there).
- **Regression test:** `scripts/load_map_smoke.py` (+ `scripts/load_map_smoke/` with a deliberately
  malformed `broken.tmx`; ci.yml on all 4 platforms). Each of two runs (`LOADS` = 20 and 1000):
  `broken.tmx` returns `nil, "cannot load map ..."` and `tilemap_get_map_info` still reports
  `test.tmx` (20x20, 32 px), a sprite added to a layer is still on it after the loads, and the
  peak memory of the 1000-load run is at most 8 MB above the 20-load one (`os.wait4`'s `ru_maxrss`,
  bytes on macOS, KB on Linux). Fails on v0.1.25: no map left after `broken.tmx`, and +27.9 MB.
- **Docs:** `tilemap_load_map`'s `@luadoc` (a load over a loaded map frees the old one and keeps
  sprites; a failed load keeps the loaded map; one map at a time, shown by every view).
- **Asked during V10 (the owner):** does multi-view mean several maps? No - checked in sunlight's
  `multiview`/`multiview_overlap` samples, `IView` (no map of its own) and `TileMapRenderer` (one
  `m_pMap`): views are cameras on the one loaded map. The owner added C7 (several maps at once).
- `plans/CARAVELLIUS_BREAKAGES.md` entry 14.

**V11 (DONE, 2026-10-30).** sunlight entry 4 (A6): the collision lookup refused only a
negative row/column, so a position past the map's far edge read past the layer's tile array.
Checking it opened up the whole sprite-vs-tile path, which no project had ever seen work: no tileset
in Scarab or Caravellius has a collision shape (only sunlight's isometric/staggered/hexagonal
obstacles, whose samples deliberately ignore the callback because of "spurious hits").
- **A Scarab bug, from before the split:** `LuaCollisionListener`'s tile callback pushed six values
  and called `lua_pcall(..., 5, ...)`, so Lua called the sprite handle ("attempt to call a number
  value") and `collision_set_tile_handler` never ran in any Scarab release. Fixed (5 -> 6).
- **Measured on the `a6ac4f6` pin (with that fix), the owner shown first:** a 32x32 sprite swept past
  one collision tile (map 160..191), default viewport (10,10). It collided for x 150..180 instead of
  129..191 (only the tile under its top-left corner, shifted by the viewport origin); with the camera
  at (64,64) a world-space sprite collided 64 px away from itself (the camera added again) and never
  with the tile it was on; the tile came back camera-relative (96,96). Zoom made no difference.
  Reported to sunlight on the owner's request; fixed as I7 (PR #227, BREAKAGES 56), which also found
  three more: a tile with two collision shapes hung the frame, the bounds check and the cell index
  disagreed about zoom, `GetTile` with no map dereferenced null. The new model: everything in MAP
  space - `TileMapToTileMatrix` takes a map position, `GetTile`/the collision callback give the tile's
  map rectangle, a collision tests every tile the collider overlaps (view-relative sprites with the
  camera, world-space as is), one callback per tile, half-open overlap (touching edges don't count).
- **Re-pin (inside V11):** `SCARAB_SUNLIGHT_REF` `4189199f7b3af75e590792f85d51051916f4a206`,
  `SCARAB_SUNLIGHT_DISPLAY_VERSION` `feat/master_plan@4189199`. Verified on `origin/feat/master_plan`,
  two commits after `a6ac4f6` (the I6 pin record and the fix, 18 files in `src/`). Reconfigured in
  place, fetch exact, content key kept.
- **Regression test:** `scripts/tile_collision_smoke.py` (+ `scripts/tile_collision_smoke/` with its
  own `collision.tmx`; ci.yml on all 4 platforms), at zoom 1 and 3.8125: `tilemap_to_tile_matrix` /
  `tilemap_get_tile` in map space; the exact windows (view-relative camera 0: 129..191; camera 64:
  65..127; world-space: 129..191 whatever the camera, none two rows away); the tile's map rectangle,
  gid and the handle in the callback; two calls a frame for a sprite on two tiles; one for a tile with
  two shapes (hung before I7); the far corner tile, and nothing past the far edge. Fails with Scarab's
  old argument count (no collision reaches Lua - checked by mutation), and the `a6ac4f6` measurement
  above is what it rejects.
- **Docs:** `collision_add_tile_rule` (only tiles with a collision shape; map space; touching edges
  don't count), `collision_set_tile_handler` (map rectangle; once per tile touched; undo a frame's
  movement once and all of it - sunlight's lesson from its own sample), `tilemap_get_tile` (map
  rectangle), `tilemap_to_tile_matrix` (a map position; it said "world/screen pixel coordinate").
- **`samples/collision` (the owner's choice: wall tiles):** now on `resources/tilemap/collision.tmx`
  (sunlight's collision sample map at `4189199`: `test.tmx` plus a `walls` layer, id 9, drawn with the
  new `tileset_wall.tsx` and `images/wall.png`, unmodified). Sunny (world-space) starts inside a box
  of brick walls that stop her from every side (the handler undoes the frame's movement once), the gap
  on the right lets her out to the second Sunny (hits counted as before), the shared zoom keys, HUD on
  plates. Created with `renderer_create{ zoom = 3.8125, preferred_zoom = 3.8125 }` so a zoom reset returns
  to the start zoom (the owner noticed the default renderer resets to 1.0). The owner's real-window
  check (2026-10-30, screenshots): flush against a wall and in a corner at zoom 3.81 and 4.56, the
  gap and the sprite hit at zoom 1.0, and the zoom reset back to 3.81.
- `plans/CARAVELLIUS_BREAKAGES.md` entry 15.

**V12 (DONE, 2026-10-30).** sunlight entry 45: a tile object stretched away from its tile's
size takes its object layer's tint (before, it was drawn untinted). The owner asked for V12 to be
covered fully rather than kept small (after what testing everything found in V11): "100% compliant
with the new sunlight features, even if it breaks Caravellius".
- **Audit:** no map in Scarab, Caravellius or sunlight's samples used `tintcolor`; at the `4189199`
  pin a tint existed only in the map file (`stLayer` had no tint), so nothing could read or change it.
- **Decisions (the owner):** a Scarab-made tint test map with every case; key `5` in `samples/shapes`;
  a runtime tint API, option (b) - asked of sunlight first, V12 held until its pin.
- **sunlight I8** (PR #229, BREAKAGES 57): `stLayer::tint` (RGBA, opaque white by default), read by
  `GetLayer` from the authored `tintcolor` alpha included, for tile, image, object and group layers,
  written by `SetLayer`; RGB multiplies into what the layer draws, alpha kept but ignored for drawing.
  Also: group layers drawn as Tiled draws them (a group's opacity and tint multiply into its children;
  shapes and text take only the opacity), and image layers apply their own opacity. Caravellius'
  one image layer has no opacity, so nothing changes for it.
- **Re-pin (inside V12):** `SCARAB_SUNLIGHT_REF` `4274ecbf6cdd71c3625254f68ae14e0ed6631acb`,
  `SCARAB_SUNLIGHT_DISPLAY_VERSION` `feat/master_plan@4274ecb`; verified on `origin/feat/master_plan`,
  two commits after `4189199` (the I7 pin record and the feature); reconfigured in place.
- **Lua API (separate functions, the owner's choice over extending `tilemap_set_layer`):**
  `tilemap_get_layer_tint(layerId) -> r, g, b, a`, `tilemap_set_layer_tint(layerId, r, g, b, a)` and
  their `_by_name` forms; strict `0..255`, `nil, "message"` on failure, read-before-write so the
  layer's other values are kept; `tilemap_set_layer` keeps the tint.
- **Regression test:** `scripts/layer_tint_smoke.py` (+ `scripts/layer_tint_smoke/`, ci.yml on all 4
  platforms) on `resources/tilemap/tint/tint.tmx`: every authored tint read back exactly (alpha 64
  kept; white without one; image, object and group layers), by name, set then get, a write-back
  changing nothing, `tilemap_set_layer` keeping the tint and the tint keeping the other values, every
  error. Mutation-checked (the alpha not stored: the set-then-get checks fail).
- **The map and the sample:** `resources/tilemap/tint/tint.tmx` (Scarab-made, README) with five
  labelled rows - tile layer, image layer (incl. opacity 0.5), tile objects own/turned/stretched with an
  untinted shape and text, tint alpha ignored vs opacity, a group's tint and opacity on its children.
  Key `5` in `samples/shapes`; `T` cycles a layer's tint and `G` a group's opacity at runtime.
- **The owner's real-window check (2026-10-30, screenshots):** the first look found three sample
  problems, all fixed: an overlapping label, invisible rectangles (their object layers had no colour),
  and a red tint barely visible on red roof tiles (now cyan). Then: every row as described, `T` and
  `G` working, shapes and text untinted, the group's opacity on its rectangle and text.
- `plans/CARAVELLIUS_BREAKAGES.md` entry 16.

## Phase O: confirmed not applicable, recorded anyway

Every reason below was re-checked against the code on 2026-10-23, after the pin had moved
twice (P4, P5). Twelve held. Two did not: O4 (moved to V13) and O8 (reworded below).

| # | Change | BREAKAGES entry | Why |
|---|--------|------------------|-----|
| O1 | Non-orthogonal maps refused | 1 | All Scarab maps are orthogonal |
| O2 | `SetPixel` removed | 15 | Zero calls anywhere in Scarab (confirmed by grep across `src/`, `resources/`, `docs/`, `samples/`, `README.md`) |
| O3 | Polyline/polygon/rectangle/ellipse width-by-zoom | 16 | No polyline/shape object in any Scarab map |
| O4 | ~~Tile animation first-frame-double-duration fix~~ - **moved to V13** (2026-10-23) | 44 | ~~No animated tiles in any Scarab map~~ - wrong: `test.tmx`'s tilesets have animated tiles, see V13 |
| O5 | Layer parallax placement correction | 34, 49 | No parallax in any Scarab map, sample or source (confirmed by grep) |
| O6 | Listener dispatch reentrancy now well-defined | 42 | Scarab registers its listeners once at startup, never from inside a dispatch |
| O7 | Isometric/staggered/hexagonal maps now load | 21-23 | All Scarab maps are orthogonal; new capability, not a requirement |
| O8 | TTF/OTF accented glyphs | 43 | Applies, harmless: `samples/text` really loads a TTF (`resources/fonts/pressstart2p-regular.ttf`; the first reason, "Scarab's docs name a TTF only as an example", was wrong, found 2026-10-23). ASCII text is drawn exactly as before, and accented letters now draw instead of `?`. Nothing to do |
| O9 | `IEngine` gains `LoadFont`/`BeginClip`/`EndClip` | 6 | Scarab doesn't implement `IEngine` |
| O10 | `SetWindowBackgroundColor` takes `stColor` | 10 | Nothing in Scarab calls it |
| O11 | `doc/TODO.txt`/`FIXME.txt`/`MISSING_FEATURES.md` removed from sunlight | 3 | No link to them anywhere in Scarab |
| O12 | `IFont` is a new interface in its own module | 9 | Only code that uses `IFont` directly; Scarab doesn't |
| O13 | `FileSystem::Mount` sets PhysFS up itself, so it can be the first call | 12 | Scarab already calls `Init(argv[0])` first |
| O14 | Sample fonts moved to `samples/shared/fonts` in sunlight | 13 | Samples only, no API change |

## Phase C: new capabilities, not required

**Why "not required":** each item is a new sunlight capability that Scarab doesn't expose yet.
Scarab compiles and behaves exactly as before without it, so whether to offer it to Lua games
is a product decision, not integration work. B1 (blend modes) was one, promoted by the owner.
**When the plan reaches this phase, ask the owner, item by item, whether to make a detailed
plan for it** (the owner's instruction, 2026-10-23). None is started or scheduled without
that.

| # | Item | BREAKAGES entry | Scope |
|---|------|------------------|-------|
| C1 | `GetControllerManager`/hot-plug/player slots/action map/haptics | 25, 27-33, 48 | Whether/when to expose any of this to Lua - a real product decision, not a breakage fix |
| C2 | `IShapeSurface` (shapes without a loaded map) | 38 | HUD/debug drawing - defer or take |
| C3 | Sprite z-order | 37 | Expose `sprite_set_z_order`? |
| C4 | Layer parallax (a code API, not just Tiled-declared) | 34, 49 | Defer until a map actually wants it |
| C5 | Non-orthogonal map support, surfaced to game authors | 21-23 | Tell them it's possible; no API change needed, `tilemap_load_map` already takes any map |
| C6 | Fonts by handle: re-pin to sunlight `9c8c6bf` (I3, entry 52) + a handle-based Lua font API, incl. `register_font` for text objects | 7, 52 | **Promoted by the owner (2026-10-24): DONE, done right before V5** - see below |
| C7 | Several maps loaded at the same time, each view showing one of them (split-screen with players in different areas, a world map beside the local one, a picture-in-picture dungeon, a menu over a different map, a cross-fade between two maps) | - | **Added by the owner (2026-10-29, during V10).** Needs sunlight first: plan it TOGETHER WITH sunlight when C7 starts - see below |

**C6 (DONE, 2026-10-24 - tested by the owner in a real window, reviewed and approved; promoted by the owner 2026-10-24, done right before V5, out of phase order;
reshaped the same day into "fonts by handle").**

*How it got here.* C6 began as "expose `RegisterFont( family, path, bold, italic )`". Without a
registered font, sunlight draws every Tiled text object as `??????` (`textobjectrenderer.cpp`,
`__MISSING_FONT_PLACEHOLDER`), and V5's `samples/objects` labels every case with text objects.
Planning it showed that `set_font` (for `draw_text`) and text objects would each load their own
copy of the same file, because sunlight's `FontRegistry` took only a path and owned a
`unique_ptr<IFont>`. The owner re-planned font handling with sunlight (as its 2026-10-04 Phase B
discussion had started), and sunlight shipped **I3, "fonts by handle"** (PR #218, pin
`9c8c6bf421a1147d1e8c099e698dfbe7c1885af2`, on top of `bfce598`; sunlight BREAKAGES entry 52,
**deliberately breaking**: the owner chose to break compatibility).

*sunlight's model at `9c8c6bf`: load once, then use the handle.* A font is loaded once into a
`std::shared_ptr<SunLight::Font::IFont>`. The same handle can be the active font for
`DrawText`/`MeasureText`, be registered for any number of text-object families and styles, and be
drawn with directly. One file means one load and one GPU copy.
- `IDrawSurface::LoadFont( path )`: new. It returns the handle, or `nullptr` if the file can't load
  or the renderer isn't started. It reads TTF/OTF (with Latin accents) or BMFont `.fnt`, through
  the filesystem mounts.
- `IDrawSurface::SetFont( font )`: takes a handle now, not a path. `nullptr` selects the built-in
  font. It returns `false`, keeping the active font, for a font another backend loaded or an
  invalid one. `IDrawSurface::GetFont()`: new, the active font or `nullptr`.
- `TileMapRenderer::RegisterFont( family, font, bold, italic )`: takes a handle, not a path. It is
  on the concrete renderer only, not on `IDrawSurface`. Registering a key again replaces its font,
  and one font may fill several keys. `UnregisterFont( font )` (every slot that font serves) and
  `UnregisterFont( family, bold, italic )` (one slot) are new. An unregistered family draws
  `??????` again, with a warning once.
- `IFont`: `IsValid`, `MeasureText( text, size )`, `DrawText( text, x, y, size, color )`,
  `DrawTextRotated(...)`. These are screen space and honour the clip and blend modes (I2).
- Ownership is shared: the game's handle, the engine while the font is active, and each registry
  slot. The last owner releases it. `Stop()` lets go of every registered font and selects the
  built-in font again while the window is open. When the window closes, every live font becomes
  invalid (calls do nothing, nothing dangles).
- References in sunlight: `doc/BREAKAGES.md` entry 52, `plans/MASTER_PLAN.md` "Integration
  follow-ups" I3, `src/drawsurface/idrawsurface.h`, `src/font/ifont.h`, `samples/text` (each face
  loaded once and registered; the Mono face also selected for a HUD line),
  `tests/test_textobjects.cpp`, `tests/test_objectrenderers.cpp`.

*Why the re-pin and the Lua API are one item.* Scarab's `set_font` calls
`IDrawSurface::SetFont( szFilePath )` (`src/lua/luatextapi.cpp`), which no longer exists, so the
re-pin alone does not compile.

*Decisions (the owner, 2026-10-24):*
1. **`set_font` takes a path, a handle, or `nil`**, told apart by the argument's Lua type. A
   string loads and selects in one step, like today; that font is owned only by the engine, so it
   is freed when another font replaces it. A handle (from `font_load`) selects that font. `nil`
   selects the built-in font. Errors are `nil, "message"` (B1/D3), where today's `set_font`
   returns `false`.
2. **A font in Lua is an opaque integer handle with an explicit release**, like `sprite_*`/`view_*`.
   A Scarab-side pool holds the `shared_ptr<IFont>` per handle. Releasing a handle drops only the
   pool's reference, so a font that is still active or registered keeps working.
3. **`draw_text` and `measure_text` get an optional last font argument**. Left out, they use the
   active font, exactly as today. Given, they draw or measure with that font through `IFont`
   directly.
4. **One item**, done right before V5.

*Planned Lua API (names to confirm when the item starts):*
```lua
local sans, err = font_load( APP_DIR .. "resources/fonts/caravellius8x8.fnt" )  --> handle | nil, message
set_font( sans )                      -- draw_text/measure_text use it
set_font( APP_DIR .. "x.ttf" )        -- or load + select in one step, as before
set_font( nil )                       -- the built-in font
register_font( "Sans", sans )         -- map text objects of family "Sans" use the SAME loaded font
register_font( "Sans", sans, true )   -- it may also stand in for "Sans bold"
unregister_font( sans )               -- every slot it serves; or unregister_font( "Sans", false, false )
draw_text( "Hi", 20, 20, 24, 255, 255, 255, 255, sans )   -- optional last argument
local w = measure_text( "Hi", 24, sans )
font_release( sans )                  -- the handle is no longer needed
```

*C++ (to detail when the item starts):* a Lua-agnostic font pool in `src/engine/`, like
`SpritePool`, holding `shared_ptr<IFont>` per handle, with generation-checked handles so a released
handle is never reused by mistake. `RegisterFont`/`UnregisterFont` reach the concrete renderer
through narrow methods on `IRendererProvider`. `LoadFont` needs a started renderer: a window-needing
call under the `renderer_create`-first rule. Every font is released before the window closes (the
same shutdown-order rule as `SpritePool::Clear` in `EngineHost::OnStop`).

*Tests:* headless smoke scenarios (sunlight's null backend loads a font iff the file exists):
load/select/register/unregister/release round trips, every error path, a released handle being
rejected, a handle still drawing after release while active or registered, and `set_font`'s three
forms, each proven by mutation. Re-pin checks as in P4/P5 (real fetch, `--version`, every smoke
test). `samples/text` and `samples/hello-world`/`samples/renderer` move to the new API where it
helps, and the owner checks text by eye. V5's `samples/objects` is the eye check for text objects.

*Breakages:* a `plans/CARAVELLIUS_BREAKAGES.md` entry. Caravellius' `Text.set_font`
(`src/core/text.lua`, called from `src/main.lua` with `caravellius8x8_bold.fnt`) keeps working
through the path form, but now gets `nil, "message"` instead of `false` on a bad path. The handle
form lets it load each font once. Per the owner (2026-10-24): try not to break Caravellius, but
prefer the best design even if it does, and log every break for the integration conversation.

*Fonts available* (the owner's point: our own, or open-license ones compatible with ours):

| Family | File | In Scarab's `resources/fonts/`? | License |
|---|---|---|---|
| "Sans" | `caravellius8x8.fnt` (+ `_bold`) | yes | the project's own (zlib; sunlight also lists SIL OFL 1.1) |
| "Pixel" | `pressstart2p-regular.ttf` | yes | SIL OFL 1.1 |
| "Mono" | `AnonymousPro-Regular/Bold/Italic/BoldItalic.ttf` | no | SIL OFL 1.1 (compatible; copy with `OFL-anonymouspro.txt`) |

Every map V5 (and V6/V7) would use (`objects.tmx`, `thick.tmx`, `rotation.tmx`, `opacity.tmx`)
uses only "Sans" regular, which is already in Scarab. Anonymous Pro is needed only for sunlight's
`text.tmx` ("Mono").

**Built (2026-10-24):**
- **Names confirmed by the owner:** `load_font`, `release_font`, `set_font`, `register_font`,
  `unregister_font`, `draw_text`, `measure_text`. The owner noted that a category prefix (like
  `sprite_*`) fits sprites, but not fonts, because `set_font` already exists. `get_font` is left
  out for now: a font selected by path has no Lua handle, and nothing needs to read the active
  font back.
- **Re-pin:** `SCARAB_SUNLIGHT_REF` `9c8c6bf421a1147d1e8c099e698dfbe7c1885af2`,
  `SCARAB_SUNLIGHT_DISPLAY_VERSION` `feat/master_plan@9c8c6bf`. Real fetch verified. Against
  the new pin, exactly one compile error, as expected: `set_font` passing a path to the old
  `SetFont`.
- `src/engine/fontpool.{h,cpp}`: `Scarab::Engine::FontPool`, Lua-agnostic like `SpritePool`.
  Each slot holds one `shared_ptr<IFont>`, and `FontHandle`s use the same generation packing as
  `SpriteHandle`, so a released handle never resolves to the font that reuses its slot.
  `EngineHost` owns it and clears it in `OnStop()`, before the window closes, like
  `SpritePool`. `LuaEngine::Init` stashes it as `fontPoolPtr`, read by
  `LuaEngineUtil::GetFontPool`.
- `IRendererProvider` gained `RegisterFont( family, font, bold, italic, trigger )` and the two
  `UnregisterFont` forms, because sunlight has them only on the concrete `TileMapRenderer`.
  `RegisterFont` creates the default renderer first, like `GetTileMap`. The unregister forms
  create none and report false without one. `LoadFont`/`SetFont` come from `IDrawSurface`.
- `src/lua/luatextapi.cpp`: the bindings. `set_font` tells a string, an integer handle and
  `nil` apart. `draw_text`/`measure_text` draw or measure through the given `IFont` directly.
  Every error is `nil, "message"`.
- **Found while building it:** the first probe showed `register_font( "Sans", f )` failing with
  "'bold' must be a boolean". `LuaEngineUtil::GetFontPool` left the pool pointer pushed on the
  Lua stack, so an omitted 3rd argument read as that pointer. `GetFontPool` now pops it, like
  `GetRendererProvider`. `GetSoundManager`, `GetSpritePool` and `GetScriptProcessor` have the
  same leak but are harmless today, because their callers read every argument first. **Record
  for D3**: make them pop too.
- **Test:** `scripts/font_api_smoke.py` + `scripts/font_api_smoke/`, in `ci.yml` on all 4
  platforms. It checks `load_font` handles and errors, `set_font`'s three forms and every error,
  `draw_text`/`measure_text`'s font argument, `register_font`/`unregister_font` by slot and by
  handle with every error, that releasing a handle keeps a font that is active or registered
  working, slot reuse with a stale handle rejected, and drawing with a handle inside a frame.
  **Mutation checks:** without the generation bump, it fails with "a reused slot gave back the
  released handle" and "stale handle after reuse". Without the stack pop, every
  `register_font` with an omitted flag fails. (Restoring needed the object files deleted, not
  just `touch`ed, before make rebuilt them; worth remembering for every mutation check.)
- `samples/text`: moved to the new API. It loads three fonts once (caravellius8x8 regular and
  bold, Press Start 2P), draws a line in each at the same time through `draw_text`'s font
  argument, keys `1`-`3`/`0` select the active font by handle or the built-in one, it shows a
  Portuguese/Spanish accented line in the TTF, and it shows `set_font`'s error for a bad path.
  README updated.
- Docs: the Text page has "Fonts" and "Fonts for the map's text objects" sections, and the
  `docs/lua-api/README.md` index row was updated. `plans/CARAVELLIUS_BREAKAGES.md` entry 6.
- **Two review changes (the owner, 2026-10-24):**
  - `screen_get_width`/`screen_get_height` moved from `LuaTextApi` to `LuaAppApi`: they return
    the fixed render-area size every screen-space call uses, and have nothing to do with
    text. Their C++ names changed from `GetWindowWidth`/`GetWindowHeight`, which misleadingly
    said "Window" for something that is deliberately not the window size, to `GetScreenWidth`/
    `GetScreenHeight`, matching their Lua names. **The Lua names are unchanged**, since 8
    sample uses and 9 Caravellius files call them. Their docs moved from the Text page to the
    App page. `LuaTextApi`'s outdated class comment ("a thin wrapper over `ITileMap::SetFont`")
    now describes the font API.
  - The forward declarations in `luaengine.h` were removed (see Phase S, "Forward
    declarations: sunlight's rule").
  - The owner noticed `LuaEngineUtil` mixes shared and category-specific helpers. This became
    a new item, **S4**. `GetFontPool`, brand new in C6, moved right away (the owner's call): it
    is now a file-local function in `luatextapi.cpp`, the only file that uses it, and pops what
    it pushes. Its global's name is defined once, as the public
    `LuaTextApi::FONT_POOL_GLOBAL`, which `LuaEngine::Init` uses to set it. That is the pattern
    S4 follows for the others.
- Checks: every other smoke test passes, as do the docs generator and all 16 samples headless.

**C7 (TODO, added by the owner 2026-10-29 during V10; plan it together with sunlight when it
starts, not before).** Today a renderer holds ONE map (`TileMapRenderer::m_pMap`), views are cameras
on that one map (`IView` has no map of its own; sunlight's `multiview` samples show one map in
three views), and Scarab allows one renderer per process. Many games show different maps at once:
split-screen with players in different areas, a world map beside the local one, a picture-in-picture
dungeon, a menu over a different map, a cross-fade between two maps.
- **sunlight's side (the map lives there, so it comes first):** several maps loaded at once, each with
  its own textures and tile animations, and a view showing one of them; layer ids collide between
  maps, so sprites, collision rules and layer masks must say which map they belong to; collisions
  per map (sprite-vs-tile against that map's tiles); camera alignment, scroll limits and `FitToMap`
  per map.
- **Scarab's side, a sketch to agree with sunlight:** `tilemap_load_map` returning a map handle, a
  `view_set_map(view, map)`, and the `tilemap_*` / `sprite_add_to_layer` / collision functions taking
  an optional map; the single-map calls keep acting on the current map, so existing games are
  unchanged.
- When C7 starts: contact sunlight (with the owner) to plan the design together.

## Phase R: requisites

| # | Item | Status |
|---|------|--------|
| R1 | AddressSanitizer CI job (Linux) | DONE |
| R2 | First doctest suite | DONE |

**R1 (DONE, 2026-10-30).** Mirrors sunlight's own "AddressSanitizer (Linux)" job - Linux only, for CI time;
also runs LeakSanitizer, which macOS's AddressSanitizer does not.
- **Job:** `ci.yml` "AddressSanitizer (Linux)" - Scarab and every fetched dependency (sunlight, raylib,
  libtmx, libxml2, Lua) built Debug with `-fsanitize=address -fno-omit-frame-pointer -g`, with the CI content
  key, so the encrypted archive test runs too. Scarab has no unit tests yet (R2), so the job runs the
  executable: `scripts/asan_run.py` runs the headless smoke test (and its budget run), every
  `scripts/*_smoke.py` and every sample headless for 120 frames, with
  `detect_leaks=1:halt_on_error=1:abort_on_error=1`. Every report goes to a log directory (`log_path`), so
  a report fails the run even where a smoke driver would only print its output's tail. New smoke tests and
  samples are picked up automatically.
- **Leaks:** `scripts/lsan.supp` is for leaks PROVEN to be inside a third-party library, one entry each with
  its reason; it is empty - the first CI run found no leak anywhere (35 of 35 runs).
- **One test adjusted:** `load_map_smoke`'s peak-memory comparison (V10) grew 365 MB under ASan: its
  quarantine keeps freed memory, so peak memory grows whatever the code does. The driver skips only that
  comparison under a sanitizer (`SCARAB_SANITIZER`, set by the wrapper); LeakSanitizer covers leaks there
  more strictly, and the normal builds still compare (0.0 MB).
- **Local (macOS, no LeakSanitizer):** the same build in `build-asan/` (now ignored by git): 35 of 35 runs
  clean (`--no-leaks`); Caravellius' acceptance PASS and fingerprint `hash=33897632` under ASan, no report;
  `leaks --atExit` on the normal build: 0 leaks for the headless smoke test and samples sprite, shapes and
  mapless.
- **Proven:** a deliberate heap use-after-free in `sprite_get_opacity` was caught locally by the wrapper (in
  `sprite_tint_smoke` and `samples/sprite`, naming `luaspriteapi.cpp:735`), then restored; a deliberate leak
  (a `new` with no `delete` in the same function) was pushed as a throwaway commit to the PR branch: the CI
  job failed with LeakSanitizer reports naming `luaspriteapi.cpp:734` (`sprite_tint_smoke`, 72 bytes in 18
  allocations; `samples/sprite`, 476 bytes in 119; 33 of 35 passed), every other job stayed green, and the
  commit was reverted.
- **Docs:** `CLAUDE.md`'s CI section. CI only - no Caravellius breakage entry.

**R2 (DONE, 2026-10-31).** Scoped with the owner (2026-10-30): all five window-free units below, the option off
by default like sunlight's, and `EngineHost::ResolveEntryScript` left for later (it needs `EngineHost` and the
filesystem, and the headless and archive smoke tests already cover it).
- **Build split:** everything in `src/` but `main.cpp` is the static library `scarab_core`, linked by `scarab`
  (`main.cpp`) and by `scarab_tests` (`tests/`, doctest `v2.4.11` - sunlight's pin), built only with
  `-DSCARAB_BUILD_TESTS=ON` (OFF by default). `scarab_tests` lands next to `scarab` with the same relocatable
  rpath and depends on `scarab_copy_binaries`. The compile definitions (version, content key) are `scarab_core`'s,
  PUBLIC. It is not registered with `ctest`: the fetched Lua project registers its own `lua-testsuite`, which
  failed there.
- **The split changes nothing:** `--version`, a relocated copy of the build output (as release.yml's smoke step),
  all 14 smoke tests, the headless smoke test, all 19 samples, and Caravellius' acceptance PASS and fingerprint
  `hash=33897632` are all as before; a build with tests OFF builds no test target.
- **Suites (25 test cases, 711 assertions):** `test_spritepool` (handle packing and the pointer tag, per-type
  capacity and `MAX_SPRITE_SLOTS`, acquire/exhaust, a released or stale handle never resolving even after its slot
  is reused, foreign handles refused, a recycled slot reset to hidden/view space/no blend mode/opaque/untinted,
  `Clear`); `test_fontpool` (a stand-in `IFont`; release drops only the pool's reference, stale handles, `Clear`
  freeing the last reference); `test_luaengineutil` (every strict reader's accept/refuse cases and exact
  messages on a bare `lua_State`, `PushError`/`PushTrue`, `FormatNumber`, `ShortestDecimal` round trips);
  `test_zoomfactor` (all 256 positions round-trip, 3.8 refused naming 3.8125, range and NaN/infinity);
  `test_crypto` (with the content key: `crypto_encrypt_data` output decrypts exactly through `TryDecryptBytes`,
  tampered or cut data and plain data refused, a fresh nonce each time; without a key: nothing encrypts).
- **Proven:** one mutation per suite, each failing its own suite and restored - no generation bump on release,
  a release that keeps the font, `3.5` accepted as an integer, the zoom position off by one, a failed MAC check
  accepted. Also clean under the local ASan build.
- **CI:** every platform configures with `-DSCARAB_BUILD_TESTS=ON` and runs `scarab_tests` after the build; the
  AddressSanitizer (Linux) job runs it under ASan/LeakSanitizer too.
- **Docs:** `CLAUDE.md` (Build: the split, how to build and run the tests, and the rule "new pure logic gets a
  doctest in the same change"; CI). No Caravellius breakage entry (build and tests only).

## Phase S: code style

| # | Item | Status |
|---|------|--------|
| S1 | Audit against sunlight's `doc/CODE_STYLE.md`, with real measured counts; Scarab's own guide `docs/CODE_STYLE.md` (C++ copied from sunlight, plus Lua rules) | DONE |
| S2 | Mechanical alignment pass, object-file-identical proof | TODO |
| S3 | `CLAUDE.md` rule change: "match existing style, don't fix it" becomes "the guide is the reference now" | TODO |
| S4 | `LuaEngineUtil` keeps only helpers several Lua modules share; category-specific ones move to their own modules | TODO |

**S1 (DONE, 2026-10-31).** The owner widened it (2026-10-31): Lua gets rules too, so Scarab now has its
own guide, **`docs/CODE_STYLE.md`**. Its C++ part is sunlight's `doc/CODE_STYLE.md` (sunlight `ef1499d`)
copied word for word, with headings one level down and sunlight's Design section replaced by Scarab's.
Its Lua part is Scarab's own, since sunlight has no Lua. The Lua part takes in Caravellius' own rules
(its `CLAUDE.md`, "Code style", `22bbaaf`: the `__` module-constant prefix it has used since 2026-08-11,
the cases that are not constants, its `tools/lua_lint.py`). The owner's decisions:
- **Private constants are `__NAME`, public ones `NAME`, in both languages** (the owner's long-standing
  habit, already Caravellius' rule). In Lua a private constant is `local __NAME <const> = value`;
  `<const>` makes Lua itself refuse a reassignment (checked on Scarab's Lua 5.4.7:
  `attempt to assign to const variable`).
- **Lua calls and definitions take spaces inside the parentheses, `f( x )`, as in C++** - Caravellius
  mostly writes `f(x)` (1631 against 297) and changes at integration.
- **The guide lives in `docs/`** (reference material that stays); `doc/` was renamed `plans/` (#66) for
  the master plan, the breakage log and research.

**The audit tool: `scripts/style_audit.py`.** C++ rules over `src/` and `tests/`, Lua rules over
`samples/`, `scripts/` and `tools/`. Like Caravellius' lint, it blanks out comments and strings before
matching, so a rule never fires on text in a comment or a message; `--list RULE` prints every finding.
**Calibrated on sunlight's own `src/`**, the code the C++ guide describes: every rule was refined until
none of its findings there is a false one (a leading `::PHYSFS_init`, a call split after `(`, a braced
initializer, a value-less `#define WIN32_LEAN_AND_MEAN`, a `@param` continued on the next line, a
keyword before `::`, an aligned `Type  *pName`, the `s_` prefix of a static, an array's `anRect`). What it
still finds in sunlight is real: 31 Hungarian mismatches, 72 `Type *pName`, 28 `@param`s without `;`,
15 `case X:`, 13 bodies without a leading blank line, 5 include guards, 15 raw `new`/`delete`, 2
`std ::make_pair`, `nCount++)` and `push_back(fZoomStep+=` - sunlight's code is not fully aligned
with its own guide either.

**Scarab, measured (2026-10-31, `master_plan` at `69143e1`):**

| Rule | Count | Files | What it is | S2 |
|---|---|---|---|---|
| `Type *pName` (`*`/`&` with the name) | 529 | 47 | 444 of them `lua_State *pLuaState` | mechanical |
| `@param` without `;` | 40 | 9 | | mechanical (comments) |
| `extern "C"` then `{` on its own line | 30 | 30 | every Lua header include block | mechanical |
| include guard's `#endif  /* */` (two spaces) | 25 | 25 | every Scarab header | mechanical |
| `->` unspaced | 14 | 2 | `it->second` (luapackapi, luatimerapi) | mechanical |
| brace without two spaces before it | 12 | 5 | `)  : m_X( ... ) {`, `) {` | mechanical |
| unspaced `::`, `( x)`, `switch (`, `default:`, other `{` alone, body without a blank line, own header not first (`main.cpp`) | 10 | 6 | | mechanical |
| names whose prefix doesn't fit their type | 24 | 9 | `SpriteHandle handle` (guide: `hName`), `m_CurrentSong` (`uint16_t`), `int bIsInteger` (x3); 13 in the new tests | rename |
| `override` missing on an interface method | 19 | 3 | `EngineHost` 4, `RendererProvider` 13, `LuaCollisionListener` 2 | code |
| `using namespace` | 5 | 5 | `enginehost.cpp` (`std :: chrono`), `luainputapi`, `luaspriteapi`, `luarendererapi`, `luatilemapapi` (5 more in tests, allowed) | code |
| file-local constant not a `#define __NAME` | 4 | 4 | `NO_CONTENT_KEY`, `NO_AUDIO_SOUND_ID`, `NO_MAP_LOADED`, a test's `PLAIN_TEXT` | code |
| raw `new`/`delete` | 8 | 3 | the collision listener and timers (`LuaEngine`, `LuaTimerApi`), the zip writer (`LuaPackApi`) | code (`unique_ptr`) |
| numbers outside a named constant | 81 lines | 15 | mostly Lua argument positions (`lua_tointeger( L, 3 )`), bit masks in `spritehandle.h`, `char szMsg[50]` | read each |
| declarations not in one column | 45 of 141 blocks | | | mechanical |
| Lua: private constant without `__` | 176 | 23 | `local GLOW_LAYER = 4` | rename |
| Lua: `__NAME` without `<const>` | 18 | 7 | | mechanical |
| Lua: magic numbers in samples | 135 lines | 19 | HUD layout and colours (`draw_text( t, 20, 50, 20, 200, ... )`) | code |
| Lua: no license header | 12 | 12 | every smoke test's `main.lua` | mechanical |
| Lua: function not `snake_case` / local not `lowerCamel` | 3 / 6 | 4 | `startPos`, `onSecondTick`, `platform_name`, `os_time_start`, ... | rename |
| Lua: numbers in smoke tests | 261 lines | 11 | expected values - allowed | none |

Zero today: C++ license headers, tabs, one-line namespaces, forward declarations, `()` for `( void )`,
`} else`; Lua tabs, parenthesis spacing, single quotes.

**S2's proof, tested:** a whitespace-only change must leave every object file byte for byte the same.
That needs debug info off (`-g0`), since `-g2` records line numbers: compiled from the flags in
`build/CMakeFiles/scarab_core.dir/flags.make` with `-g0`, `luatimerapi.cpp` gave identical objects
twice, identical again after three real spacing fixes (`Timer:: Interval`, `pTImer)`,
`(int nId ) {`), and a different one after a one-character code change - so the proof can fail. No
Scarab C++ uses `__LINE__`, `__FILE__` or `assert` (the 15 `assert` hits are Lua examples in doc
comments).

**What this means for S2** (to plan with the owner when it starts): the "mechanical" rows can be done
with the identical-object proof; the "rename" and "code" rows change object files, so they need the
usual proof (unit tests, every smoke test and sample, Caravellius' acceptance and fingerprint); the
Lua rows are proven by the smoke tests, the samples headless and a window check. That is three kinds of
proof, so S2 is probably three PRs.

**S4 (TODO, added 2026-10-24 by the owner during C6's review).** `src/lua/luaengineutil.{h,cpp}`
mixes helpers every Lua module shares with ones only one category uses. Each new engine object
got another "fetch the light-userdata global" getter there (C6's `GetFontPool` too), so
`luaengineutil.h` includes the sound, sprite-pool, font-pool, script-processor and timer headers,
and a change to any of them recompiles all 16 `src/lua/*api.cpp` files. Usage today (2026-10-24):

| Member | Used by | Where it belongs |
|---|---|---|
| `GetRendererProvider`, `GetTileMap`, `GetDrawSurface`, `EnsureRenderer` | renderer, view, app, text, tilemap, camera, input, sprite, collision, blend | stays (shared renderer access) |
| `RegisterConstants` + `stNamedConstant`, `RegisterRealConstants` + `stNamedRealConstant` | input, tilemap, sprite, renderer, app, blend (the real-valued one renderer only, but generic in kind) | stays |
| `PushError` / `PushTrue` | sprite, text, blend (every module after D3) | stays |
| `s_LuaMutex` | `LuaEngine`, timer | stays |
| `GetSoundManager` | sound only | `LuaSoundApi` |
| `GetSpritePool` | sprite only | `LuaSpriteApi` |
| `GetFontPool` | text only | **done in C6**: a file-local function in `luatextapi.cpp`, global name `LuaTextApi::FONT_POOL_GLOBAL` |
| `GetScriptProcessor`, `AddOneParmCommandScript` | scripting, sound (queued song commands) | `LuaScriptingApi`; sound calls it there |
| `TimerMap` typedef | timer (and `LuaEngine`, which owns the timers) | `luatimerapi.h` |

Scope:
- A single shared `LuaEngineUtil::GetGlobalPointer( lua_State*, const char* name )` that always
  pops what it pushed. Today each getter repeats `lua_getglobal` + `lua_touserdata`, and
  `GetSoundManager`, `GetSpritePool` and `GetScriptProcessor` leave the value on the stack (the
  leak C6 hit through `GetFontPool`, which was fixed there). Each category getter then becomes
  one line in its own module, with its global's name defined once next to where `LuaEngine::Init`
  sets it.
- Move the category-specific members as in the table. `luaengineutil.h` then includes only
  what the shared helpers need.
- Behavior unchanged: a pure refactor, proven the way S2 plans (object-file-identical where the
  code didn't move, otherwise every smoke test and sample unchanged). It is deliberately kept out
  of D3, so every PR is either a refactor or a behavior change, never both.

**Rules the owner has already decided (S1 records them, S3 writes them into `CLAUDE.md`):**
- **A class from sunlight (or any other external library) is never forward-declared: include
  its header** (decided 2026-10-22). The header is then the only place that says where the
  class lives, so a namespace move on sunlight's side (like P2's `TileMap` to `Base`) can't leave
  a stale declaration in Scarab that silently declares a phantom class. After a move, the
  build fails cleanly at each line that still names the old path, or not at all. Applied to the
  only two headers that had any: `src/engine/irendererprovider.h` (`ITileMap`,
  `IDrawSurface`) and `src/lua/luaengine.h` (`ITileMap`, `SoundManager`).
- **Forward declarations: sunlight's rule** (decided by the owner in the sunlight session,
  2026-10-24, and drafted for sunlight's `doc/CODE_STYLE.md` on its branch
  `doc/code-style-forward-declarations`, not on `feat/master_plan` yet). This **replaces** the
  one-line C++17 form Scarab adopted on 2026-10-22, which sunlight's guide and code never use.
  - The form is nested and indented, one level each, like every namespace. A class in the
    header's own namespace is declared inside that namespace's existing block. A class from
    another namespace gets its own nested block (sunlight's `NullBackend` in
    `tilemaprenderer.h`).
  - When: forward-declare only the project's own classes, only where the header needs just the
    name (a pointer, a reference, a smart pointer, a friend), and only to break an include cycle
    (sunlight's `View` <-> `TileMapRenderer`) or to keep a backend out of a public header
    (`NullBackend`). Everything else is included, and so is every external library's class.
  - Applied in C6: `src/lua/luaengine.h`'s three forward declarations were all unnecessary
    (`SpritePool`/`FontPool` are already included through `luaengineutil.h`;
    `LuaCollisionListener` has no cycle and is no backend), so they were removed and
    `lua/luacollisionlistener.h` is included. Scarab now has no forward declarations at all.
- **Also for S1's audit:** sunlight's guide forbids `using namespace` in a project's own code,
  and Scarab has 5 (`luainputapi.cpp`, `luaspriteapi.cpp`, `luarendererapi.cpp`,
  `luatilemapapi.cpp`, `enginehost.cpp`).

## Phase B: blend modes

| # | Item | Status |
|---|------|--------|
| B1 | Lua bindings, tests and a sample for sunlight's blend modes (I2, sunlight BREAKAGES entry 51) | DONE |

**B1 (DONE, 2026-10-23 - samples tested in a real window and code reviewed and approved by the owner; added 2026-10-22 at the owner's request, before Phase T so its docs are part of
that review; done right after D2, out of phase order, by the owner's decision).** Exposes sunlight's I2 (see P5) to Lua. Names are suggestions, decided when the
item starts:
- A scoped mode around a script's own drawing, e.g. `draw_begin_blend(mode)` /
  `draw_end_blend()`, with the modes as constants (e.g. `BLEND_NORMAL`, `BLEND_ADD`,
  `BLEND_MULTIPLY`, `BLEND_SUBTRACT`) tagged `@luaconstants`.
- A sprite's own mode, e.g. `sprite_set_blend_mode(sprite, mode)`,
  `sprite_clear_blend_mode(sprite)`, and a getter.
- Tests now, as the owner decided, not waiting for R2's doctest suite: a headless Lua smoke
  scenario covering argument checks, unknown mode values, round trips through the getters, and
  that a recycled sprite slot does not inherit its previous owner's mode (`SpritePool`, like
  `sprite_set_world_space`; sunlight's `Unload()` keeps the mode). A headless run cannot see
  colour, so what each mode looks like is checked by eye.
- A new `samples/blend`, modelled on sunlight's `samples/blend` (a glow sprite whose mode keys
  choose, a soft shadow that always multiplies, panels drawn per mode), for the owner's
  real-window check.
- The generated Lua API pages (the `@luaname`/`@luadoc` tags), and a mapping in
  `scripts/_lua_api_shared.py`'s `SOURCE_TO_DOC` if a new `src/lua/*api.cpp` file is added.
- Any file that includes `raylib.h` must write `SunLight :: Base :: BlendMode` in full.

Reference in sunlight: `plans/MASTER_PLAN.md` "Integration follow-ups" I2,
`samples/blend/docs/README.md`, `CHANGELOG.md` [Unreleased].

**Decided with the owner (2026-10-23):**
- Names in Scarab's own style, not sunlight's shorter suggestions: `draw_begin_blend_mode` /
  `draw_end_blend_mode`, `sprite_set_blend_mode` / `sprite_clear_blend_mode` /
  `sprite_get_blend_mode`, and the constants `BLEND_MODE_*`, which mirror sunlight's enum one to
  one like every other Scarab constant set.
- Every new function reports a failure as `nil, "message"` (like `renderer_*`/`view_*`), never
  silently. The owner also asked for item D3, which brings the older functions in line.
- The sample follows sunlight's: keys `1` (no mode of its own) and `2`-`5` (normal, add (the
  default), multiply, subtract), and sunlight's `glow.png`/`shadow.png` copied into
  `resources/sprites/`.

**Built (2026-10-23):**
- `src/lua/luablendapi.{h,cpp}` (new page "Blend modes", `blend.md` in `SOURCE_TO_DOC`, linked
  from `docs/lua-api/README.md`). It holds the four `BLEND_MODE_*` constants (`@luaconstants`),
  `draw_begin_blend_mode(mode)` / `draw_end_blend_mode()` (pass-throughs to
  `IDrawSurface::BeginBlendMode`/`EndBlendMode`), and `LuaBlendApi::ReadBlendMode`, the strict
  mode reader shared with the sprite functions. It is registered with the renderer-free modules,
  so the constants exist before any window. The draw functions reach the renderer only when
  called.
- `src/lua/luaspriteapi.cpp`: `sprite_set_blend_mode`, `sprite_clear_blend_mode` and
  `sprite_get_blend_mode` (the mode, or `nil` when the sprite has none of its own, which differs
  from `BLEND_MODE_NORMAL`), with a strict handle reader (`"invalid sprite handle"`).
- `SpritePool::Release` calls `ClearBlendMode()` next to `SetWorldSpace( false )`, so a recycled
  handle never inherits a mode (sunlight's `Unload()` keeps it on purpose).
- `LuaEngineUtil::PushError`/`PushTrue`: the shared `nil, "message"` / `true` helpers, for D3 to
  reuse. The renderer and view files keep their own copies until D3 (or S).
- Test: `scripts/blend_api_smoke.py` + `scripts/blend_api_smoke/`, in `ci.yml` on all 4
  platforms. It checks the constants' values, set/get/clear round trips for every mode, every
  error (unknown mode, negative, string, fraction, missing, bad, string and released handles),
  that a rejected call changes nothing, the recycled slot, that no sprite call creates the
  renderer, the scoped pair around real drawing for every mode, that a second begin replaces the
  first, an end with no begin, and a begin with no end over the next 5 frames. Mutation check:
  without the `ClearBlendMode()` line in `Release`, the test fails with "a recycled sprite
  inherited blend mode 1", and it passes again once the line is restored. (A first rerun after
  restoring still failed: the restored file had the same mtime second as the mutated build, so
  make skipped it. A `touch` fixed it. Worth knowing for every mutation check here.)
- `samples/blend` (+ `docs/README.md`, listed in `samples/README.md`), modelled on sunlight's:
  Sunny with a glow under it (keys `1`-`5`), a shadow that always multiplies, and four panels
  (red/green/blue squares, alpha 200) drawn with `draw_filled_rectangle` between begin and end,
  each over its own backdrop (sky; night for add). It uses `renderer_create{ zoom = 1.0,
  preferred_zoom = 1.0 }` so the map and the 128x128 glow fit next to the panels.
- `resources/sprites/glow.png` and `shadow.png` come from sunlight's `samples/blend`
  (`bfce598`), unmodified, with provenance in `resources/sprites/README.md`.
- `docs/vscode/.vscode/launch.json` (the tracked VSCode template; the root `.vscode/` is
  git-ignored): the "Launch sample" picker had fallen behind `samples/`. It now lists all 16,
  adding `blend` plus the `renderer`, `views` and `crypto` samples that earlier work never
  added. The owner's local copy was updated the same way.
- No CARAVELLIUS_BREAKAGES entry: nothing changes for a game that uses no mode.

## Phase T: documentation review

Done last, the same reasoning sunlight's own Phase T used: a documentation pass is only
accurate once the features it describes are actually settled, not before.

| # | Item | Status |
|---|------|--------|
| T1 | A full documentation pass once every other phase's changes have landed | TODO |
| T2 | Simplify `README.md` | TODO |
| T3 | Every source file and script carries the project's copyright/license notice | TODO |

**T1.** Scope: `README.md`; the generated Lua API reference's own source comments
(`@luadoc`/`@luaexample` tags in `src/lua/`); `samples/*/docs/README.md`; `CLAUDE.md`'s own
"Known gotchas" (several will need a new entry or an update from this plan's own
findings - the `use_default_key_handler` design (D1), the non-integer-zoom pixel-snap
fixes (V2), `MoveCamera*`'s new boundary behavior (V1)); `doc/lua-integration.md`.

**T2.** The owner's own assessment (2026-10-20): Scarab's `README.md` reads as "too much
polluted." Measured, not assumed: it is 197 lines with 17 section headings, against
sunlight's own `README.md` at 193 lines with 22 headings - comparable in raw size, so
whatever makes it feel polluted is density or structure, not length alone. sunlight's
`README.md` can be used as a base. Scope, left open: what moves out (to `CLAUDE.md`, to a
dedicated `docs/` file, or is dropped), what stays, and the target structure - discussed in
full when this item starts, not decided here.

**T3 (TODO, added 2026-10-23 at the owner's request).** The owner noticed that some scripts,
and maybe some source code, don't carry the project's copyright/license notice
(`docs/HEADER.txt`, zlib). `CLAUDE.md` already requires it for every `.cpp`/`.h` under `src/`
("a copyright/licensing requirement, not a style nicety"), but says nothing about other
files. A first count over the tracked files (2026-10-23, grepping for the notice's first line):
- `.cpp`/`.h`: all 50 have it. The root `CMakeLists.txt` has it.
- `.lua`: 5 of 22 don't, all under `scripts/` (the smoke-test projects: `headless_smoke`,
  `renderer_api_smoke`, `camera_boundary_smoke`, `blend_api_smoke`, `tilemap_layer_smoke`).
  Every `samples/` script has it.
- `.py`: 5 of 7 don't (`renderer_api_smoke.py`, `tileset_archive_smoke.py`,
  `camera_boundary_smoke.py`, `blend_api_smoke.py`, `tilemap_layer_smoke.py`). The two docs
  scripts have it.
- `.github/workflows/*.yml`: none of the 3 have it.
- V4's new `input_constants_smoke.py` and `main.lua` got it when they were written, so the
  list did not grow.

Scope, to settle when the item starts:
- Which file kinds need it. Code and scripts surely. Probably workflows and other config
  (`Doxyfile`, `CMakePresets.json` - JSON has no comments). Docs (`.md`) and assets probably
  not; `resources/` keeps its own per-folder license READMEs.
- The exact comment style per language (C++ `/* */`, Lua `--[[ ]]`, Python/YAML/CMake `#`),
  matching what already exists.
- Whether to add a CI check, like the `lua-api-docs` hard fail, so a new file without the
  notice fails CI instead of being found by eye again.
- Write the rule into `CLAUDE.md` for every file kind it covers, not only `src/`.

## Process

- **Peer impact for Caravellius**: [plans/CARAVELLIUS_BREAKAGES.md](CARAVELLIUS_BREAKAGES.md),
  the same strategy sunlight used for `doc/BREAKAGES.md` - an entry is added the moment an
  item that affects Caravellius merges, not collected at the end. It lives on `master_plan`
  only and is removed once this plan merges to `main`, after Caravellius has been told.
- A large phase may get its own staging branch (`feat/complete_<phase>_plan`, created from
  `master_plan`); its items' PRs go there first, and one final PR closes the phase into
  `master_plan` - the same pattern sunlight used for its own Phases E, F, L, M, S, T. A
  staging branch is added to `.github/workflows/ci.yml`'s branch list while its phase runs,
  and dropped right after the phase merges.
- Items are closed one at a time, in phase order, unless the owner decides otherwise.
- Each item is closed only when its test items pass and the owner has confirmed it.
- Sub-tasks for an item are detailed when that item starts, not all upfront.
- **`master_plan` merges to `main`, and this plan's own items here close, only on the
  owner's own explicit command** - after Scarab's own work in this plan is integrated with
  Caravellius, and after Caravellius has been told what changed. Not automatically once
  every item above shows `DONE`, and not before the owner gives that command. No Scarab
  version is tagged before then either (per the handoff brief's own §1 - sunlight, Scarab
  and Caravellius all merge to their own `main` together, not one at a time as each
  finishes). `--version` reports the branch and a short commit hash for the whole plan (see
  P1).
- This document and `plans/CARAVELLIUS_BREAKAGES.md` are removed from the tree in the same PR
  that merges `master_plan` into `main`, after Caravellius has been told.
