# Scarab TODO

Engine-level work items for **Scarab** (the C++ engine layer — `src/` at
this repo's root, built on top of `sunlight`), earmarked to start **after**
Caravellius splits out into it's own repo (see
[[project_scarab_caravellius_split_plan]] in Claude's own project memory —
that split is scheduled but not yet triggered). Once the split happens,
Scarab gets it's own independent phase planning; this document is the
seed of that plan, not something to act on before then.

Nothing in this document should be started before the split actually
happens, unless the user explicitly says otherwise.

---

## 1. Mapless rendering support (fonts/images/sprites without a loaded Tiled map)

**Status:** investigated, not started. Found 2026-08-21 while building
Caravellius's Phase 10 cover/presentation screen.

### Motivation

Building a title/cover screen (shown before any real gameplay stage
exists, and returned to on game-over/stage-clear) needed a full-screen
image plus blinking text. Scarab's entire stage-bootstrap today
(`on_load_stage` in `games/caravellius/resources/scripts/core/bootstrap.lua`,
calling `tilemap_load_map`) hard-requires a real `.tmx` map to be loaded
before *anything* can be shown — there is no "just show an image / draw
some text" mode that doesn't go through the full stage/map machinery.

The workaround shipped in Caravellius: the cover screen is it's own
"stage" backed by a deliberately minimal, degenerate map
(`games/caravellius/resources/tilemap/cover/cover.tmx`) — a single Tiled
`<imagelayer>` sized to exactly fill the screen, no tile layer or tileset
at all, plus the same empty placeholder object-group layers every other
stage map carries. It works, and is a reasonable stopgap given the
engine's *current* architecture — but it's a workaround, not a real
capability, and the user's own reaction after seeing it was:

> "in the future we need to have fonts, images and sprites also ready to
> run outside a map... make sense for an engine and library also."

### The user's hypothesis, checked directly against source

> "I think sunlight supports at least sprite running outside a map,
> because this layer id used in sunlight sprite's is something logical
> and not physically linked to the map."

**Confirmed TRUE:** a sprite's own "layer id" (`sprite_add_to_layer` →
`LuaSpriteApi::AddToLayer` → `TileMapRenderer::AddSprite`) is a purely
logical integer key into `TileMapRenderer::m_SpriteMap`/
`CollisionManager` — `AddSprite` only checks that `GetLayer(nLayerId)`
finds *some* layer object with that id. `tmx_find_layer_by_id` (libtmx)
matches purely by id, regardless of the actual Tiled layer TYPE — a tile
layer, an `<objectgroup>`, and an `<imagelayer>` are all equally valid
anchors. Confirmed both by reading `AddSprite`'s own source
(`sunlight/src/renderer/tilemaprenderer.cpp`) and empirically: `cover.tmx`
reuses an `<imagelayer>` (not a tile layer) as the anchor for
`LAYER_PLAYER_SHIP` (id=1) — the same "id doesn't care what content sits
there" trick `corsair.tmx` already used for that same id (it's own real
"Background Layer", a genuine tile layer). Both work identically as far
as `AddSprite`/collision registration are concerned.

**Confirmed FALSE — still a real, hard blocker:** sprites cannot exist
with *zero* map loaded at all today. `TileMapRenderer::GetLayer(int)`
calls `::tmx_find_layer_by_id(m_pTmxMap, nLayerId)`, which returns `NULL`
immediately if `m_pTmxMap` is `nullptr` (confirmed in `tmx-src/src/tmx.c`)
— so `AddSprite` unconditionally fails with no map loaded, regardless of
how logical/decoupled the id itself otherwise is. A genuine mapless mode
needs `AddSprite` (and anything else keying off `GetLayer`) to support a
real "no map, layer ids are just abstract sprite buckets with no backing
tmx layer at all" mode — that does not exist today.

**Already effectively mapless-capable — worth knowing before starting
this work:** screen-space drawing (`draw_text`/`measure_text`, and the
`screen_fade` overlay) doesn't touch `m_pTmxMap` at all inside
`TileMapRenderer::RenderMap()`/`Run()` — `RenderMap()`'s own tile-map
drawing step is separately guarded (`if (m_pTmxMap) DrawAllLayers(...)`),
while FPS/fade-overlay drawing happens unconditionally, and
`HandleUserUpdate()` (where Lua's `on_update`/`draw_text` calls actually
happen) runs unconditionally inside `Run()`'s own per-frame
`BeginRenderTarget()/EndRenderTarget()` bracket regardless of whether a
map is loaded. **The actual blocker for "just show text with no map" is
Scarab's own C++ state machine** (`EngineHost`), which requires a stage
(`on_load_stage`, hence a real `.tmx`) to load before it ever transitions
into `STATE_STAGE_RUNNING` and starts calling Lua's `on_update` at all —
not a sunlight rendering limitation. Text/font rendering is closer to
"mapless-ready" than sprites are; the remaining gap there is Scarab's own
bootstrap, not sunlight's renderer.

### How sprite drawing actually works (resolved 2026-08-21)

