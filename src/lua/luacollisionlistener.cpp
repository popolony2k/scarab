/*
 * luacollisionlistener.cpp
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
 */

#include "lua/luacollisionlistener.h"
#include "engine/spritehandle.h"
#include "tilemap/tilemapdefs.h"

extern "C"
{
  #include "lauxlib.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief Constructor. Initialize all class data.
             *
             * @param pLuaState Lua state used to invoke the registered handlers;
             */
            LuaCollisionListener :: LuaCollisionListener( lua_State *pLuaState )  {

                m_pLuaState = pLuaState;
            }

            /**
             * @brief Destructor. Finalize all class data.
             */
            LuaCollisionListener :: ~LuaCollisionListener( void )  {

            }

            /**
             * @brief Event thrown when two colliders hit one another. Only
             * processes the collision when BOTH sides are Lua-owned - true for
             * every sprite in the game now (every enemy type and the player
             * ship are Lua/SpritePool-owned; the legacy stSpriteData-based
             * creation path that could produce a non-Lua side was deleted in
             * Phase 9). The check is kept as a defensive guard rather than
             * assumed away.
             */
            void LuaCollisionListener :: OnCollision( SunLight :: Collision :: Collider *pFirst,
                                                      SunLight :: Collision :: Collider *pSecond )  {

                void  *pFirstData  = pFirst -> GetPtrData();
                void  *pSecondData = pSecond -> GetPtrData();
                bool  bFirstIsLua  = IsPackedSpriteHandle( pFirstData );
                bool  bSecondIsLua = IsPackedSpriteHandle( pSecondData );

                if( !( bFirstIsLua && bSecondIsLua ) )
                    return;

                SpriteHandle  handleA = UnpackSpriteHandleFromPtr( pFirstData );
                SpriteHandle  handleB = UnpackSpriteHandleFromPtr( pSecondData );

                lua_getglobal( m_pLuaState, "__collision_handler" );

                if( !lua_isfunction( m_pLuaState, -1 ) )  {
                    lua_pop( m_pLuaState, 1 );

                    return;
                }

                lua_pushinteger( m_pLuaState, ( lua_Integer ) handleA );
                lua_pushinteger( m_pLuaState, ( lua_Integer ) handleB );

                if( lua_pcall( m_pLuaState, 2, 0, 0 ) != 0 )  {
                    fprintf( stderr, "Error calling collision handler: %s\n", lua_tostring( m_pLuaState, -1 ) );
                    lua_pop( m_pLuaState, 1 );
                }
            }

            /**
             * @brief Event thrown when a collider hits a tile. Same Lua-owned
             * check as the collider-to-collider overload, dispatching to the
             * handler registered via collision_set_tile_handler.
             */
            void LuaCollisionListener :: OnCollision( SunLight :: Collision :: Collider *pFirst,
                                                      SunLight :: TileMap :: stTile *pSecond )  {

                void  *pFirstData = pFirst -> GetPtrData();

                if( !IsPackedSpriteHandle( pFirstData ) )
                    return;

                SpriteHandle  handle = UnpackSpriteHandleFromPtr( pFirstData );

                lua_getglobal( m_pLuaState, "__collision_tile_handler" );

                if( !lua_isfunction( m_pLuaState, -1 ) )  {
                    lua_pop( m_pLuaState, 1 );

                    return;
                }

                lua_pushinteger( m_pLuaState, ( lua_Integer ) handle );
                lua_pushinteger( m_pLuaState, ( lua_Integer ) pSecond -> nGID );
                lua_pushinteger( m_pLuaState, pSecond -> dimension.pos.x );
                lua_pushinteger( m_pLuaState, pSecond -> dimension.pos.y );
                lua_pushinteger( m_pLuaState, pSecond -> dimension.size.nWidth );
                lua_pushinteger( m_pLuaState, pSecond -> dimension.size.nHeight );

                if( lua_pcall( m_pLuaState, 5, 0, 0 ) != 0 )  {
                    fprintf( stderr, "Error calling collision tile handler: %s\n", lua_tostring( m_pLuaState, -1 ) );
                    lua_pop( m_pLuaState, 1 );
                }
            }
        }
    }
}
