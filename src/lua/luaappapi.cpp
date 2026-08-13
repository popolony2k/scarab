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
             * @brief Enter or leave fullscreen. Caravellius always renders
             * at it's own fixed internal resolution regardless - the
             * engine handles scaling/letterboxing to whatever the actual
             * window size ends up being, so this is purely a presentation
             * toggle, not something Lua needs to account for elsewhere.
             */
            int LuaAppApi :: SetFullscreen( lua_State *pLuaState )  {

                bool  bFullscreen = lua_toboolean( pLuaState, 1 );

                LuaEngineUtil :: GetTileMap( pLuaState ) -> SetFullscreen( bFullscreen );

                return 0;
            }

            /**
             * @brief Query whether the window is currently fullscreen (see
             * @link SetFullscreen).
             */
            int LuaAppApi :: GetFullscreen( lua_State *pLuaState )  {

                bool  bFullscreen = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetFullscreen();

                lua_pushboolean( pLuaState, bFullscreen );

                return 1;
            }

            /**
             * @brief Show or hide the on-screen FPS counter.
             */
            int LuaAppApi :: SetDrawFps( lua_State *pLuaState )  {

                bool  bDrawFPS = lua_toboolean( pLuaState, 1 );

                LuaEngineUtil :: GetTileMap( pLuaState ) -> SetDrawFPS( bDrawFPS );

                return 0;
            }

            /**
             * @brief Query whether the FPS counter is currently being drawn
             * (see @link SetDrawFps).
             */
            int LuaAppApi :: GetDrawFps( lua_State *pLuaState )  {

                bool  bDrawFPS = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetDrawFPS();

                lua_pushboolean( pLuaState, bDrawFPS );

                return 1;
            }

            /**
             * @brief Allow or disallow the user resizing the window by
             * dragging it's edges/corners. Safe to call at any time - both
             * before the window is created and, unlike the old pre-Start()
             * -only mechanism, while it's already running too (a genuine
             * live toggle, in either direction).
             */
            int LuaAppApi :: SetWindowResizeable( lua_State *pLuaState )  {

                bool  bResizeable = lua_toboolean( pLuaState, 1 );

                LuaEngineUtil :: GetTileMap( pLuaState ) -> SetWindowResizeable( bResizeable );

                return 0;
            }

            /**
             * @brief Query whether the window is currently resizeable (see
             * @link SetWindowResizeable).
             */
            int LuaAppApi :: GetWindowResizeable( lua_State *pLuaState )  {

                bool  bResizeable = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetWindowResizeable();

                lua_pushboolean( pLuaState, bResizeable );

                return 1;
            }

            /**
             * @brief Register the application-level Lua-callable functions.
             */
            void LuaAppApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "app_set_name", LuaAppApi :: SetAppName );
                lua_register( pLuaState, "screen_fade", LuaAppApi :: ScreenFade );
                lua_register( pLuaState, "app_set_fullscreen", LuaAppApi :: SetFullscreen );
                lua_register( pLuaState, "app_get_fullscreen", LuaAppApi :: GetFullscreen );
                lua_register( pLuaState, "app_set_draw_fps", LuaAppApi :: SetDrawFps );
                lua_register( pLuaState, "app_get_draw_fps", LuaAppApi :: GetDrawFps );
                lua_register( pLuaState, "app_set_window_resizeable", LuaAppApi :: SetWindowResizeable );
                lua_register( pLuaState, "app_get_window_resizeable", LuaAppApi :: GetWindowResizeable );
            }
        }
    }
}
