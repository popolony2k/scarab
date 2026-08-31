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
#include "engine/spritehandle.h"
#include <cstdint>

extern "C"
{
  #include "lauxlib.h"
}

using namespace SunLight :: Canvas;

#define __CONST( name )  { #name, ( int ) name }


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @luaname{pool_register_type(typeTag, capacity) -> success}
             * @luadoc
             * Reserve `capacity` slots for sprites acquired with this
             * `typeTag` string. Must be called once before the first
             * `sprite_acquire` for that tag.
             */
            int LuaSpriteApi :: RegisterType( lua_State *pLuaState )  {

                const char  *szTypeTag = lua_tostring( pLuaState, 1 );
                uint32_t    nCapacity  = ( uint32_t ) lua_tointeger( pLuaState, 2 );
                bool        bResult   = LuaEngineUtil :: GetSpritePool( pLuaState ) -> RegisterType( szTypeTag, nCapacity );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @luaname{sprite_acquire(typeTag) -> handle}
             * @luadoc
             * Get a free handle from the pool registered for `typeTag`.
             * Returns `0` if the pool is exhausted (every slot currently
             * in use).
             */
            int LuaSpriteApi :: Acquire( lua_State *pLuaState )  {

                const char    *szTypeTag = lua_tostring( pLuaState, 1 );
                SpriteHandle  handle     = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Acquire( szTypeTag );

                lua_pushinteger( pLuaState, ( lua_Integer ) handle );

                return 1;
            }

            /**
             * @luaname{sprite_release(handle) -> success}
             * @luadoc
             * Return a handle to its pool, making the slot available for
             * a future `sprite_acquire`. **Call
             * `sprite_remove_from_layer` first** if the sprite was ever
             * added to a layer (see the gotcha above).
             */
            int LuaSpriteApi :: Release( lua_State *pLuaState )  {

                SpriteHandle  handle  = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                bool          bResult = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Release( handle );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @brief Load a texture sequence's file and configure it's tiling/
             * animation, mirroring the old WorldBase::LoadSprite per-texture body.
             *
             * @luaname{sprite_configure_texture(handle, sequenceId, path, framesByTexture, activeTileIndex, animationMode, delayMilli?) -> success}
             * @luadoc
             * Load a texture file and configure it as animation
             * `sequenceId` on `handle`. `path` points at a texture sheet
             * containing `framesByTexture` equal-width frames laid out
             * horizontally; `activeTileIndex` is which frame to start on.
             * `delayMilli` (optional, defaults to `-1`) controls
             * automatic switching to a *different* texture later added to
             * this same sequence — pass `-1` to disable that switching
             * entirely, which is what every sprite in Caravellius does
             * today (each sequence holds exactly one texture). It's
             * unrelated to the per-frame tile animation within that one
             * texture, which `animationMode` controls.
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

                SpriteHandle  handle           = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                int           nSequenceId      = ( int ) lua_tointeger( pLuaState, 2 );
                const char    *szPath          = lua_tostring( pLuaState, 3 );
                int           nFramesByTexture = ( int ) lua_tointeger( pLuaState, 4 );
                int           nActiveTileIndex = ( int ) lua_tointeger( pLuaState, 5 );
                int           nAnimationMode   = ( int ) lua_tointeger( pLuaState, 6 );
                int           nDelayMilli      = ( int ) luaL_optinteger( pLuaState, 7, -1 );

                SpritePool                *pPool    = LuaEngineUtil :: GetSpritePool( pLuaState );
                SunLight :: Sprite :: Sprite  *pSprite  = pPool -> Resolve( handle );
                TextureCanvas             *pTexture = pPool -> GetOrCreateTexture( handle, nSequenceId );

                if( ( pSprite == nullptr ) || ( pTexture == nullptr ) )  {
                    lua_pushboolean( pLuaState, false );

                    return 1;
                }

                /*
                 * A recycled pool slot's TextureCanvas may already hold a
                 * previously loaded texture (e.g. a prior life of this same
                 * slot) - unload it first or the old GPU texture handle leaks
                 * every time this sequence gets reconfigured.
                 */
                pTexture -> Unload();

                if( !pTexture -> Load( szPath ) )  {
                    lua_pushboolean( pLuaState, false );

                    return 1;
                }

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
                int  nTextureWidth = nNativeWidth / nFramesByTexture;

                /*
                 * Same caching for height (not frame-divided, but equally
                 * unreliable to re-read post-aliasing) so SetActiveSequence can
                 * sync it correctly even for sequences whose height genuinely
                 * differs from each other (not the case for Satellite's own
                 * assets today, but will be for other enemy types).
                 */
                pPool -> GetOrCacheNativeHeight( handle, nSequenceId, pTexture -> GetDimension2D().size.nHeight );

                pTexture -> SetCenterTileIndex( nActiveTileIndex );
                pTexture -> SetActiveTileIndex( nActiveTileIndex );
                pTexture -> SetTileSize( nTextureWidth );
                pTexture -> SetAnimationMode( ( AnimationMode ) nAnimationMode );

                pSprite -> AddTextureSequence( nSequenceId, pTexture, nDelayMilli );
                pSprite -> SetVisible( true );

                lua_pushboolean( pLuaState, true );

                return 1;
            }

            /**
             * @brief Set the sprite's active texture sequence, also syncing it's
             * bounding-box width/height from that sequence's cached natural
             * size - a step callers had to remember to do separately in the old
             * C++ path (and the old path never had to worry about a recycled,
             * reconfigured texture's size becoming unreliable to re-read).
             *
             * @luaname{sprite_set_active_sequence(handle, sequenceId) -> success}
             * @luadoc
             * Switch which configured sequence is currently
             * displayed/collided against (also syncs the sprite's
             * bounding box to that sequence's size — important since a
             * "main" and an "explosion" sequence are often different
             * sizes).
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

                SpriteHandle  handle      = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                int           nSequenceId = ( int ) lua_tointeger( pLuaState, 2 );

                SpritePool                    *pPool    = LuaEngineUtil :: GetSpritePool( pLuaState );
                SunLight :: Sprite :: Sprite  *pSprite  = pPool -> Resolve( handle );
                TextureCanvas                 *pTexture = pPool -> GetTexture( handle, nSequenceId );

                if( ( pSprite == nullptr ) || ( pTexture == nullptr ) )  {
                    lua_pushboolean( pLuaState, false );

                    return 1;
                }

                bool  bResult = pSprite -> SetActiveTextureSequence( nSequenceId );

                if( bResult )  {
                    pSprite -> GetDimension2D().size.nWidth  = pTexture -> GetTileSize();
                    pSprite -> GetDimension2D().size.nHeight = pPool -> GetOrCacheNativeHeight( handle, nSequenceId, 0 );
                }

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @brief Change a texture sequence's animation mode after it's
             * already been configured - needed by sprites (eg. the player ship)
             * that switch between ANIMATE_LEFT/RIGHT/CENTER at runtime in
             * response to input, unlike enemies which set their mode once at
             * ConfigureTexture time and never touch it again.
             *
             * @luaname{sprite_set_animation_mode(handle, sequenceId, mode) -> success}
             * @luadoc
             * Change a sequence's animation mode *after* it's already
             * configured — for sprites that switch modes at runtime
             * (e.g. the player ship banking left/right based on input).
             * Enemies that only ever set their mode once typically just
             * pass the final mode to `sprite_configure_texture` and never
             * call this.
             * @luaexample
             * sprite_set_animation_mode(handle, 0, TEXTURE_ANIMATION_MODE_ANIMATE_LEFT)
             */
            int LuaSpriteApi :: SetAnimationMode( lua_State *pLuaState )  {

                SpriteHandle  handle      = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                int           nSequenceId = ( int ) lua_tointeger( pLuaState, 2 );
                int           nMode       = ( int ) lua_tointeger( pLuaState, 3 );

                TextureCanvas  *pTexture = LuaEngineUtil :: GetSpritePool( pLuaState ) -> GetTexture( handle, nSequenceId );

                if( pTexture == nullptr )  {
                    lua_pushboolean( pLuaState, false );

                    return 1;
                }

                pTexture -> SetAnimationMode( ( AnimationMode ) nMode );

                lua_pushboolean( pLuaState, true );

                return 1;
            }

            /**
             * @luaname{sprite_get_active_sequence(handle) -> sequenceId}
             */
            int LuaSpriteApi :: GetActiveSequence( lua_State *pLuaState )  {

                SpriteHandle  handle  = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                SunLight :: Sprite :: Sprite  *pSprite = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Resolve( handle );

                if( pSprite == nullptr )  {
                    lua_pushnil( pLuaState );

                    return 1;
                }

                lua_pushinteger( pLuaState, pSprite -> GetActiveTextureSequence() );

                return 1;
            }

            /**
             * @luaname{sprite_set_visible(handle, visible)}
             * @luagroup{visibility}
             * @luaheading{Visibility}
             * @luaexample
             * sprite_set_visible(handle, visible)
             * sprite_get_visible(handle) -> visible
             */
            int LuaSpriteApi :: SetVisible( lua_State *pLuaState )  {

                SpriteHandle  handle   = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                bool          bVisible = lua_toboolean( pLuaState, 2 );
                SunLight :: Sprite :: Sprite  *pSprite = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Resolve( handle );

                if( pSprite != nullptr )
                    pSprite -> SetVisible( bVisible );

                return 0;
            }

            /**
             * @luaname{sprite_get_visible(handle) -> visible}
             * @luagroup{visibility}
             */
            int LuaSpriteApi :: GetVisible( lua_State *pLuaState )  {

                SpriteHandle  handle  = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                SunLight :: Sprite :: Sprite  *pSprite = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Resolve( handle );

                lua_pushboolean( pLuaState, ( pSprite != nullptr ) && pSprite -> GetVisible() );

                return 1;
            }

            /**
             * @luaname{sprite_get_pos(handle) -> x, y}
             * @luagroup{position_size}
             * @luaheading{Position and size}
             * @luadoc
             * Position is **top-left anchored**, not center-anchored —
             * `x, y` is the sprite's top-left corner, matching both the
             * render clip rect and the AABB collision math. `pos -
             * height/2` means "above the sprite's own top edge," not
             * "above center."
             * @luaexample
             * sprite_get_pos(handle) -> x, y
             * sprite_set_pos(handle, x, y)
             * sprite_get_size(handle) -> width, height
             */
            int LuaSpriteApi :: GetPos( lua_State *pLuaState )  {

                SpriteHandle  handle  = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                SunLight :: Sprite :: Sprite  *pSprite = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Resolve( handle );

                if( pSprite == nullptr )  {
                    lua_pushnil( pLuaState );

                    return 1;
                }

                SunLight :: TileMap :: stCoordinate2D&  pos = pSprite -> GetDimension2D().pos;

                lua_pushinteger( pLuaState, pos.x );
                lua_pushinteger( pLuaState, pos.y );

                return 2;
            }

            /**
             * @luaname{sprite_set_pos(handle, x, y)}
             * @luagroup{position_size}
             */
            int LuaSpriteApi :: SetPos( lua_State *pLuaState )  {

                SpriteHandle  handle = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                int           nX     = ( int ) lua_tointeger( pLuaState, 2 );
                int           nY     = ( int ) lua_tointeger( pLuaState, 3 );
                SunLight :: Sprite :: Sprite  *pSprite = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Resolve( handle );

                if( pSprite != nullptr )  {
                    pSprite -> GetDimension2D().pos.x = nX;
                    pSprite -> GetDimension2D().pos.y = nY;
                }

                return 0;
            }

            /**
             * @luaname{sprite_get_size(handle) -> width, height}
             * @luagroup{position_size}
             */
            int LuaSpriteApi :: GetSize( lua_State *pLuaState )  {

                SpriteHandle  handle  = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                SunLight :: Sprite :: Sprite  *pSprite = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Resolve( handle );

                if( pSprite == nullptr )  {
                    lua_pushnil( pLuaState );

                    return 1;
                }

                SunLight :: TileMap :: stSize2D&  size = pSprite -> GetDimension2D().size;

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
             * @luaname{sprite_set_collision_inset(handle, leftPct, topPct, rightPct, bottomPct)}
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
             * sprite or it's tracked position.
             * @luaexample
             * sprite_set_collision_inset(handle, leftPct, topPct, rightPct, bottomPct)
             */
            int LuaSpriteApi :: SetCollisionInset( lua_State *pLuaState )  {

                SpriteHandle  handle    = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                float         fLeftPct  = ( float ) lua_tonumber( pLuaState, 2 );
                float         fTopPct   = ( float ) lua_tonumber( pLuaState, 3 );
                float         fRightPct = ( float ) lua_tonumber( pLuaState, 4 );
                float         fBottomPct = ( float ) lua_tonumber( pLuaState, 5 );
                SunLight :: Sprite :: Sprite  *pSprite = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Resolve( handle );

                if( pSprite != nullptr )  {
                    pSprite -> GetCollider().SetInset( fLeftPct, fTopPct, fRightPct, fBottomPct );
                }

                return 0;
            }

            /**
             * @brief Add the sprite to a Tiled layer, restoring it's collider
             * parent and stashing the opaque handle on the collider so a future
             * collision callback can decode which sprite collided without
             * exposing raw pointers to Lua.
             *
             * @luaname{sprite_add_to_layer(handle, layerId) -> success}
             * @luagroup{layers}
             * @luaheading{Layers}
             * @luadoc
             * Adding a sprite to a layer is also what registers it for
             * collision detection on that layer (see [collision.md](collision.md)) — a
             * sprite never added to any layer never collides with
             * anything, regardless of `collision_add_rule`.
             * @luaexample
             * sprite_add_to_layer(handle, layerId) -> success
             * sprite_remove_from_layer(handle, layerId) -> success
             */
            int LuaSpriteApi :: AddToLayer( lua_State *pLuaState )  {

                SpriteHandle  handle   = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                int           nLayerId = ( int ) lua_tointeger( pLuaState, 2 );
                SunLight :: Sprite :: Sprite  *pSprite = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Resolve( handle );

                if( pSprite == nullptr )  {
                    lua_pushboolean( pLuaState, false );

                    return 1;
                }

                bool  bResult = LuaEngineUtil :: GetTileMap( pLuaState ) -> AddSprite( nLayerId, *pSprite );

                if( bResult )  {
                    pSprite -> GetCollider().SetParent( pSprite );
                    pSprite -> GetCollider().SetPtrData( PackSpriteHandleAsPtr( handle ) );
                }

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @luaname{sprite_remove_from_layer(handle, layerId) -> success}
             * @luagroup{layers}
             */
            int LuaSpriteApi :: RemoveFromLayer( lua_State *pLuaState )  {

                SpriteHandle  handle   = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                int           nLayerId = ( int ) lua_tointeger( pLuaState, 2 );
                SunLight :: Sprite :: Sprite  *pSprite = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Resolve( handle );

                if( pSprite == nullptr )  {
                    lua_pushboolean( pLuaState, false );

                    return 1;
                }

                lua_pushboolean( pLuaState, LuaEngineUtil :: GetTileMap( pLuaState ) -> RemoveSprite( nLayerId, *pSprite ) );

                return 1;
            }

            void LuaSpriteApi :: RegisterEnums( lua_State *pLuaState )  {

                static const stNamedConstant  s_aAnimationModes[] = {
                    __CONST( TEXTURE_ANIMATION_MODE_MANUAL ),
                    __CONST( TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR ),
                    __CONST( TEXTURE_ANIMATION_MODE_AUTOMATIC_RIGHT_LEFT ),
                    __CONST( TEXTURE_ANIMATION_MODE_ANIMATE_LEFT ),
                    __CONST( TEXTURE_ANIMATION_MODE_ANIMATE_RIGHT ),
                    __CONST( TEXTURE_ANIMATION_MODE_ANIMATE_CENTER ),
                };

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
