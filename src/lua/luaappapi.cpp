/*
 * luaappapi.cpp
 *
 *  Created on: Jul 11, 2026
 *      Author: popolony2k
 */

#include "lua/luaappapi.h"
#include "lua/luaengineutil.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief Set the application window's title. Scarab (the engine)
             * has no opinion on what a game should call itself - the window
             * is created with a generic default title (APP_NAME, "Scarab")
             * before any Lua exists, and the game's own main.lua is expected
             * to call this with its real name once it starts running.
             */
            int LuaAppApi :: SetAppName( lua_State *pLuaState )  {

                const char  *szName = lua_tostring( pLuaState, 1 );

                LuaEngineUtil :: GetTileMap( pLuaState ) -> SetWindowTitle( szName );

                return 0;
            }

            /**
             * @brief Set a whole-screen fade overlay, drawn on top of every
             * other rendered element - used for stage-start/stage-end (and
             * game-over) transitions, not any single sprite/texture effect.
             */
            int LuaAppApi :: ScreenFade( lua_State *pLuaState )  {

                float  fAlpha = ( float ) lua_tonumber( pLuaState, 1 );

                LuaEngineUtil :: GetTileMap( pLuaState ) -> SetScreenFade( fAlpha );

                return 0;
            }

            /**
             * @brief Register the application-level Lua-callable functions.
             */
            void LuaAppApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "app_set_name", LuaAppApi :: SetAppName );
                lua_register( pLuaState, "screen_fade", LuaAppApi :: ScreenFade );
            }
        }
    }
}
