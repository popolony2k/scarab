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
`feat/master_plan` when the handoff was sent).

## Phase P: pin & plumbing

| # | Item | Status |
|---|------|--------|
| P0 | Write and get this document approved | DONE |
| P1 | Split `SCARAB_SUNLIGHT_VERSION`'s single hardcoded-`v`-prefix mechanism into a fetch-ref variable and a display-version variable; decide `--version`'s format for a commit-pinned build | DONE |
| P2 | Pin `GIT_TAG` to `772851ce7120c0d4f25dbf72c8d15f7e54bcd1d5`; make the code compile against it (the `SunLight::Base` namespace move, BREAKAGES entry 14; `BaseCommand`'s new header, entry 5) | DONE |
| P3 | Every existing Scarab check green against the pin, before any further BREAKAGES item is touched | DONE |

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

## Phase D: design items

| # | Item | Status |
|---|------|--------|
| D1 | `use_default_key_handler` (BREAKAGES entry 26) - keep the Lua shape, back it with `DefaultControllerBindings::Install` | DONE |

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

## Phase V: visual/behavioral re-verification

| # | Item | Status |
|---|------|--------|
| V1 | `MoveCamera*` boundary behavior (BREAKAGES entries 24, 46) - 32 call sites | TODO |
| V2 | Pixel-snap/scroll-boundary rendering fixes at non-integer zoom (entry 35) | TODO |
| V3 | `nOpacity` 0..255 round-trip (entry 41) | TODO |
| V4 | `KEY_MENU` value change, 82 to 5 (entry 2) | TODO |
| V5 | Tile/text/point objects now drawn where they were invisible before (entries 1, 8, 11, 17) | TODO |
| V6 | Rotated shape/tile/text objects (entries 18-20) | TODO |
| V7 | Shape/text object opacity now respects layer opacity (entry 36) | TODO |
| V8 | `AddSprite` before a map loads is now actually drawn, not silently invisible (entry 39) | TODO |
| V9 | `Sprite::Unload()` leaves no active sequence (entry 47) | TODO |
| V10 | `LoadMap` over a loaded map no longer leaks (entry 40) | TODO |
| V11 | The collision lookup no longer reads a wrong tile for a sprite past the map's far edge (entry 4, A6) | TODO |
| V12 | A tile object resized away from its own tile's size now takes its object layer's tint (entry 45) | TODO |

**V1.** Not just a rename: the scroll clamp genuinely changed (the last step can now be
partial, reaching the map's true edge where it used to stop a step short;
`MoveCameraDown`/`Right` can now return exactly to origin, where a `< 0` vs `<= 0` bug used
to block the single step that would). Scope: extend the existing alignment/scroll probe
(built for the sunlight v0.29.0 viewport semantics work) to cover this.

**V2.** Three real rendering bugs sunlight found and fixed at non-integer zoom - a
rasterization seam between adjacent tiles, a scroll-boundary gap of tens of pixels, and a
`std::round` tie-breaking bug at screen positions straddling x/y = 0. Scope: **this needs
checking at Scarab's own actual default (zoom position 60, factor 3.8125), not only at zoom
1.0** - every game using Scarab's unmodified defaults runs at exactly the kind of
non-integer zoom where sunlight's own three bugs were found live (`samples/sprite`, at its
own default zoom).

**V3.** `src/lua/luatilemapapi.cpp`'s `GetLayer`/`SetLayer` pass `nOpacity` straight to
Lua - self-correcting now that both directions are 0..255 (an opaque layer used to read
back as 256), but its own `@luaexample` doc comment ("fade a layer out") names a specific
number that should be checked still reads right.

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
