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

#include "lua/luaengineutil.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            std :: mutex LuaEngineUtil :: s_LuaMutex;

            /**
             * @brief Fetch the renderer provider stashed as a Lua light userdata global - the
             * one object every renderer-needing primitive reaches the renderer through, since
             * the renderer does not exist yet when the Lua engine is built (see
             * IRendererProvider).
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            Engine :: IRendererProvider* LuaEngineUtil :: GetRendererProvider( lua_State *pLuaState )  {

                lua_getglobal( pLuaState, "rendererProviderPtr" );

                Engine :: IRendererProvider  *pProvider = static_cast<Engine :: IRendererProvider *>( lua_touserdata( pLuaState, -1 ) );

                lua_pop( pLuaState, 1 );

                return pProvider;
            }

            /**
             * @brief Name of the Lua-callable primitive currently running (as the script called
             * it, eg. "app_set_name") - recorded by the provider when this call is what creates
             * the default renderer, so a later renderer_create can name the culprit.
             */
            static std :: string GetCallerName( lua_State *pLuaState )  {

                lua_Debug  ar;

                if( lua_getstack( pLuaState, 0, &ar ) && lua_getinfo( pLuaState, "n", &ar ) && ar.name )
                    return ar.name;

                return "(unknown primitive)";
            }

            /**
             * @brief Make sure a renderer exists, creating the default one (and opening its
             * window) if the script hasn't - for primitives that need a live window/GL context
             * but don't go through GetTileMap()/GetDrawSurface() (eg. sprite_configure_texture,
             * which loads a GPU texture).
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            void LuaEngineUtil :: EnsureRenderer( lua_State *pLuaState )  {

                GetRendererProvider( pLuaState ) -> GetTileMap( GetCallerName( pLuaState ).c_str() );
            }

            /**
             * @brief The engine's ITileMap - creating the default renderer first if the script
             * has not created one yet (see IRendererProvider::GetTileMap).
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            SunLight :: TileMap :: ITileMap* LuaEngineUtil :: GetTileMap( lua_State *pLuaState )  {

                return GetRendererProvider( pLuaState ) -> GetTileMap( GetCallerName( pLuaState ).c_str() );
            }

            /**
             * @brief The engine's IDrawSurface - screen-space text/rectangle drawing and
             * window-state primitives (see sunlight's own IDrawSurface header comment), a
             * separate interface from GetTileMap()'s since sunlight v0.12.0 split those off
             * ITileMap. Both address the same underlying TileMapRenderer object today (it
             * implements both), but callers should reach for whichever interface actually
             * matches what they're doing, not assume that always holds. Creates the default
             * renderer first if the script has not created one yet.
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            SunLight :: DrawSurface :: IDrawSurface* LuaEngineUtil :: GetDrawSurface( lua_State *pLuaState )  {

                return GetRendererProvider( pLuaState ) -> GetDrawSurface( GetCallerName( pLuaState ).c_str() );
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
