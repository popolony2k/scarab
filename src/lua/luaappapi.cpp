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

#include "lua/luaappapi.h"
#include "lua/luaengineutil.h"

extern "C"
{
  #include "lauxlib.h"
}


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

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> SetWindowTitle( szName );

                return 0;
            }

            /**
             * @brief Set a whole-screen fade overlay, drawn on top of every
             * other rendered element - used for stage-start/stage-end (and
             * game-over) transitions, not any single sprite/texture effect.
             */
            int LuaAppApi :: ScreenFade( lua_State *pLuaState )  {

                float  fAlpha = ( float ) lua_tonumber( pLuaState, 1 );

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> SetScreenFade( fAlpha );

                return 0;
            }

            /**
             * @brief Enter or leave fullscreen. A game always renders at
             * it's own fixed internal resolution regardless - the engine
             * handles scaling/letterboxing to whatever the actual window
             * size ends up being, so this is purely a presentation toggle,
             * not something Lua needs to account for elsewhere.
             *
             * strategy (optional, second argument) picks which of
             * IEngine::FullscreenStrategy's two strategies to enter
             * fullscreen with - FULLSCREEN_STRATEGY_REAL (a genuine OS-
             * level fullscreen space, sunlight's own default as of
             * v0.14.0, fixing a real macOS Dock-overlap bug found live in a
             * game built on this engine - see the memory/commit history for
             * the story)
             * or FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED (the older
             * ordinary-window-resized-to-native-resolution behavior, kept
             * as a fallback for any platform/window manager where a true
             * video-mode switch misbehaves). Defaults to
             * FULLSCREEN_STRATEGY_REAL when omitted, matching
             * IEngine::SetFullscreen's own C++-side default - a Lua call
             * site that only ever passed the bool (every one, before this
             * parameter existed) keeps behaving exactly as before.
             * Ignored when bFullscreen is false. Switching strategy while
             * already fullscreen in the OTHER one is unsupported (see
             * IEngine::SetFullscreen's own doc comment) - call
             * app_set_fullscreen(false) first, then re-enter fullscreen
             * with the new strategy.
             */
            int LuaAppApi :: SetFullscreen( lua_State *pLuaState )  {

                bool  bFullscreen = lua_toboolean( pLuaState, 1 );
                int   nStrategy   = ( int ) luaL_optinteger( pLuaState, 2, SunLight :: Engines :: IEngine :: FULLSCREEN_STRATEGY_REAL );

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> SetFullscreen( bFullscreen,
                    ( SunLight :: Engines :: IEngine :: FullscreenStrategy ) nStrategy );

                return 0;
            }

            /**
             * @brief Query whether the window is currently fullscreen (see
             * @link SetFullscreen).
             */
            int LuaAppApi :: GetFullscreen( lua_State *pLuaState )  {

                bool  bFullscreen = LuaEngineUtil :: GetDrawSurface( pLuaState ) -> GetFullscreen();

                lua_pushboolean( pLuaState, bFullscreen );

                return 1;
            }

            /**
             * @brief Set the renderer's own target frame rate - the cap
             * the game loop paces itself against, not the current
             * measured FPS (see @link GetDrawFps/app_get_draw_fps for the
             * on-screen counter, which shows the latter). Safe to call at
             * any time - a live change takes effect immediately.
             */
            int LuaAppApi :: SetTargetFps( lua_State *pLuaState )  {

                int  nTargetFps = ( int ) lua_tointeger( pLuaState, 1 );

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> SetTargetFPS( nTargetFps );

                return 0;
            }

            /**
             * @brief Query the currently configured target FPS (see
             * @link SetTargetFps) - NOT the current measured FPS.
             */
            int LuaAppApi :: GetTargetFps( lua_State *pLuaState )  {

                int  nTargetFps = LuaEngineUtil :: GetDrawSurface( pLuaState ) -> GetTargetFPS();

                lua_pushinteger( pLuaState, nTargetFps );

                return 1;
            }

            /**
             * @brief Show or hide the on-screen FPS counter.
             */
            int LuaAppApi :: SetDrawFps( lua_State *pLuaState )  {

                bool  bDrawFPS = lua_toboolean( pLuaState, 1 );

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> SetDrawFPS( bDrawFPS );

                return 0;
            }

            /**
             * @brief Query whether the FPS counter is currently being drawn
             * (see @link SetDrawFps).
             */
            int LuaAppApi :: GetDrawFps( lua_State *pLuaState )  {

                bool  bDrawFPS = LuaEngineUtil :: GetDrawSurface( pLuaState ) -> GetDrawFPS();

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

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> SetWindowResizeable( bResizeable );

                return 0;
            }

            /**
             * @brief Query whether the window is currently resizeable (see
             * @link SetWindowResizeable).
             */
            int LuaAppApi :: GetWindowResizeable( lua_State *pLuaState )  {

                bool  bResizeable = LuaEngineUtil :: GetDrawSurface( pLuaState ) -> GetWindowResizeable();

                lua_pushboolean( pLuaState, bResizeable );

                return 1;
            }

            /**
             * @brief Choose how the fixed-internal-resolution render
             * target is blitted onto the real window/screen when their
             * sizes differ - see IDrawSurface::SetStretchToFill's own doc
             * comment for the full behavior (letterbox vs. stretch-fill).
             */
            int LuaAppApi :: SetStretchToFill( lua_State *pLuaState )  {

                bool  bStretchToFill = lua_toboolean( pLuaState, 1 );

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> SetStretchToFill( bStretchToFill );

                return 0;
            }

            /**
             * @brief Query whether the render target is currently being
             * stretched to fill (see @link SetStretchToFill).
             */
            int LuaAppApi :: GetStretchToFill( lua_State *pLuaState )  {

                bool  bStretchToFill = LuaEngineUtil :: GetDrawSurface( pLuaState ) -> GetStretchToFill();

                lua_pushboolean( pLuaState, bStretchToFill );

                return 1;
            }

            /**
             * @brief Draw a filled, solid-color rectangle in screen space
             * (see IDrawSurface::DrawFilledRectangle's own doc comment) -
             * meant for simple HUD elements (a progress/health bar) that
             * don't warrant a whole sprite/texture asset. Same screen-
             * space coordinate system as draw_text/screen_get_width -
             * independent of the live window size/fullscreen/stretch-to-
             * fill state, same as every other screen-space draw call.
             */
            int LuaAppApi :: DrawFilledRectangle( lua_State *pLuaState )  {

                int            nPosX  = ( int ) lua_tointeger( pLuaState, 1 );
                int            nPosY  = ( int ) lua_tointeger( pLuaState, 2 );
                int            nWidth  = ( int ) lua_tointeger( pLuaState, 3 );
                int            nHeight = ( int ) lua_tointeger( pLuaState, 4 );
                unsigned char  nRed    = ( unsigned char ) lua_tointeger( pLuaState, 5 );
                unsigned char  nGreen  = ( unsigned char ) lua_tointeger( pLuaState, 6 );
                unsigned char  nBlue   = ( unsigned char ) lua_tointeger( pLuaState, 7 );
                unsigned char  nAlpha  = ( unsigned char ) lua_tointeger( pLuaState, 8 );

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> DrawFilledRectangle( nPosX, nPosY, nWidth, nHeight,
                                                                                  nRed, nGreen, nBlue, nAlpha );

                return 0;
            }

            /**
             * @brief Register app_set_fullscreen's own optional strategy
             * argument (see @link SetFullscreen) as Lua globals, same
             * names as the underlying SunLight::Engines::IEngine enum -
             * mirrors LuaTilemapApi::RegisterEnums's own MAP_ALIGNMENT_*
             * pattern for a small, engine-defined enum exposed to Lua.
             */
            void LuaAppApi :: RegisterEnums( lua_State *pLuaState )  {

                static const stNamedConstant  s_aFullscreenStrategies[] = {
                    { "FULLSCREEN_STRATEGY_REAL", SunLight :: Engines :: IEngine :: FULLSCREEN_STRATEGY_REAL },
                    { "FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED", SunLight :: Engines :: IEngine :: FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED },
                };

                LuaEngineUtil :: RegisterConstants( pLuaState, s_aFullscreenStrategies,
                    sizeof( s_aFullscreenStrategies ) / sizeof( s_aFullscreenStrategies[0] ) );
            }

            /**
             * @brief Register the application-level Lua-callable functions.
             */
            void LuaAppApi :: Register( lua_State *pLuaState )  {

                RegisterEnums( pLuaState );

                lua_register( pLuaState, "app_set_name", LuaAppApi :: SetAppName );
                lua_register( pLuaState, "screen_fade", LuaAppApi :: ScreenFade );
                lua_register( pLuaState, "app_set_fullscreen", LuaAppApi :: SetFullscreen );
                lua_register( pLuaState, "app_get_fullscreen", LuaAppApi :: GetFullscreen );
                lua_register( pLuaState, "app_set_target_fps", LuaAppApi :: SetTargetFps );
                lua_register( pLuaState, "app_get_target_fps", LuaAppApi :: GetTargetFps );
                lua_register( pLuaState, "app_set_draw_fps", LuaAppApi :: SetDrawFps );
                lua_register( pLuaState, "app_get_draw_fps", LuaAppApi :: GetDrawFps );
                lua_register( pLuaState, "app_set_window_resizeable", LuaAppApi :: SetWindowResizeable );
                lua_register( pLuaState, "app_get_window_resizeable", LuaAppApi :: GetWindowResizeable );
                lua_register( pLuaState, "app_set_stretch_to_fill", LuaAppApi :: SetStretchToFill );
                lua_register( pLuaState, "app_get_stretch_to_fill", LuaAppApi :: GetStretchToFill );
                lua_register( pLuaState, "draw_filled_rectangle", LuaAppApi :: DrawFilledRectangle );
            }
        }
    }
}
