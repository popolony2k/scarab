# Scarab master plan

This is the working plan for integrating sunlight's `feat/master_plan`. Each item has a
status, a scope, and the test items that must pass before it is closed. Items are closed
one at a time, in phase order, unless the owner decides otherwise.

Like [`doc/CARAVELLIUS_BREAKAGES.md`](CARAVELLIUS_BREAKAGES.md), this file is integration
material only: it lives on `master_plan` and is removed before `master_plan` goes to
`main`. `master_plan` merges to `main` only on the owner's own explicit command, after
Scarab's own work here is integrated with Caravellius - not automatically once this plan's
own items are closed, and not before Caravellius has been told what changed.

Every change follows this project's own rules in [`CLAUDE.md`](../CLAUDE.md): tests first
(a regression test must fail on the old code, where applicable), the full existing check
suite passing (samples, the renderer/view API smoke test, the tileset archive smoke test,
the headless smoke test), an AddressSanitizer run for anything touching lifetimes once
Phase R lands, and a PR per item (or per consolidated set of mechanical items).

This plan follows sunlight's own [`doc/MASTER_PLAN.md`](../../sunlight/doc/MASTER_PLAN.md)
in shape and in working rules - see sunlight's handoff, read from
[`doc/SCARAB_HANDOFF.md`](../../sunlight/doc/SCARAB_HANDOFF.md) in a local checkout of
sunlight (`../sunlight` from this repo's root), and its peer-impact list,
[`doc/BREAKAGES.md`](../../sunlight/doc/BREAKAGES.md) (49 entries). sunlight's own commit
pinned for this work: `772851ce7120c0d4f25dbf72c8d15f7e54bcd1d5` (the head of
`feat/master_plan` when the handoff was sent), re-pinned in P4 to
`a63fec6811ba28fb46a94015d05b852393016692` (sunlight's fix for semi-transparent drawing), then in
P5 to `bfce5983370fa0661c18701164a10f7e18f1e7a1` (sunlight's blend modes).

## Phase P: pin & plumbing

| # | Item | Status |
|---|------|--------|
| P0 | Write and get this document approved | DONE |
| P1 | Split `SCARAB_SUNLIGHT_VERSION`'s single hardcoded-`v`-prefix mechanism into a fetch-ref variable and a display-version variable; decide `--version`'s format for a commit-pinned build | DONE |
| P2 | Pin `GIT_TAG` to `772851ce7120c0d4f25dbf72c8d15f7e54bcd1d5`; make the code compile against it (the `SunLight::Base` namespace move, BREAKAGES entry 14; `BaseCommand`'s new header, entry 5) | DONE |
| P3 | Every existing Scarab check green against the pin, before any further BREAKAGES item is touched | DONE |
| P4 | Re-pin to `a63fec6811ba28fb46a94015d05b852393016692` (sunlight's I1 fix: semi-transparent drawing no longer darker; sunlight BREAKAGES entry 50) | DONE |
| P5 | Re-pin to `bfce5983370fa0661c18701164a10f7e18f1e7a1` (sunlight's I2: blend modes; sunlight BREAKAGES entry 51) | DONE |

**P0 (DONE).** This document and [`doc/CARAVELLIUS_BREAKAGES.md`](CARAVELLIUS_BREAKAGES.md)
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
- `doc/CARAVELLIUS_BREAKAGES.md` entry 2.
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

## Phase D: design items

| # | Item | Status |
|---|------|--------|
| D1 | `use_default_key_handler` (BREAKAGES entry 26) - keep the Lua shape, back it with `DefaultControllerBindings::Install` | DONE |
| D2 | `renderer_create{ preferred_zoom }` - a new, optional option for the zoom `zoom_reset` returns to (Scarab only, no sunlight change) | DONE |
| D3 | Consistent error reporting: every Lua function that can fail returns `nil, "message"` instead of silently ignoring bad input | TODO |

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

**D3 (TODO, added 2026-10-23 at the owner's request during B1, done in plan order).**
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

## Phase V: visual/behavioral re-verification

| # | Item | Status |
|---|------|--------|
| V1 | `MoveCamera*` boundary behavior (BREAKAGES entries 24, 46) - 8 Lua wrappers (`camera_move_*`, `view_move_camera_*`) | DONE |
| V2 | Pixel-snap/scroll-boundary rendering fixes at non-integer zoom (entry 35) | DONE |
| V3 | `nOpacity` 0..255 round-trip (entry 41) | DONE |
| V4 | `KEY_MENU` value change, 82 to 5 (entry 2) | TODO |
| V5 | Tile/text/point objects now drawn where they were invisible before (entries 1, 8, 11, 17) | TODO |
| V6 | Rotated shape/tile/text objects (entries 18-20) | TODO |
| V7 | Shape/text object opacity now respects layer opacity (entry 36) | TODO |
| V8 | `AddSprite` before a map loads is now actually drawn, not silently invisible (entry 39) | TODO |
| V9 | `Sprite::Unload()` leaves no active sequence (entry 47) | TODO |
| V10 | `LoadMap` over a loaded map no longer leaks (entry 40) | TODO |
| V11 | The collision lookup no longer reads a wrong tile for a sprite past the map's far edge (entry 4, A6) | TODO |
| V12 | A tile object resized away from its own tile's size now takes its object layer's tint (entry 45) | TODO |

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
- **V1c** - the first entry in `doc/CARAVELLIUS_BREAKAGES.md`.
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
- **V2c:** `doc/CARAVELLIUS_BREAKAGES.md` entry 3.

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
- **V3d:** `doc/CARAVELLIUS_BREAKAGES.md` entry 4.

**V4.** `src/lua/luainputapi.cpp` exposes `KEY_MENU` to Lua *by value* - the value itself
(82 to 5) is what a script sees, not just an internal rename. Scope: check whether anything
stored the old number (a saved binding, a hard-coded 82).

**V5-V10.** Scope as stated in each row; each closes with a check against Scarab's actual
`resources/` and `samples/` maps and sprite-pool/collision code, confirmed either
unaffected or adjusted, not assumed from the BREAKAGES table alone.

**V11.** BREAKAGES' own note: "any game with a sprite in the last strip of a map, at any
camera position" could have hit this before the fix, and "Caravellius has not hit it so far
(the maintainer's observation, not a test)" - Scarab has made no observation of its own
either way. Scope: check `samples/collision` and any other Scarab sample/sprite setup that
can reach a map's last strip.

**V12.** Scope: check `resources/tilemap/test.tmx` and any other Scarab map for a resized
tile object on a tinted object layer - before this fix it drew untinted (white, with only
its opacity); if any Scarab asset relied on that, it now looks different.

## Phase O: confirmed not applicable, recorded anyway

| # | Change | BREAKAGES entry | Why |
|---|--------|------------------|-----|
| O1 | Non-orthogonal maps refused | 1 | All Scarab maps are orthogonal |
| O2 | `SetPixel` removed | 15 | Zero calls anywhere in Scarab (confirmed by grep across `src/`, `resources/`, `docs/`, `samples/`, `README.md`) |
| O3 | Polyline/polygon/rectangle/ellipse width-by-zoom | 16 | No polyline/shape object in any Scarab map |
| O4 | Tile animation first-frame-double-duration fix | 44 | No animated tiles in any Scarab map |
| O5 | Layer parallax placement correction | 34, 49 | No parallax in any Scarab map, sample or source (confirmed by grep) |
| O6 | Listener dispatch reentrancy now well-defined | 42 | Scarab registers its listeners once at startup, never from inside a dispatch |
| O7 | Isometric/staggered/hexagonal maps now load | 21-23 | All Scarab maps are orthogonal; new capability, not a requirement |
| O8 | TTF/OTF accented glyphs | 43 | Scarab's docs name a TTF only as an example; no live behavior to check |
| O9 | `IEngine` gains `LoadFont`/`BeginClip`/`EndClip` | 6 | Scarab doesn't implement `IEngine` |
| O10 | `SetWindowBackgroundColor` takes `stColor` | 10 | Nothing in Scarab calls it |
| O11 | `doc/TODO.txt`/`FIXME.txt`/`MISSING_FEATURES.md` removed from sunlight | 3 | No link to them anywhere in Scarab |
| O12 | `IFont` is a new interface in its own module | 9 | Only code that uses `IFont` directly; Scarab doesn't |
| O13 | `FileSystem::Mount` sets PhysFS up itself, so it can be the first call | 12 | Scarab already calls `Init(argv[0])` first |
| O14 | Sample fonts moved to `samples/shared/fonts` in sunlight | 13 | Samples only, no API change |

## Phase C: new capabilities, not required

| # | Item | BREAKAGES entry | Scope |
|---|------|------------------|-------|
| C1 | `GetControllerManager`/hot-plug/player slots/action map/haptics | 25, 27-33, 48 | Whether/when to expose any of this to Lua - a real product decision, not a breakage fix |
| C2 | `IShapeSurface` (shapes without a loaded map) | 38 | HUD/debug drawing - defer or take |
| C3 | Sprite z-order | 37 | Expose `sprite_set_z_order`? |
| C4 | Layer parallax (a code API, not just Tiled-declared) | 34, 49 | Defer until a map actually wants it |
| C5 | Non-orthogonal map support, surfaced to game authors | 21-23 | Tell them it's possible; no API change needed, `tilemap_load_map` already takes any map |
| C6 | `TileMapRenderer::RegisterFont(family, path, bold, italic)` | 7 | A new public method; text objects draw with the registered font - expose to Lua? |

## Phase R: requisites

| # | Item | Status |
|---|------|--------|
| R1 | AddressSanitizer CI job (Linux) | TODO |
| R2 | First doctest suite | TODO |

**R1.** Mirrors sunlight's own "AddressSanitizer (Linux)" job - Linux only, for CI time;
also runs LeakSanitizer, which macOS's AddressSanitizer does not.

**R2.** Needs its own scoping discussion: what's testable without a window. `SpritePool`
(`src/engine/`) is the obvious first candidate - deliberately Lua-agnostic, no `lua_State`
touched anywhere in it (confirmed in `CLAUDE.md`'s own description of the module).

## Phase S: code style

| # | Item | Status |
|---|------|--------|
| S1 | Audit against sunlight's `doc/CODE_STYLE.md`, with real measured counts | TODO |
| S2 | Mechanical alignment pass, object-file-identical proof | TODO |
| S3 | `CLAUDE.md` rule change: "match existing style, don't fix it" becomes "the guide is the reference now" | TODO |

**Rules the owner has already decided (S1 records them, S3 writes them into `CLAUDE.md`):**
- **A class from sunlight (or any other external library) is never forward-declared: include
  its header** (decided 2026-10-22). The header is then the only place that says where the
  class lives, so a namespace move on sunlight's side (like P2's `TileMap` to `Base`) can't leave
  a stale declaration in Scarab that silently declares a phantom class. After a move, the
  build fails cleanly at each line that still names the old path, or not at all. Applied to the
  only two headers that had any: `src/engine/irendererprovider.h` (`ITileMap`,
  `IDrawSurface`) and `src/lua/luaengine.h` (`ITileMap`, `SoundManager`).
- **A forward declaration of Scarab's own classes uses a C++17 nested namespace definition**,
  one line per class, with the project's space-padded `::`:
  `namespace Scarab :: Engine :: Lua  { class LuaCollisionListener; }`, never nested
  `namespace Scarab { namespace Engine { ... } }` blocks (decided 2026-10-22). This applies to
  forward declarations ONLY. Namespace blocks that define Scarab's own code keep today's nested
  style. A qualified name alone (`class Scarab :: Engine :: SpritePool;`) is not valid C++ (it
  cannot introduce a class), so the namespace definition is required. Applied in
  `src/lua/luaengine.h`.

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

Reference in sunlight: `doc/MASTER_PLAN.md` "Integration follow-ups" I2,
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
dedicated `doc/` file, or is dropped), what stays, and the target structure - discussed in
full when this item starts, not decided here.

## Process

- **Peer impact for Caravellius**: [doc/CARAVELLIUS_BREAKAGES.md](CARAVELLIUS_BREAKAGES.md),
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
- This document and `doc/CARAVELLIUS_BREAKAGES.md` are removed from the tree in the same PR
  that merges `master_plan` into `main`, after Caravellius has been told.
