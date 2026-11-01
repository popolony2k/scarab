# Scarab code style

Scarab has two languages: C++ (the engine layer, `src/` and `tests/`) and Lua (the samples, the smoke
tests, `tools/` - and every game built on Scarab, Caravellius first).

- **C++** follows sunlight's guide. Its text is copied below from sunlight's `doc/CODE_STYLE.md`
  as of sunlight commit `ef1499d` (2026-10-30, the version Scarab's pin `9789ac5` still carries), with
  its headings one level down and its Design section replaced by Scarab's own; every other word is
  sunlight's, so the examples are sunlight code. When sunlight changes its guide, this copy
  is updated from it; Scarab does not change the C++ rules on its own.
- **Lua** has no place in sunlight, so its rules are Scarab's own, written in the
  [Lua](#lua) section. They follow the C++ guide wherever the idea carries over (license header,
  spacing, private and public names, no magic numbers), so moving between the two languages
  changes as little as possible. They take in Caravellius' own Lua rules (its `CLAUDE.md`, "Code
  style", as of its commit `22bbaaf`): the `__` module-constant prefix it has applied to every file
  since 2026-08-11, with the cases that are not constants, and its lint (`tools/lua_lint.py`).
  Caravellius' `CLAUDE.md` points to Scarab for style, so this section is the reference for both.

The project rules in [`CLAUDE.md`](../CLAUDE.md) (the Lua API's `nil, "message"` convention, the
`@luaname` documentation tags, unit and smoke tests, AddressSanitizer) apply on top of this guide.

## C++

### Files

- Every file starts with the zlib license header, as in any file under `src/`.
- One module per folder, one namespace per module: `src/collision/` is `SunLight::Collision`.
- File names are lower case and match the class they hold: `CollisionManager` lives in
  `collisionmanager.h` and `collisionmanager.cpp`. An interface's file starts with `i`
  (`isound.h`).
- Include guards are the file name in upper case between double underscores, and the closing
  `#endif` repeats it after one space **(chosen)**:

```cpp
#ifndef __COLLISIONMANAGER_H__
#define __COLLISIONMANAGER_H__

...

#endif /* __COLLISIONMANAGER_H__ */
```

- Includes use paths from `src/` (`"collision/collider.h"`). A `.cpp` file includes its own
  header first, by its path from `src/` too **(chosen)**:
  `#include "collision/collisionmanager.h"`.

### Namespaces and indentation

- Indentation is 4 spaces, never tabs.
- Namespaces are nested and indented, one level each:

```cpp
namespace SunLight  {
    namespace Collision  {

        class CollisionManager : public ICollisionManager  {
            ...
        };
    }
}
```

- A forward declaration follows the same rule: nested and indented, one level each, never the
  one-line C++17 form (`namespace A :: B  { class X; }`). A class of the header's own namespace
  is declared inside that namespace's block; one from another namespace gets its own nested
  block (the example shows the form only - see the next point for when to forward-declare):

```cpp
namespace SunLight  {
    namespace Backends  {
        namespace Null  {
            class NullBackend;
        }
    }
}
```

- Include directly. Forward-declare only to break an include cycle - two of the project's own
  headers that need each other, where one of them needs nothing but the other's name (a
  pointer, a reference, a smart pointer, a `friend`). sunlight has one: `View` points back at
  its `TileMapRenderer`, which holds its views, so `tilemaprenderer.h` includes `view.h` and
  `view.h` declares `class TileMapRenderer;`. Two classes of one file that point at each other
  are the same case (the tests' `MockFont` and `MockEngine`). Everything else is included - a
  backend's class too, and every class of an external library (owner's decision, I10 of the
  master plan; it allowed a backend kept out of a public header before).

- There is no `using namespace` in sunlight's own code; names are written in full
  (`SunLight :: TileMap :: stTile`). Tests may use `using` for the class under test.

### Spacing

- `::` and `->` have a space on each side: `SunLight :: Collision :: Collider`, `std :: vector`,
  `pLayer -> offsetx`.
- A function with no parameters says so: `( void )`, in declarations and definitions. A call
  has none: `Clear();`.