An earlier pass at this document flagged the sprite-draw dispatch as an
open, unresolved mystery — first-pass tracing (both static reading and a
live diagnostic) only ever found `TileMapRenderer::HandleSpriteUpdate`,
which is called exclusively for whichever single layer id corresponds to
the currently-loaded map's own visible tile layer. Since every gameplay
sprite (enemies, bullets, pickups, options) sits on a deliberately
*invisible* `<objectgroup>` layer (`MAP_AUTHORING.md`'s "empty-objectgroup
pattern"), that path alone can't be what's actually drawing them — yet
they obviously render correctly, every session, for years.

**The user (the engine's own original author) clarified the actual
design directly, from having built it:** a sprite is an independent
visual entity. It only "glues" to a map for two specific, narrower
reasons — respecting the map's own viewport/zoom factor (so a
zoomed/stretched map scales every sprite inside it consistently), and
because the collision manager groups colliders by that same layer id.
Neither of those is "the map decides whether this sprite gets drawn."

**Re-traced with that framing and fully confirmed, this time with the
actual mechanism in hand:** `TileMapRenderer::AddSprite` does two things
with a sprite's layer id — `sprite.SetParent(this)` (viewport/zoom
inheritance, `BaseCanvas::SetParent`) and
`m_CollisionManager.AddCollider(nLayerId, ...)` (collision grouping). It
also stores the sprite in `m_SpriteMap[nLayerId]`, but that particular
bucket is only ever read by `HandleSpriteUpdate` — a sunlight-internal
path, and (per the finding below) not the one Scarab actually uses to
draw.

**The real per-frame draw call lives in Scarab's own code, not sunlight
at all:** `Scarab::Engine::SpritePool::UpdateAll()`
(`src/engine/spritepool.cpp`) — "Advance the animation frame of every
in-use sprite. Called once per frame by the engine so Lua never needs to
remember to do it." — iterates every acquired pool slot directly and
calls `slot.sprite.Update()` (the same `SunLight::Sprite::Sprite::Update()`
that performs the actual texture draw), completely independent of which
tmx layer that sprite is nominally "in," or whether that layer is
visible. It's called from `EngineHost::RunStageStateHandler()`
(`src/host/enginehost.cpp`), once every frame, right before Lua's own
`on_update`:

```cpp
void EngineHost :: RunStageStateHandler( void )  {
    CheckSpritesQueueEmpty();
    RunScriptMachine();

    m_SpritePool.UpdateAll();
    m_LuaEngine.CallOnUpdate( __FRAME_DELTA_MILLI );
}
```

Confirmed empirically too, not just from source: temporary instrumentation
in `Sprite::Update()` (reverted after use) showed 14 distinct sprite
objects — the player ship plus every live enemy/bullet — genuinely
getting their `Update()` call during a real `debug/enemy_main.lua`
session, entirely via this path; `HandleSpriteUpdate` meanwhile still only
ever fired for the player ship's own layer id, exactly as the first pass
found, but that's now understood to be incidental (that one layer happens
to be a real, visible tile layer) rather than evidence of anything special
about how sprites draw.

**So, for this document's actual purpose (mapless rendering):** a map is
needed today because `AddSprite`/`GetLayer` require one to resolve a
layer id at all — not because drawing itself depends on the map. That
narrows the real work needed for genuine mapless sprites to two things,
both now well-understood rather than mysterious:

1. `AddSprite`/`GetLayer` need a "no map, just an abstract sprite bucket"
   mode, since `tmx_find_layer_by_id` unconditionally fails with
   `m_pTmxMap == nullptr`.
2. Collision registration (`CollisionManager::AddCollider`) and viewport
   inheritance (`SetParent`) both key off the same layer id and need to
   keep working without a real backing tmx layer either — `SetParent`
   already takes any `BaseCanvas*`, so this is likely a smaller change
   than it first looked; `AddCollider` just needs the layer id treated as
   a plain grouping key, which it already effectively is.
3. `SpritePool::UpdateAll()` itself needs **zero changes** — it already
   draws every acquired sprite regardless of map state, which is exactly
   the mapless behavior this document originally wanted for sprites.

### Recommended first steps, whenever this is picked up

1. Extend `AddSprite`/`GetLayer` (and whatever `LuaSpriteApi` surface sits
   on top) to accept operating with no map loaded — layer ids become pure
   grouping keys for collision, with no tmx layer required to back them.
2. Extend Scarab's own `EngineHost` state machine so a
   `STATE_STAGE_RUNNING`-equivalent per-frame update loop (which already
   calls `SpritePool::UpdateAll()` unconditionally) doesn't require a
   stage/map to have ever been loaded at all — text/font rendering is
   already architecturally ready for this on the sunlight side (see
   above); the remaining gap is Scarab's own bootstrap sequencing.
3. Caravellius's own `cover.tmx` workaround does not need to be ripped
   out as a prerequisite — once a real mapless primitive exists, it can
   be swapped over as a routine follow-up whenever convenient, not as a
   blocking step of this work itself.
