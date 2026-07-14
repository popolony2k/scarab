/*
 * luaspriteapi.cpp
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
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

            int LuaSpriteApi :: RegisterType( lua_State *pLuaState )  {

                const char  *szTypeTag = lua_tostring( pLuaState, 1 );
                uint32_t    nCapacity  = ( uint32_t ) lua_tointeger( pLuaState, 2 );
                bool        bResult   = LuaEngineUtil :: GetSpritePool( pLuaState ) -> RegisterType( szTypeTag, nCapacity );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaSpriteApi :: Acquire( lua_State *pLuaState )  {

                const char    *szTypeTag = lua_tostring( pLuaState, 1 );
                SpriteHandle  handle     = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Acquire( szTypeTag );

                lua_pushinteger( pLuaState, ( lua_Integer ) handle );

                return 1;
            }

            int LuaSpriteApi :: Release( lua_State *pLuaState )  {

                SpriteHandle  handle  = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                bool          bResult = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Release( handle );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @brief Load a texture sequence's file and configure it's tiling/
             * animation, mirroring the old WorldBase::LoadSprite per-texture body.
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

            int LuaSpriteApi :: SetVisible( lua_State *pLuaState )  {

                SpriteHandle  handle   = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                bool          bVisible = lua_toboolean( pLuaState, 2 );
                SunLight :: Sprite :: Sprite  *pSprite = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Resolve( handle );

                if( pSprite != nullptr )
                    pSprite -> SetVisible( bVisible );

                return 0;
            }

            int LuaSpriteApi :: GetVisible( lua_State *pLuaState )  {

                SpriteHandle  handle  = ( SpriteHandle ) lua_tointeger( pLuaState, 1 );
                SunLight :: Sprite :: Sprite  *pSprite = LuaEngineUtil :: GetSpritePool( pLuaState ) -> Resolve( handle );

                lua_pushboolean( pLuaState, ( pSprite != nullptr ) && pSprite -> GetVisible() );

                return 1;
            }

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
             * @brief Add the sprite to a Tiled layer, restoring it's collider
             * parent and stashing the opaque handle on the collider so a future
             * collision callback can decode which sprite collided without
             * exposing raw pointers to Lua.
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
                lua_register( pLuaState, "sprite_add_to_layer", LuaSpriteApi :: AddToLayer );
                lua_register( pLuaState, "sprite_remove_from_layer", LuaSpriteApi :: RemoveFromLayer );

                RegisterEnums( pLuaState );
            }
        }
    }
}