- Inside parentheses there is a space after `(` and before `)`; a keyword takes no space before
  its parenthesis:

```cpp
if( nColliderLayerId < m_ColliderLayerList.size() )  {
for( size_t nListener = 0; nListener < nListeners; nListener++ )  {
switch( pLayer -> type )  {
dst.pos.x = ( int ) ( ( src.pos.x * fZoomFactor ) + vp.pos.x );
```

- A cast has spaces inside its parentheses and one after: `( float ) nWidth`.

### Braces

- An opening brace goes at the end of the line, after **two spaces** **(chosen)**, on every line
  that ends with one: functions, control statements, lambdas, `namespace`, `class`, `struct`
  and `enum`.
- A function body starts with a blank line.
- `else` goes on its own line, never `} else {`:

```cpp
void CollisionManager :: FireOnCollision( SunLight :: Collision :: Collider* pFirst,
                                          SunLight :: TileMap :: stTile* pSecond )  {

    size_t  nListeners = m_Listeners.size();

    for( size_t nListener = 0; nListener < nListeners; nListener++ )  {
        m_Listeners[nListener] -> OnCollision( pFirst, pSecond );
    }
}

if( d1 < 0 )  {
    x++;
}
else  {
    x++;
    y--;
}
```

- `switch` cases are indented one level inside the `switch` and their bodies one more; a case
  label has a space before its colon (`case L_GROUP :`), and each case ends with `break;`:

```cpp
switch( pLayer -> type )  {
    case L_GROUP :
        DrawAllLayers( pLayer -> content.group_head );
        break;

    case L_OBJGR :
        ...
}
```

- A short body may go without braces on the next line (`if( !Tmx() )` / `    return;`).

### Pointers and references

The `*` and the `&` stay with the type **(chosen)**:

```cpp
SunLight :: Canvas :: TextureCanvas* pTexture
SunLight :: TileMap :: stDimension2D& GetDimension2D( void );
void GetCameraPosition( int& nX, int& nY );
```

A pointer is declared alone on its line, so `Type* pA, pB` (one pointer, one value) cannot
mislead.

### Names

Hungarian prefixes tell what a name holds:

| Prefix | For | Example |
|---|---|---|
| `m_` | a class member (before the type prefix) | `m_pParent`, `m_nZOrder`, `m_Sequences` |
| `p` | a pointer | `pLayer`, `m_pMap` |
| `n` | an integer | `nLayerId`, `nWidth` |
| `f` | a `float` or `double` | `fZoomFactor` |
| `b` | a `bool` | `bVisible`, `m_bIsStarted` |
| `sz` | a C string | `szFileName` |
| `str` | a `std::string` | `strTitle` |
| `it` | an iterator | `itItem`, `m_itActiveSequence` |
| `h` | a handle | `hTexture`, `hController` |

- Classes and methods are `PascalCase` (`SpriteRegistry`, `AddSprite`); objects of a class type
  and structs take no prefix (`viewport`, `dimension`, `m_Fonts`).
- Interfaces start with `I` (`ISound`, `ITileMap`); structs with `st` (`stDimension2D`).
- A container type gets a `typedef` named for what it holds:
  `typedef std :: map<int, TextureMap*>  TextureSequenceList;`.
- Enumerators are upper case and start with their type's name: `MAP_ALIGNMENT_TOP_LEFT`,
  `RENDERER_BACKEND_NULL`.

### Constants

- No magic numbers in new code: every value gets a name.
- A constant local to one `.cpp` file is a `#define` with a double underscore, in upper case,
  aligned in columns, with a comment when the value is not obvious:

```cpp
#define __DEFAULT_FPS                   30
#define __MAX_OPACITY_LEVEL            0xFF
#define __CULL_PADDING_TILES           1       // extra tiles either side of the visible range
```

- A constant that users of the library see, in a header, has no underscores in front:
  `MAX_COLLIDER_LAYERS`, `MAX_GAMEPAD_COUNT`, `WHITE_COLOR`.

### Declarations

Types and names are aligned in columns, for members and for local variables alike:

