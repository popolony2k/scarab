/*
 * luaengineutil.cpp
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
 */

#include "lua/luaengineutil.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            std :: mutex LuaEngineUtil :: s_LuaMutex;

            /**
             * @brief Fetch the engine's ITileMap instance stashed as a Lua light userdata global.
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            SunLight :: TileMap :: ITileMap* LuaEngineUtil :: GetTileMap( lua_State *pLuaState )  {

                lua_getglobal( pLuaState, "tileMapPtr" );

                return static_cast<SunLight :: TileMap :: ITileMap *>( lua_touserdata( pLuaState, -1 ) );
            }

            /**
             * @brief Fetch the engine's IDrawSurface instance stashed as a Lua light userdata
             * global - screen-space text/rectangle drawing and window-state primitives (see
             * sunlight's own IDrawSurface header comment), a separate pointer from
             * GetTileMap()'s since sunlight v0.12.0 split those off ITileMap. Both pointers
             * happen to address the same underlying TileMapRenderer object today (it
             * implements both interfaces), but callers should reach for whichever interface
             * actually matches what they're doing, not assume that always holds.
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            SunLight :: DrawSurface :: IDrawSurface* LuaEngineUtil :: GetDrawSurface( lua_State *pLuaState )  {

                lua_getglobal( pLuaState, "drawSurfacePtr" );

                return static_cast<SunLight :: DrawSurface :: IDrawSurface *>( lua_touserdata( pLuaState, -1 ) );
            }

            /**
             * @brief Fetch the engine's SoundManager instance stashed as a Lua light userdata global.
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            SunLight :: Sound :: SoundManager* LuaEngineUtil :: GetSoundManager( lua_State *pLuaState )  {

                lua_getglobal( pLuaState, "soundManagerPtr" );

                return static_cast<SunLight :: Sound :: SoundManager *>( lua_touserdata( pLuaState, -1 ) );
            }

            /**
             * @brief Fetch the engine's SpritePool instance stashed as a Lua light userdata global.
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            Engine :: SpritePool* LuaEngineUtil :: GetSpritePool( lua_State *pLuaState )  {

                lua_getglobal( pLuaState, "spritePoolPtr" );

                return static_cast<Engine :: SpritePool *>( lua_touserdata( pLuaState, -1 ) );
            }

            /**
             * @brief Fetch the engine's ScriptProcessor instance stashed as a Lua light userdata global.
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            SunLight :: Scripting :: ScriptProcessor* LuaEngineUtil :: GetScriptProcessor( lua_State *pLuaState )  {

                lua_getglobal( pLuaState, "scriptProcessorPtr" );

                return static_cast<SunLight :: Scripting :: ScriptProcessor *>( lua_touserdata( pLuaState, -1 ) );
            }

            /**
             * @brief Add an one parameter command to the ScriptProcessor's queue;
             * shared by every Lua primitive that queues a command rather than
             * acting immediately (sp_wait, sp_move_sprites_to_screen,
             * sp_play_song, ...).
             *
             * @param pLuaState Lua state to be used by engine call.
             * @param cmd The command to add to queue;
             */
            void LuaEngineUtil :: AddOneParmCommandScript( lua_State *pLuaState, SunLight :: Scripting :: Commands cmd )  {

                if( lua_gettop( pLuaState ) == 1 )  {

                    lua_Integer nParm = lua_tonumber( pLuaState, 1 );

                    GetScriptProcessor( pLuaState ) -> AddOneParmCmd( cmd, ( uint16_t ) nParm );
                }
            }

            /**
             * @brief Register a table of named integer constants as Lua globals.
             *
             * @param pLuaState Lua state to be used by engine call.
             * @param pTable The constants table to register;
             * @param nCount Number of entries in the table;
             */
            void LuaEngineUtil :: RegisterConstants( lua_State *pLuaState, const stNamedConstant *pTable, size_t nCount )  {

                for( size_t nIndex = 0; nIndex < nCount; nIndex++ )  {
                    lua_pushinteger( pLuaState, pTable[nIndex].nValue );
                    lua_setglobal( pLuaState, pTable[nIndex].szName );
                }
            }
        }
    }
}
