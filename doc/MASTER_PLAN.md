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
| P1 | Split `SCARAB_SUNLIGHT_VERSION`'s single hardcoded-`v`-prefix mechanism into a fetch-ref variable and a display-version variable; decide `--version`'s format for a commit-pinned build | TODO |
| P2 | Pin `GIT_TAG` to `772851ce7120c0d4f25dbf72c8d15f7e54bcd1d5`; make the code compile against it (the `SunLight::Base` namespace move, BREAKAGES entry 14; `BaseCommand`'s new header, entry 5) | TODO |
| P3 | Every existing Scarab check green against the pin, before any further BREAKAGES item is touched | TODO |

**P0 (DONE).** This document and [`doc/CARAVELLIUS_BREAKAGES.md`](CARAVELLIUS_BREAKAGES.md)
(this plan's own peer-impact log, mirroring sunlight's `BREAKAGES.md` one level down the
chain), reviewed and approved by the owner before the `master_plan` branch held anything
else.

**P1.** `GIT_TAG v${SCARAB_SUNLIGHT_VERSION}` (`CMakeLists.txt`) and
`"sunlight v" SCARAB_SUNLIGHT_VERSION` (`main.cpp`'s `--version` string) both hardcode a
literal `v` in front of the variable, so it can't hold a commit hash as-is - confirmed by
reading both call sites, not assumed. Scope: two variables instead of one - one holding the
exact ref to fetch verbatim, one holding what `--version` prints; `--version`'s exact
wording during this plan (candidate: `sunlight feat/master_plan@<short hash>`, from the
brief's own suggestion).

**P2.** `stDimension2D`/`stSize2D`/`stCoordinate2D` move from `SunLight::TileMap` to
`SunLight::Base`, no aliases kept (BREAKAGES entry 14) - 11 sites in Scarab (`main.cpp`,
`src/lua/luaspriteapi.cpp`, `src/lua/luatilemapapi.cpp`, `src/lua/luarendererapi.cpp`,
`src/lua/luaviewapi.cpp`, `src/lua/luacameraapi.cpp`), confirmed by grep. `BaseCommand`
gaining a virtual destructor moves the command enum to a new header, `scripting/command.h`
(entry 5) - check for any direct include of the old path. Both are flat compile errors the
moment the pin lands, not optional follow-ups, so they land in the same PR as the pin
itself, not after it.

**P3.** Scope: rebuild clean, run `scripts/renderer_api_smoke.py`,
`scripts/tileset_archive_smoke.py`, the headless smoke project
(`scripts/headless_smoke/`), and all 15 samples headless, with no behavior change expected
at this stage.

## Phase D: design items

| # | Item | Status |
|---|------|--------|
| D1 | `use_default_key_handler` (BREAKAGES entry 26) - keep the Lua shape, back it with `DefaultControllerBindings::Install` | TODO |

**D1 (design settled, 2026-10-19; implementation TODO).** `RendererConfig::bUseDefaultKeyHandler`
and the classic 5-argument `TileMapRenderer` constructor are both removed in sunlight; the
equivalent is now an explicit, one-time call,
`SunLight::Input::DefaultControllerBindings::Install( ITileMap& map, IControllerManager& controllerManager )`
(confirmed by reading `src/input/defaultcontrollerbindings.h` directly - stateless,
registering twice re-registers independently, no deduplication). Option B, agreed with the
owner: `renderer_create`'s/`renderer_get_config`'s `use_default_key_handler` keeps its
exact name and meaning (nothing in Scarab - not one sample - sets it `true` outside the
renderer-API smoke test's own option round-trip, confirmed by grep). Scope:
`RendererProvider::Create` calls `Install` once, right after `Start()`, when the option was
requested `true`; confirm `Install` is safe to call before any map is loaded (expected yes,
given `MoveCamera*`'s own null-map guards - not yet tested).

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