```cpp
TextureSequenceList             m_Sequences;
TextureSequenceList :: iterator m_itActiveSequence;
bool                            m_bIsValidActiveSequence;

float  fZoom  = context.fZoom;
int    nPadX  = __CULL_PADDING_TILES * ( int ) pTmxMap -> tile_width;
```

### Classes

- Data members first, private by default; then `public:`, indented at member level and with a
  blank line on each side:

```cpp
class Sprite : public SunLight :: Canvas :: Canvas  {

    TextureSequenceList             m_Sequences;
    bool                            m_bIsValidActiveSequence;

    public:

    Sprite( void );
    virtual ~Sprite( void );
    ...
};
```

- An interface is pure virtual, with a virtual destructor: `virtual ~ISound( void )  {}`,
  `virtual bool Load( std :: string strFileName ) = 0;`. An implementation marks its methods
  `override`.
- A destructor that deliberately leaves something alone says what and why in its comment (see
  `Sprite :: ~Sprite`, which does not unload the canvases it was given).

### Comments

- Functions are documented with a Doxygen comment, in the header or in the `.cpp`; each
  `@param` ends with `;`:

```cpp
/**
 * Add a collider to manager;
 * @param nColliderLayerId collider layer id to add the collider;
 * @param pCollider Pointer to collider to add;
 */
```

- Comments explain why, not only what. A `/* ... */` block explains a decision where it is made.
- Every claim a comment makes - about a library's behaviour, the standard, a measurement - is
  checked before it is written (read the vendored source, measure it). A wrong "why" is worse
  than none (see `CLAUDE.md`).

### Design

The rules Scarab's code relies on, beyond formatting (each one in `CLAUDE.md`):

- A member that owns heap data is a `std::unique_ptr`; a raw pointer never owns.
- Scarab never calls raylib, SDL or PhysFS itself: everything goes through sunlight's interfaces.
- A Lua-callable function reports a failure as `nil, "message"` and reads its arguments with
  `LuaEngineUtil`'s strict readers, before it needs the renderer.
- Everything that can be tested without a window is tested with doctest (`tests/`); a Lua function's
  behaviour is pinned by a headless smoke test (`scripts/*_smoke.py`); a bug fix comes with a test
  shown to fail on the old code.

### Documentation (Scarab)

Scarab's own addition to the comment rule above:

- **Every function has a Doxygen comment** - not only public methods: an `inline` helper in a header,
  a file-local `static` function in a `.cpp`, a class's private helpers, and `main`. It is written once,
  where the function is declared or where it is defined; a method that implements an interface may rely
  on the interface's comment. It has a `@brief`, a `@param` for each parameter and a `@return` when the
  function returns a value, each ending with `;`:

```cpp
/**
 * @brief The slot index a handle addresses.
 * @param handle The handle;
 * @return The 0-based slot index; -1 for INVALID_SPRITE_HANDLE;
 */
inline int32_t SpriteHandleIndex( SpriteHandle handle )  {
```

- A Lua-callable function's comment also carries its `@luaname`/`@luadoc`/`@luaexample` tags, which
  generate the published Lua API reference (`CLAUDE.md`).

## Lua

### Files

- Every `.lua` file starts with the zlib license header, as a block comment, then a blank line:

```lua
--[[
 Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira
 ...
 3. This notice may not be removed or altered from any source distribution.
]]
```

- After it, a `--[[ ... ]]` block says what the file is for (a sample: what it shows and how to
  run it; a smoke test: what it pins and how it passes or fails).
- File names are lower case. A module's file is named after the module it returns:
  `renderersetup.lua` returns `RendererSetup`.

### Indentation and spacing

- Indentation is 4 spaces, never tabs. A line that continues a call is aligned under it.
- As in C++, inside parentheses there is a space after `(` and before `)`, in calls and in
  definitions; an empty pair has none:

```lua
sprite_set_pos( handle, x, y )
local function draw_text_plate( text, line, color )
app_quit()
```

- Strings use double quotes; single quotes only for a string that holds a double quote.

### Private and public names

The same split as C++: a name only its own file sees is private, one other code reads is public.

