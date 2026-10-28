/*
 * Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "lua/luaspriteapi.h"
#include "lua/luaengineutil.h"
#include "lua/luablendapi.h"
#include "engine/spritehandle.h"
#include "collision/collisionmanager.h"
#include <climits>
#include <cstdint>

extern "C"
{
  #include "lauxlib.h"
}

using namespace SunLight :: Canvas;

#define __CONST( name )  { #name, ( int ) name }

/** delayMilli's default and smallest value: a held frame (see sprite_configure_texture). */
#define __HELD_FRAME_DELAY  -1


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @luaconstants{Animation modes}
             * @luadoc
             * The `animationMode` argument `sprite_configure_texture`/
             * `sprite_set_animation_mode` take — see either
             * primitive's own doc for the full behavior of each mode.
             */
            static const stNamedConstant  s_aAnimationModes[] = {
                __CONST( TEXTURE_ANIMATION_MODE_MANUAL ),
                __CONST( TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR ),
                __CONST( TEXTURE_ANIMATION_MODE_AUTOMATIC_RIGHT_LEFT ),
                __CONST( TEXTURE_ANIMATION_MODE_ANIMATE_LEFT ),
                __CONST( TEXTURE_ANIMATION_MODE_ANIMATE_RIGHT ),
                __CONST( TEXTURE_ANIMATION_MODE_ANIMATE_CENTER ),
            };

            /** @brief Read an animationMode argument (a TEXTURE_ANIMATION_MODE_* constant). */
            static bool ReadAnimationModeArg( lua_State *pLuaState, int nIndex, AnimationMode &mode, std :: string &strError )  {

                int  nMode = 0;

                if( !LuaEngineUtil :: ReadConstantArg( pLuaState, nIndex, "animationMode", s_aAnimationModes,
                                                       sizeof( s_aAnimationModes ) / sizeof( s_aAnimationModes[0] ),
                                                       "TEXTURE_ANIMATION_MODE_*", nMode, strError ) )
                    return false;

                mode = ( AnimationMode ) nMode;

                return true;
            }

            /**
             * @brief Read a layer id argument: sunlight keeps a sprite's collider in a fixed array
             * of MAX_COLLIDER_LAYERS lists, so a layer id outside 0..MAX_COLLIDER_LAYERS-1 can't
             * hold a sprite - checked here so a script gets that reason, not a plain refusal.
             */
            static bool ReadLayerIdArg( lua_State *pLuaState, int nIndex, int &nLayerId, std :: string &strError )  {

                lua_Integer  nValue = 0;

                if( !LuaEngineUtil :: ReadIntegerArgInRange( pLuaState, nIndex, "layerId", 0, MAX_COLLIDER_LAYERS - 1, nValue, strError ) )
                    return false;

                nLayerId = ( int ) nValue;

                return true;
            }

            /**
             * @brief Resolve a sprite handle argument strictly: a non-integer or a handle that is
             * not live (never acquired, or already released) is an error.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nIndex Stack index of the handle;
             * @param strError Receives the reason on failure;
             * @return The sprite, or nullptr (with strError set);
             */
            static SunLight :: Sprite :: Sprite* ResolveHandleArg( lua_State *pLuaState, int nIndex, std :: string &strError )  {

                SunLight :: Sprite :: Sprite  *pSprite = nullptr;

                if( lua_isinteger( pLuaState, nIndex ) )
                    pSprite = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Resolve( ( SpriteHandle ) lua_tointeger( pLuaState, nIndex ) );

                if( pSprite == nullptr )
                    strError = "invalid sprite handle";

                return pSprite;
            }

            /**
             * @luaname{pool_register_type(typeTag, capacity) -> true | nil, message}
             * @luaheading{Error reporting}
             * @luadoc
             * Every sprite function reports a failure as `nil, "message"` and
             * never raises a Lua error: `local ok, err = sprite_set_pos( h, x, y )`.
             * A function with nothing else to return returns `true` on success.
             * Arguments are checked strictly: `"invalid sprite handle"` (never
             * acquired, or already released), `"argument 'x' must be an integer"`
             * (a string, a missing argument, or a fractional number like `3.5` —
             * round it with `math.floor` first; `3.0` is fine), `"argument 'visible'
             * must be a boolean"` (not `nil`, not `0`/`1`).
             *
             * Reserve `capacity` slots (1 or more) for sprites acquired with this
             * `typeTag` string. Must be called once before the first
             * `sprite_acquire` for that tag. Fails for a tag already registered,
             * and when the pool would hold more than 65535 sprites in all.
             */
            int LuaSpriteApi :: RegisterType( lua_State *pLuaState )  {

                std :: string  strTypeTag;
                lua_Integer    nCapacity = 0;
                std :: string  strError;
                SpritePool     *pPool    = LuaEngineUtil :: GetSpritePool( pLuaState );

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "typeTag", strTypeTag, strError ) ||
                    !LuaEngineUtil :: ReadIntegerArgInRange( pLuaState, 2, "capacity", 1, MAX_SPRITE_SLOTS, nCapacity, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( pPool -> HasType( strTypeTag ) )
                    return LuaEngineUtil :: PushError( pLuaState, "pool type '" + strTypeTag + "' is already registered" );

                if( !pPool -> RegisterType( strTypeTag, ( uint32_t ) nCapacity ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the pool holds at most " + std :: to_string( MAX_SPRITE_SLOTS ) +
                                                       " sprites in all, and " + std :: to_string( pPool -> GetSlotCount() ) + " are registered" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{sprite_acquire(typeTag) -> handle | nil, message}
             * @luadoc
             * Get a free handle from the pool registered for `typeTag`.
             * Fails for a tag never registered and when every slot of that
             * tag is in use (`"pool type 'x' is full (N sprites)"`). A handle is
             * never `0`.
             */
            int LuaSpriteApi :: Acquire( lua_State *pLuaState )  {

                std :: string  strTypeTag;
                std :: string  strError;
                SpritePool     *pPool = LuaEngineUtil :: GetSpritePool( pLuaState );

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "typeTag", strTypeTag, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pPool -> HasType( strTypeTag ) )
                    return LuaEngineUtil :: PushError( pLuaState, "unknown pool type '" + strTypeTag + "' (see pool_register_type)" );

                SpriteHandle  handle = pPool -> Acquire( strTypeTag );

                if( handle == INVALID_SPRITE_HANDLE )
                    return LuaEngineUtil :: PushError( pLuaState, "pool type '" + strTypeTag + "' is full" );

                lua_pushinteger( pLuaState, ( lua_Integer ) handle );

                return 1;
            }

            /**
             * @luaname{sprite_release(handle) -> true | nil, message}
             * @luadoc
             * Return a handle to its pool, making the slot available for
             * a future `sprite_acquire`. **Call
             * `sprite_remove_from_layer` first** if the sprite was ever
             * added to a layer (see the gotcha above).
             */
            int LuaSpriteApi :: Release( lua_State *pLuaState )  {

                std :: string  strError;

                if( ResolveHandleArg( pLuaState, 1, strError ) == nullptr )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                LuaEngineUtil :: GetSpritePool( pLuaState ) -> Release( ( SpriteHandle ) lua_tointeger( pLuaState, 1 ) );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Load a texture sequence's file and configure it's tiling/
             * animation, mirroring the old WorldBase::LoadSprite per-texture body.
             *
             * @luaname{sprite_configure_texture(handle, sequenceId, path, framesByTexture, activeTileIndex, animationMode, delayMilli?) -> true | nil, message}
             * @luadoc
             * Load a texture file and configure it as animation
             * `sequenceId` on `handle`. `path` points at a texture sheet
             * containing `framesByTexture` equal-width frames laid out
             * horizontally; `activeTileIndex` is which frame to start on.
             * `delayMilli` (optional, defaults to `-1`) is the time, in
             * milliseconds, between animation steps of this sequence. **`-1`
             * means "no animation": the frame is held forever**, whatever
             * `animationMode` says — a sprite configured without a delay
             * shows its `activeTileIndex` frame and never steps. Pass a real
             * delay (say `100`) for any sprite whose frames should cycle:
             * `AUTOMATIC_CIRCULAR`/`AUTOMATIC_RIGHT_LEFT` step about once per
             * `delayMilli`. Configuring the same sequence again (a recycled
             * pool slot) reloads the texture and applies the new `delayMilli`;
             * the sequence keeps holding exactly one texture.
             *
             * Fails for an invalid handle, a `framesByTexture` below 1, an
             * `activeTileIndex` outside `0`..`framesByTexture - 1`, an unknown
             * mode, a `delayMilli` below `-1`, and a texture that can't be loaded
             * (`"cannot load texture '<path>'"`). The arguments are checked
             * before anything is loaded, so a wrong call never creates the
             * default renderer.
             *
             * Animation mode constants:
             *
             * | Constant | Behavior |
             * |---|---|
             * | `TEXTURE_ANIMATION_MODE_MANUAL` | No automatic frame advance — you control the active tile yourself |
             * | `TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR` | Cycles through frames automatically, wrapping around |
             * | `TEXTURE_ANIMATION_MODE_AUTOMATIC_RIGHT_LEFT` | Cycles automatically, bouncing back and forth |
             * | `TEXTURE_ANIMATION_MODE_ANIMATE_LEFT` / `_RIGHT` / `_CENTER` | Runtime-switchable directional animation — the player ship uses these to bank left/right/center in response to input (see `sprite_set_animation_mode` below) |
             * @luaexample
             * -- a 5-frame strip, starting on frame 2, auto-animating
             * sprite_configure_texture(handle, 0, path, 5, 2, TEXTURE_ANIMATION_MODE_ANIMATE_CENTER, 100)
             */
            int LuaSpriteApi :: ConfigureTexture( lua_State *pLuaState )  {

                std :: string   strError;
                std :: string   strPath;
                int             nSequenceId      = 0;
                lua_Integer     nFramesByTexture = 0;
                lua_Integer     nActiveTileIndex = 0;
                lua_Integer     nDelayMilli      = __HELD_FRAME_DELAY;
                AnimationMode   animationMode    = TEXTURE_ANIMATION_MODE_MANUAL;

                SunLight :: Sprite :: Sprite  *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "sequenceId", nSequenceId, strError ) ||
                    !LuaEngineUtil :: ReadStringArg( pLuaState, 3, "path", strPath, strError ) ||
                    !LuaEngineUtil :: ReadIntegerArgInRange( pLuaState, 4, "framesByTexture", 1, INT_MAX, nFramesByTexture, strError ) ||
                    !LuaEngineUtil :: ReadIntegerArgInRange( pLuaState, 5, "activeTileIndex", 0, nFramesByTexture - 1, nActiveTileIndex, strError ) ||
                    !ReadAnimationModeArg( pLuaState, 6, animationMode, strError ) ||
                    !LuaEngineUtil :: ReadOptionalIntegerArg( pLuaState, 7, "delayMilli", __HELD_FRAME_DELAY, nDelayMilli, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( nDelayMilli < __HELD_FRAME_DELAY || nDelayMilli > INT_MAX )
                    return LuaEngineUtil :: PushError( pLuaState, "argument 'delayMilli' must be -1 (a held frame) or more, got " + std :: to_string( nDelayMilli ) );

                /*
                 * Loading a texture needs a live window/GL context, but this primitive never
                 * touches GetTileMap()/GetDrawSurface() itself - make sure the renderer exists
                 * (creating the default one if the script hasn't) before anything is loaded.
                 */
                LuaEngineUtil :: EnsureRenderer( pLuaState );

                SpriteHandle   handle   = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                SpritePool     *pPool   = LuaEngineUtil :: GetSpritePool( pLuaState );
                TextureCanvas  *pTexture = pPool -> GetOrCreateTexture( handle, nSequenceId );

                /*
                 * A recycled pool slot's TextureCanvas may already hold a
                 * previously loaded texture (e.g. a prior life of this same
                 * slot) - unload it first or the old GPU texture handle leaks
                 * every time this sequence gets reconfigured.
                 */
                pTexture -> Unload();

                if( !pTexture -> Load( strPath.c_str() ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot load texture '" + strPath + "'" );

                /*
                 * pTexture->GetDimension2D() is only reliable the first time
                 * this sequence is ever configured - Sprite::AddTextureSequence
                 * (below) unconditionally aliases it to the sprite's own shared
                 * dimension from then on, so a later reconfigure (recycled pool
                 * slot) would otherwise divide a stale/aliased width instead of
                 * this texture's true natural size. GetOrCacheNativeWidth pins
                 * down the correct value the first time and returns it on every
                 * later call regardless of what's currently aliased.
                 */
                int  nNativeWidth  = pPool -> GetOrCacheNativeWidth( handle, nSequenceId, pTexture -> GetDimension2D().size.nWidth );
                int  nTextureWidth = nNativeWidth / ( int ) nFramesByTexture;

                /*
                 * Same caching for height (not frame-divided, but equally
                 * unreliable to re-read post-aliasing) so SetActiveSequence can
                 * sync it correctly even for sequences whose height genuinely
                 * differs from each other (not the case for Satellite's own
                 * assets today, but will be for other enemy types).
                 */
                pPool -> GetOrCacheNativeHeight( handle, nSequenceId, pTexture -> GetDimension2D().size.nHeight );

                pTexture -> SetCenterTileIndex( ( int ) nActiveTileIndex );
                pTexture -> SetActiveTileIndex( ( int ) nActiveTileIndex );
                pTexture -> SetTileSize( nTextureWidth );
                pTexture -> SetAnimationMode( animationMode );

                /*
                 * A recycled pool slot already holds this sequence's canvas: adding it again would
                 * append a second entry for the same canvas (one more every recycle - memory growth,
                 * and the entries step one after another with their own delays, so the animation pace
                 * changed with the recycle count). Keep the entry and only apply the (possibly new)
                 * delay; the canvas's parent, dimension and mode were set the first time round.
                 */
                if( pSprite -> GetTextureSequenceSize( nSequenceId ) > 0 )
                    pSprite -> SetTextureSequenceDelay( nSequenceId, ( int ) nDelayMilli );
                else
                    pSprite -> AddTextureSequence( nSequenceId, pTexture, ( int ) nDelayMilli );

                pSprite -> SetVisible( true );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Set the sprite's active texture sequence, also syncing it's
             * bounding-box width/height from that sequence's cached natural
             * size - a step callers had to remember to do separately in the old
             * C++ path (and the old path never had to worry about a recycled,
             * reconfigured texture's size becoming unreliable to re-read).
             *
             * @luaname{sprite_set_active_sequence(handle, sequenceId) -> true | nil, message}
             * @luadoc
             * Switch which configured sequence is currently
             * displayed/collided against (also syncs the sprite's
             * bounding box to that sequence's size — important since a
             * "main" and an "explosion" sequence are often different
             * sizes). Fails for an invalid handle and for a sequence never
             * configured with `sprite_configure_texture`
             * (`"sequence N is not configured"`).
             *
             * **Gotcha: this unconditionally resets the target
             * sequence's own animation state**, even if it's already the
             * active sequence — confirmed via sunlight's own source
             * (`Sprite::SetActiveTextureSequence`,
             * `src/sprite/sprite.cpp`) unconditionally calling `Reset()`
             * on the newly-selected sequence's `TextureCanvas`, which
             * snaps it back to it's own starting tile every time.
             * Calling this every frame regardless of whether the
             * sequence actually changed (found live building
             * Options/satellites' pose-sync, 2026-08-19 — see
             * `caravellius/src/core/options.lua`'s own header comment)
             * silently freezes any
             * `AUTOMATIC_CIRCULAR`/`AUTOMATIC_RIGHT_LEFT` animation on
             * that sequence at it's first frame forever, since the reset
             * undoes each frame's own advance before it's ever drawn.
             * Always track the last sequence you actually set and only
             * call this again when it genuinely changes:
             *
             * ```lua
             * if sat.lastLeanIndex ~= leanIndex then
             *   sprite_set_active_sequence(sat.handle, leanIndex)
             *   sat.lastLeanIndex = leanIndex
             * end
             * ```
             *
             * **Pattern: pose-following without a new engine
             * primitive.** Nothing exposes a sequence's own *currently
             * animating tile* back to Lua (`sprite_get_active_sequence`
             * returns which *sequence* is active, not the current *tile*
             * within it) — so a sprite that needs to track another
             * sprite's real-time animated pose (e.g. Options' satellites
             * echoing the player ship's own left/center/right lean)
             * can't just read that pose back from C++. The working
             * pattern, built for exactly this case: give the follower
             * one texture sequence per pose the leader can be in
             * (`sprite_configure_texture` called once per pose, sequence
             * id == pose index), then have Lua *independently recompute*
             * which pose the leader is currently in (mirroring whatever
             * discrete input/state drives the leader's own animation,
             * ticked at the leader's own real `animation_delay`) and
             * drive the follower's `sprite_set_active_sequence` from
             * that — see `player.lua`'s
             * `update_lean_index`/`Player.get_lean_index()` and
             * `options.lua`'s pose-sync loop for a full worked example.
             * Cheaper and simpler than adding a C++ read-back primitive
             * for a purely cosmetic need.
             * @luaexample
             * sprite_set_active_sequence(handle, 1)  -- switch to the explosion sequence
             */
            int LuaSpriteApi :: SetActiveSequence( lua_State *pLuaState )  {

                std :: string  strError;
                int            nSequenceId = 0;

                SunLight :: Sprite :: Sprite  *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr || !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "sequenceId", nSequenceId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SpriteHandle   handle   = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                SpritePool     *pPool   = LuaEngineUtil :: GetSpritePool( pLuaState );
                TextureCanvas  *pTexture = pPool -> GetTexture( handle, nSequenceId );

                if( pTexture == nullptr || !pSprite -> SetActiveTextureSequence( nSequenceId ) )
                    return LuaEngineUtil :: PushError( pLuaState, "sequence " + std :: to_string( nSequenceId ) + " is not configured" );

                pSprite -> GetDimension2D().size.nWidth  = pTexture -> GetTileSize();
                pSprite -> GetDimension2D().size.nHeight = pPool -> GetOrCacheNativeHeight( handle, nSequenceId, 0 );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Change a texture sequence's animation mode after it's
             * already been configured - needed by sprites (eg. the player ship)
             * that switch between ANIMATE_LEFT/RIGHT/CENTER at runtime in
             * response to input, unlike enemies which set their mode once at
             * ConfigureTexture time and never touch it again.
             *
             * @luaname{sprite_set_animation_mode(handle, sequenceId, mode) -> true | nil, message}
             * @luadoc
             * Change a sequence's animation mode *after* it's already
             * configured — for sprites that switch modes at runtime
             * (e.g. the player ship banking left/right based on input).
             * Enemies that only ever set their mode once typically just
             * pass the final mode to `sprite_configure_texture` and never
             * call this. Fails for an invalid handle, an unknown mode, and a
             * sequence never configured.
             * @luaexample
             * sprite_set_animation_mode(handle, 0, TEXTURE_ANIMATION_MODE_ANIMATE_LEFT)
             */
            int LuaSpriteApi :: SetAnimationMode( lua_State *pLuaState )  {

                std :: string   strError;
                int             nSequenceId = 0;
                AnimationMode   mode        = TEXTURE_ANIMATION_MODE_MANUAL;

                if( ResolveHandleArg( pLuaState, 1, strError ) == nullptr ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "sequenceId", nSequenceId, strError ) ||
                    !ReadAnimationModeArg( pLuaState, 3, mode, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                TextureCanvas  *pTexture = LuaEngineUtil :: GetSpritePool( pLuaState ) -> GetTexture( ( SpriteHandle ) lua_tointeger( pLuaState, 1 ), nSequenceId );

                if( pTexture == nullptr )
                    return LuaEngineUtil :: PushError( pLuaState, "sequence " + std :: to_string( nSequenceId ) + " is not configured" );

                pTexture -> SetAnimationMode( mode );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{sprite_get_active_sequence(handle) -> sequenceId | nil, message}
             */
            int LuaSpriteApi :: GetActiveSequence( lua_State *pLuaState )  {

                std :: string  strError;

                SunLight :: Sprite :: Sprite  *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushinteger( pLuaState, pSprite -> GetActiveTextureSequence() );

                return 1;
            }

            /**
             * @luaname{sprite_set_visible(handle, visible) -> true | nil, message}
             * @luagroup{visibility}
             * @luaheading{Visibility}
             * @luadoc
             * `visible` must be a boolean. `sprite_get_visible` returns
             * `nil, "invalid sprite handle"` for a handle that isn't live.
             * @luaexample
             * sprite_set_visible(handle, visible)
             * sprite_get_visible(handle) -> visible
             */
            int LuaSpriteApi :: SetVisible( lua_State *pLuaState )  {

                std :: string  strError;
                bool           bVisible = false;

                SunLight :: Sprite :: Sprite  *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr || !LuaEngineUtil :: ReadBooleanArg( pLuaState, 2, "visible", bVisible, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                pSprite -> SetVisible( bVisible );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{sprite_get_visible(handle) -> visible | nil, message}
             * @luagroup{visibility}
             */
            int LuaSpriteApi :: GetVisible( lua_State *pLuaState )  {

                std :: string  strError;

                SunLight :: Sprite :: Sprite  *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushboolean( pLuaState, pSprite -> GetVisible() );

                return 1;
            }

            /**
             * @luaname{sprite_set_world_space(handle, world_space) -> true | nil, message}
             * @luagroup{world_space}
             * @luaheading{World space}
             * @luadoc
             * Choose what the sprite's position (`sprite_set_pos`) means. Off by default: the
             * position is **relative to the view that draws it** — measured from the view's own
             * top-left, scaled by its zoom, and ignoring its camera — which is what a HUD or a
             * single-view game wants. With `world_space` on, the position is a **map position in
             * pixels**: every view draws the sprite where it draws the map at that position, so one
             * sprite is correct in the main view, in a minimap and in a close-up, whatever their
             * zoom and camera. It applies to every texture sequence of the sprite, including ones
             * configured later; `sprite_release` turns it back off, so a recycled handle never
             * inherits it.
             *
             * **Collisions:** a collider uses its sprite's position as it is, so a world-space
             * sprite collides in *map* coordinates and a screen-relative one does not — do not
             * put both kinds in one collision rule. **Games that never call this are unchanged.**
             * `sprite_get_world_space` reads it back (`nil, "invalid sprite handle"` for a
             * handle that isn't live); `world_space` must be a boolean.
             * @luaexample
             * local enemy = sprite_acquire( "enemy" )
             * sprite_configure_texture( enemy, 0, ... )
             * sprite_set_world_space( enemy, true )
             * sprite_set_pos( enemy, 640, 320 )   -- a MAP position: shown right in every view
             */
            int LuaSpriteApi :: SetWorldSpace( lua_State *pLuaState )  {

                std :: string  strError;
                bool           bWorldSpace = false;

                SunLight :: Sprite :: Sprite  *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr || !LuaEngineUtil :: ReadBooleanArg( pLuaState, 2, "world_space", bWorldSpace, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                pSprite -> SetWorldSpace( bWorldSpace );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{sprite_get_world_space(handle) -> world_space | nil, message}
             * @luagroup{world_space}
             */
            int LuaSpriteApi :: GetWorldSpace( lua_State *pLuaState )  {

                std :: string  strError;

                SunLight :: Sprite :: Sprite  *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushboolean( pLuaState, pSprite -> IsWorldSpace() );

                return 1;
            }

            /**
             * @luaname{sprite_set_blend_mode(handle, mode) -> true | nil, message}
             * @luagroup{sprite_blend}
             * @luaheading{Blend mode}
             * @luadoc
             * Give the sprite its own blend mode (a `BLEND_MODE_*` constant, see the Blend modes
             * page): it draws by that mode in every view and the mode in effect is restored
             * right after, so it affects that sprite alone — a glow in `BLEND_MODE_ADD`, a soft
             * shadow in `BLEND_MODE_MULTIPLY`. It stays until `sprite_clear_blend_mode`;
             * `sprite_release` clears it, so a recycled handle never inherits it.
             *
             * `sprite_get_blend_mode` returns the sprite's mode, or `nil` when it has none of its
             * own. That is different from `BLEND_MODE_NORMAL`: a sprite with no mode draws by
             * whatever mode is in effect, one set to `BLEND_MODE_NORMAL` always draws normally.
             * A sprite is drawn before `on_update`, so `draw_begin_blend_mode` never reaches it.
             *
             * Failures are `nil, "message"`, like every sprite function: `"invalid sprite
             * handle"`, `"unknown blend mode N"` or `"expected a BLEND_MODE_* constant"`.
             * **Games that never call them are unchanged.**
             * @luaexample
             * local glow = sprite_acquire( "glow" )
             * sprite_configure_texture( glow, ... )
             * assert( sprite_set_blend_mode( glow, BLEND_MODE_ADD ) )   -- always drawn as a light
             * assert( sprite_get_blend_mode( glow ) == BLEND_MODE_ADD )
             * sprite_clear_blend_mode( glow )
             * assert( sprite_get_blend_mode( glow ) == nil )             -- no mode of its own
             */
            int LuaSpriteApi :: SetBlendMode( lua_State *pLuaState )  {

                std :: string                  strError;
                SunLight :: Base :: BlendMode  mode    = SunLight :: Base :: BLEND_MODE_NORMAL;
                SunLight :: Sprite :: Sprite   *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !LuaBlendApi :: ReadBlendMode( pLuaState, 2, mode, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                pSprite -> SetBlendMode( mode );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{sprite_clear_blend_mode(handle) -> true | nil, message}
             * @luagroup{sprite_blend}
             */
            int LuaSpriteApi :: ClearBlendMode( lua_State *pLuaState )  {

                std :: string                  strError;
                SunLight :: Sprite :: Sprite   *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                pSprite -> ClearBlendMode();

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{sprite_get_blend_mode(handle) -> mode | nil | nil, message}
             * @luagroup{sprite_blend}
             */
            int LuaSpriteApi :: GetBlendMode( lua_State *pLuaState )  {

                std :: string                  strError;
                SunLight :: Sprite :: Sprite   *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pSprite -> HasBlendMode() )  {
                    lua_pushnil( pLuaState );
                    return 1;
                }

                lua_pushinteger( pLuaState, pSprite -> GetBlendMode() );

                return 1;
            }

            /**
             * @luaname{sprite_get_pos(handle) -> x, y | nil, message}
             * @luagroup{position_size}
             * @luaheading{Position and size}
             * @luadoc
             * Position is **top-left anchored**, not center-anchored —
             * `x, y` is the sprite's top-left corner, matching both the
             * render clip rect and the AABB collision math. `pos -
             * height/2` means "above the sprite's own top edge," not
             * "above center."
             *
             * `x` and `y` must be integers: a fractional value like `3.5` is
             * an error (it used to be silently written as `0`, which made a
             * sprite moved by a fractional velocity look stuck) — round it
             * with `math.floor` first.
             * @luaexample
             * sprite_get_pos(handle) -> x, y
             * sprite_set_pos(handle, x, y)
             * sprite_get_size(handle) -> width, height
             */
            int LuaSpriteApi :: GetPos( lua_State *pLuaState )  {

                std :: string  strError;

                SunLight :: Sprite :: Sprite  *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: Base :: stCoordinate2D&  pos = pSprite -> GetDimension2D().pos;

                lua_pushinteger( pLuaState, pos.x );
                lua_pushinteger( pLuaState, pos.y );

                return 2;
            }

            /**
             * @luaname{sprite_set_pos(handle, x, y) -> true | nil, message}
             * @luagroup{position_size}
             */
            int LuaSpriteApi :: SetPos( lua_State *pLuaState )  {

                std :: string  strError;
                int            nX = 0;
                int            nY = 0;

                SunLight :: Sprite :: Sprite  *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "x", nX, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 3, "y", nY, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                pSprite -> GetDimension2D().pos.x = nX;
                pSprite -> GetDimension2D().pos.y = nY;

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{sprite_get_size(handle) -> width, height | nil, message}
             * @luagroup{position_size}
             */
            int LuaSpriteApi :: GetSize( lua_State *pLuaState )  {

                std :: string  strError;

                SunLight :: Sprite :: Sprite  *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: Base :: stSize2D&  size = pSprite -> GetDimension2D().size;

                lua_pushinteger( pLuaState, size.nWidth );
                lua_pushinteger( pLuaState, size.nHeight );

                return 2;
            }

            /**
             * @brief Shrink the sprite's own collision area relative to it's
             * full render size - e.g. a non-rectangular sprite (a ship hull
             * tapered at bow/stern, a round enemy) whose actual art doesn't
             * fill it's whole bounding box. Every percentage is a fraction
             * (0.0-1.0) of the sprite's own current width/height, recomputed
             * every collision check rather than baked in - it stays correct
             * even if the sprite's active sequence later changes size. Only
             * affects this sprite's own side of a collision test (see
             * Collider::SetInset's own doc comment in sunlight) - the
             * sprite's drawn size/position and how other sprites see it are
             * untouched.
             *
             * @luaname{sprite_set_collision_inset(handle, leftPct, topPct, rightPct, bottomPct) -> true | nil, message}
             * @luaheading{Collision inset}
             * @luadoc
             * Shrinks the rectangle used for this sprite's own side of a
             * collision test, relative to it's full render size — useful
             * for a non-rectangular sprite (a ship hull tapered at
             * bow/stern, a round enemy) whose actual art doesn't fill
             * it's whole bounding box, so the default full-size hitbox
             * reads as unfair ("that clearly missed"). Every argument is
             * a fraction (`0.0`-`1.0`) of the sprite's own current
             * width/height, e.g.
             * `sprite_set_collision_inset(handle, 0.15, 0.10, 0.15, 0.10)`
             * shrinks 15% off each side and 10% off top/bottom.
             * Recomputed from the sprite's current size on every
             * collision check rather than cached as pixels, so it stays
             * correct even if the active sequence later changes size.
             * Defaults to `0` on every side (today's full-size behavior)
             * until called — only affects this sprite's own side of the
             * test, not how other sprites collide against each other.
             * Only shrinks the collision rectangle, never the drawn
             * sprite or it's tracked position. A fraction outside `0.0`-`1.0`,
             * or a non-number, is an error.
             * @luaexample
             * sprite_set_collision_inset(handle, leftPct, topPct, rightPct, bottomPct)
             */
            int LuaSpriteApi :: SetCollisionInset( lua_State *pLuaState )  {

                static const char * const  aszNames[4] = { "leftPct", "topPct", "rightPct", "bottomPct" };

                std :: string  strError;
                double         afInset[4] = { 0.0, 0.0, 0.0, 0.0 };

                SunLight :: Sprite :: Sprite  *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                for( int nArg = 0; nArg < 4; nArg++ )
                    if( !LuaEngineUtil :: ReadNumberArgInRange( pLuaState, 2 + nArg, aszNames[nArg], 0.0, 1.0, afInset[nArg], strError ) )
                        return LuaEngineUtil :: PushError( pLuaState, strError );

                pSprite -> GetCollider().SetInset( ( float ) afInset[0], ( float ) afInset[1], ( float ) afInset[2], ( float ) afInset[3] );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Add the sprite to a Tiled layer, restoring it's collider
             * parent and stashing the opaque handle on the collider so a future
             * collision callback can decode which sprite collided without
             * exposing raw pointers to Lua.
             *
             * @luaname{sprite_add_to_layer(handle, layerId) -> true | nil, message}
             * @luagroup{layers}
             * @luaheading{Layers}
             * @luadoc
             * Adding a sprite to a layer is also what registers it for
             * collision detection on that layer (see [collision.md](collision.md)) — a
             * sprite never added to any layer never collides with
             * anything, regardless of `collision_add_rule`.
             *
             * Fails for an invalid handle, for a layer id that isn't a layer of
             * the loaded map (`"layer N is not a layer of the loaded map"` —
             * before, that silently drew nothing), and for a sprite already on
             * that layer. With no map loaded yet any layer id is accepted, and
             * the sprite is drawn right away: without a map a layer id is a
             * grouping key, drawn in ascending id order, that a view's layer
             * mask hides by id (`samples/mapless`).
             * Layer ids are integers from `0` to `254`.
             * `sprite_remove_from_layer` fails when the sprite isn't on that
             * layer.
             * @luaexample
             * assert( sprite_add_to_layer(handle, layerId) )
             * sprite_remove_from_layer(handle, layerId)
             */
            int LuaSpriteApi :: AddToLayer( lua_State *pLuaState )  {

                std :: string  strError;
                int            nLayerId = 0;

                SunLight :: Sprite :: Sprite  *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr || !ReadLayerIdArg( pLuaState, 2, nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: TileMap :: ITileMap    *pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );
                SunLight :: TileMap :: stMapInfo   mapInfo;
                SunLight :: TileMap :: stLayer     layer;

                // With no map loaded any layer id is accepted (the sprite is drawn once a map with that layer loads).
                if( pTileMap -> GetMapInfo( mapInfo ) && !pTileMap -> GetLayer( nLayerId, layer ) )
                    return LuaEngineUtil :: PushError( pLuaState, "layer " + std :: to_string( nLayerId ) + " is not a layer of the loaded map" );

                if( !pTileMap -> AddSprite( nLayerId, *pSprite ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the sprite is already on layer " + std :: to_string( nLayerId ) );

                pSprite -> GetCollider().SetParent( pSprite );
                pSprite -> GetCollider().SetPtrData( PackSpriteHandleAsPtr( ( SpriteHandle ) lua_tointeger( pLuaState, 1 ) ) );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{sprite_remove_from_layer(handle, layerId) -> true | nil, message}
             * @luagroup{layers}
             */
            int LuaSpriteApi :: RemoveFromLayer( lua_State *pLuaState )  {

                std :: string  strError;
                int            nLayerId = 0;

                SunLight :: Sprite :: Sprite  *pSprite = ResolveHandleArg( pLuaState, 1, strError );

                if( pSprite == nullptr || !ReadLayerIdArg( pLuaState, 2, nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !LuaEngineUtil :: GetTileMap( pLuaState ) -> RemoveSprite( nLayerId, *pSprite ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the sprite is not on layer " + std :: to_string( nLayerId ) );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            void LuaSpriteApi :: RegisterEnums( lua_State *pLuaState )  {

                LuaEngineUtil :: RegisterConstants( pLuaState, s_aAnimationModes, sizeof( s_aAnimationModes ) / sizeof( s_aAnimationModes[0] ) );
            }

            /**
             * @brief Register the sprite pool Lua-callable functions and enums.
             */
            void LuaSpriteApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "pool_register_type", LuaSpriteApi :: RegisterType );
                lua_register( pLuaState, "sprite_acquire", LuaSpriteApi :: Acquire );
                lua_register( pLuaState, "sprite_release", LuaSpriteApi :: Release );
                lua_register( pLuaState, "sprite_configure_texture", LuaSpriteApi :: ConfigureTexture );
                lua_register( pLuaState, "sprite_set_active_sequence", LuaSpriteApi :: SetActiveSequence );
                lua_register( pLuaState, "sprite_get_active_sequence", LuaSpriteApi :: GetActiveSequence );
                lua_register( pLuaState, "sprite_set_animation_mode", LuaSpriteApi :: SetAnimationMode );
                lua_register( pLuaState, "sprite_set_visible", LuaSpriteApi :: SetVisible );
                lua_register( pLuaState, "sprite_get_visible", LuaSpriteApi :: GetVisible );
                lua_register( pLuaState, "sprite_set_world_space", LuaSpriteApi :: SetWorldSpace );
                lua_register( pLuaState, "sprite_get_world_space", LuaSpriteApi :: GetWorldSpace );
                lua_register( pLuaState, "sprite_set_blend_mode", LuaSpriteApi :: SetBlendMode );
                lua_register( pLuaState, "sprite_clear_blend_mode", LuaSpriteApi :: ClearBlendMode );
                lua_register( pLuaState, "sprite_get_blend_mode", LuaSpriteApi :: GetBlendMode );
                lua_register( pLuaState, "sprite_get_pos", LuaSpriteApi :: GetPos );
                lua_register( pLuaState, "sprite_set_pos", LuaSpriteApi :: SetPos );
                lua_register( pLuaState, "sprite_get_size", LuaSpriteApi :: GetSize );
                lua_register( pLuaState, "sprite_set_collision_inset", LuaSpriteApi :: SetCollisionInset );
                lua_register( pLuaState, "sprite_add_to_layer", LuaSpriteApi :: AddToLayer );
                lua_register( pLuaState, "sprite_remove_from_layer", LuaSpriteApi :: RemoveFromLayer );

                RegisterEnums( pLuaState );
            }
        }
    }
}