- **A private constant** - a `local` at the top of the file, given its value where it is declared
  and never changed (a pool type, a duration, a threshold, a layer id, a state name, a lookup
  table built once and only read) - is `__NAME`, upper case, with Lua 5.4's `<const>` so a
  reassignment is an error. Constants are aligned in columns, with a comment when the value is not
  obvious, like C++'s `#define`s:

```lua
local __FADE_STEP      <const> = 1 / 60
local __HUD_SIZE       <const> = 16
local __GLOW_LAYER     <const> = 4      -- "clouds" in test.tmx: drawn before Sunny's layer
```

  `<const>` holds the variable, not what it points to: a constant table's fields can still be
  changed, so a table meant to stay fixed is never written to.

- **Not a constant, so neither `__` nor `<const>`** (Caravellius' rule):
  - state that changes: per-frame or per-hit values, handle-keyed registries (`ships`,
    `bullets`), state-machine variables, counters and timers - named like any variable
    (`lowerCamel`);
  - a configuration value declared empty and filled once later, eg. from JSON in `init()`
    (`local shipVelocity` ... `shipVelocity = cfg.velocity`): `<const>` needs its value at the
    declaration, and a value that arrives later is state, however rarely it changes;
  - a module's own table (`local Cephalon = {}`): it is the module, named in `PascalCase`.
- **A public constant** - a field of the table a module returns, or a global - is `NAME`, with no
  underscores in front, like a constant in a C++ header: `Movement.STEP_360`, and the constants
  Scarab registers (`KEY_ESCAPE`, `BLEND_MODE_ADD`, `MAP_ALIGNMENT_TOP_LEFT`).
- `__` never clashes with Lua's own names: Lua reserves one underscore followed by capitals
  (`_VERSION`, `_G`, `_ENV`), and metamethods (`__index`, `__gc`) are keys of a metatable, not
  variables.

### Other names

- Functions are `snake_case`, like the Lua API and the engine's callbacks (`on_update`,
  `on_load_stage`): `draw_text_plate`, `set_glow_mode`.
- A module is a table named in `PascalCase` (`RendererSetup`, `Movement`), and its functions are
  `snake_case` fields: `function Movement.step_towards( ... )`.
- Local variables are `lowerCamel`, with no type prefix (a Lua variable has no fixed type):
  `frameCount`, `fadeAlpha`, `mapPixelWidth`.
- Globals are only what the engine calls (`on_update` and the other callbacks) and module tables a
  game shares on purpose. Everything else is `local`.

### Constants and numbers

- No magic numbers: every value that means something gets a private constant, layout and colours
  included (`draw_text( text, __HUD_X, y, __HUD_SIZE, ... )`, not `draw_text( text, 20, 50, 20, ...)`).
- The values a smoke test checks against are test data, not magic numbers: they can be written
  where they are compared.

### Errors

- Every Lua function Scarab provides returns `nil, "message"` when it fails (`CLAUDE.md`), so a
  call whose failure matters is checked: `assert( tilemap_load_map( ... ) )` where a failure should
  stop the script, `local ok, err = ...` where it is handled.
- Numbers that are positions stay as they are: a table field's value (its key names it, as in
  `renderer_create{ width = 960 }`), a subscript (`color[3]`), and a HUD row (`Hud.line( text, 3, ... )`).
  Everything else that means something is a constant. Expected values in a smoke test are test data.

### Checking

- A rule that a script can check is checked by one: `scripts/style_audit.py` measures every rule of
  this guide it can (C++ and Lua), and CI runs `scripts/style_audit.py --check`, which fails on any
  finding - the same idea as Caravellius' `tools/lua_lint.py`, which enforces the `__` prefix on every
  build: it reads the source with comments and strings blanked
  out, and a deliberate exception is marked on the line it applies to (or the line above) with
  `-- lint: allow <rule> (why)`, so every exception carries its reason.

### Order

- Never iterate a string-keyed table with `pairs()` where the order matters (registration, spawn
  order, which pool slot something gets): Lua 5.4 changes that order on every launch
  (`CLAUDE.md`). Use an array, or the keys collected into an array and sorted.
