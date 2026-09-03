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
             *
             * @luaname{app_set_name(name)}
             * @luadoc
             * Set the application window's title. Scarab (the engine) has no
             * opinion on what a game calls itself — the window opens with a
             * generic default title (`"Scarab"`, `main.h`'s `APP_NAME`)
             * before any Lua exists to override it, since the window has to
             * exist before `main.lua`'s first line runs. Call this once,
             * early, with the game's real name:
             * @luaexample
             * -- main.lua
             * app_set_name( "Caravellius" )
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
             *
             * @luaname{screen_fade(alpha)}
             * @luadoc
             * Set a whole-screen fade overlay, drawn on top of every other
             * rendered element (tilemap, sprites, everything) — not a
             * per-texture effect. `alpha` ranges from `0.0` (fully visible,
             * no overlay) to `1.0` (fully black). Intended for
             * stage-start/stage-end and game-over transitions: ramp it over
             * several frames from `on_update` to fade in/out rather than
             * jumping straight to an endpoint.
             * @luaexample
             * -- fade in from black over ~1 second at 60fps
             * local __FADE_STEP = 1 / 60
             * local fade_alpha = 1.0
             *
             * function on_update( dt )
             *     if fade_alpha > 0 then
             *         fade_alpha = math.max( 0, fade_alpha - __FADE_STEP )
             *         screen_fade( fade_alpha )
             *     end
             * end
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
             *
             * @luaname{app_set_fullscreen(fullscreen, strategy)}
             * @luagroup{fullscreen}
             * @luadoc
             * Enter/leave fullscreen, or query the current state. The game
             * always renders at it's own fixed internal resolution
             * regardless of the actual window/monitor size — the engine
             * handles scaling and letterboxing (black bars, preserving
             * aspect ratio) to fit whatever the real window size ends up
             * being, whether that's from this toggle or the player manually
             * resizing the window. Nothing else needs to account for it.
             *
             * `strategy` (optional, sunlight `v0.14.0`+) picks which of two
             * fullscreen strategies to enter with:
             *
             * - `FULLSCREEN_STRATEGY_REAL` (the default when `strategy` is
             *   omitted) — a genuine OS-level fullscreen space. On macOS
             *   this is the strategy that actually gets the Dock/menu bar
             *   to hide automatically; a real bug found live in Caravellius
             *   (the Dock drawing on top of the game window) is what
             *   prompted sunlight to switch its own default to this.
             * - `FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED` — the older
             *   behavior (an ordinary window resized to the monitor's
             *   native resolution, no real fullscreen space entered) — kept
             *   available as a fallback for any platform/window manager
             *   where a true video-mode switch misbehaves.
             *
             * Switching strategy while already fullscreen in the *other*
             * one is unsupported — call `app_set_fullscreen(false)` first,
             * then re-enter fullscreen with the new strategy.
             * @luaexample
             * -- toggle fullscreen on a key press (edge-triggered, not held) - defaults to real fullscreen
             * if input_is_key_released( KEY_F5 ) then
             *     app_set_fullscreen( not app_get_fullscreen() )
             * end
             *
             * -- explicitly ask for the older borderless-windowed behavior instead
             * app_set_fullscreen( true, FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED )
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
             *
             * @luaname{app_get_fullscreen()}
             * @luagroup{fullscreen}
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
             *
             * @luaname{app_set_target_fps(fps)}
             * @luagroup{target_fps}
             * @luadoc
             * Set (or query) the renderer's own target frame rate — the cap
             * the game loop paces itself against, not the current
             * *measured* FPS (see `app_get_draw_fps`'s own on-screen
             * counter, right below, for that). Scarab's own default is
             * `main.h`'s `FRAMES_PER_SECOND` (`60` today), applied once at
             * boot; this primitive is a genuine live change, taking effect
             * immediately, in either direction:
             * @luaexample
             * -- halve the target frame rate at runtime
             * app_set_target_fps( app_get_target_fps() / 2 )
             */
            int LuaAppApi :: SetTargetFps( lua_State *pLuaState )  {

                int  nTargetFps = ( int ) lua_tointeger( pLuaState, 1 );

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> SetTargetFPS( nTargetFps );

                return 0;
            }

            /**
             * @brief Query the currently configured target FPS (see
             * @link SetTargetFps) - NOT the current measured FPS.
             *
             * @luaname{app_get_target_fps()}
             * @luagroup{target_fps}
             */
            int LuaAppApi :: GetTargetFps( lua_State *pLuaState )  {

                int  nTargetFps = LuaEngineUtil :: GetDrawSurface( pLuaState ) -> GetTargetFPS();

                lua_pushinteger( pLuaState, nTargetFps );

                return 1;
            }

            /**
             * @brief Show or hide the on-screen FPS counter.
             *
             * @luaname{app_set_draw_fps(draw_fps)}
             * @luagroup{draw_fps}
             * @luadoc
             * Show or hide the on-screen FPS counter (raylib's own, drawn
             * at the top-left corner of the window every frame), or query
             * the current state. Scarab's own default (`main.cpp`) is
             * `false` — a generic engine-level choice, not a game one.
             * Caravellius overrides it to `true`, early in `main.lua`:
             * @luaexample
             * -- main.lua
             * app_set_draw_fps( true )
             */
            int LuaAppApi :: SetDrawFps( lua_State *pLuaState )  {

                bool  bDrawFPS = lua_toboolean( pLuaState, 1 );

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> SetDrawFPS( bDrawFPS );

                return 0;
            }

            /**
             * @brief Query whether the FPS counter is currently being drawn
             * (see @link SetDrawFps).
             *
             * @luaname{app_get_draw_fps()}
             * @luagroup{draw_fps}
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
             *
             * @luaname{app_set_window_resizeable(resizeable)}
             * @luagroup{window_resizeable}
             * @luadoc
             * Allow or disallow the user resizing the window by dragging
             * its edges/corners, or query the current state. A genuine
             * **live** toggle — safe to call both before the window exists
             * and at any point after, in either direction (unlike raylib's
             * own pre-`InitWindow`-only `FLAG_WINDOW_RESIZABLE` config
             * flag). Scarab's own default (`main.cpp`) is `true`;
             * Caravellius overrides it to `false` (the game renders at a
             * fixed internal resolution with letterboxing, so a resizeable
             * window offers no benefit):
             * @luaexample
             * -- main.lua
             * app_set_window_resizeable( false )
             */
            int LuaAppApi :: SetWindowResizeable( lua_State *pLuaState )  {

                bool  bResizeable = lua_toboolean( pLuaState, 1 );

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> SetWindowResizeable( bResizeable );

                return 0;
            }

            /**
             * @brief Query whether the window is currently resizeable (see
             * @link SetWindowResizeable).
             *
             * @luaname{app_get_window_resizeable()}
             * @luagroup{window_resizeable}
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
             *
             * @luaname{app_set_stretch_to_fill(stretchToFill)}
             * @luagroup{stretch_to_fill}
             * @luadoc
             * Choose how the engine's fixed internal render resolution
             * (`DISPLAY_W`x`DISPLAY_H`, `1260x920` today — what
             * `screen_get_width`/`screen_get_height` report, unrelated to
             * the real window size) gets blitted onto the real
             * window/screen whenever the two sizes differ (fullscreen, or
             * a live-resized window). Recomputed every frame, so it applies
             * immediately regardless of when it's called.
             *
             * - `false` (the engine's own default, both Scarab's and
             *   Caravellius's) — **letterbox**: preserves the render
             *   target's own aspect ratio via one uniform scale factor,
             *   filling whichever axis doesn't fit exactly with black bars.
             * - `true` — **stretch-to-fill**: fills the entire
             *   window/screen with no black bars at all, using independent
             *   X/Y scale factors — the image visibly warps
             *   (stretches/squishes) whenever the window's own aspect
             *   ratio doesn't match the render target's `1260:920`.
             * @luaexample
             * app_set_stretch_to_fill( true )   -- fill the window completely, no letterboxing
             */
            int LuaAppApi :: SetStretchToFill( lua_State *pLuaState )  {

                bool  bStretchToFill = lua_toboolean( pLuaState, 1 );

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> SetStretchToFill( bStretchToFill );

                return 0;
            }

            /**
             * @brief Query whether the render target is currently being
             * stretched to fill (see @link SetStretchToFill).
             *
             * @luaname{app_get_stretch_to_fill()}
             * @luagroup{stretch_to_fill}
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
             *
             * @luaname{draw_filled_rectangle(x, y, width, height, r, g, b, a)}
             * @luadoc
             * Draw a filled, solid-color rectangle in screen space — same
             * coordinate system as `draw_text`/`screen_get_width` (the
             * fixed `1260x920` design resolution, independent of the live
             * window size, fullscreen state, or `app_set_stretch_to_fill`),
             * not a world-space/camera-relative one. Meant for simple HUD
             * elements — a progress/health bar, a meter — that don't
             * warrant a whole sprite/texture asset. A thin pass-through to
             * `ITileMap::DrawFilledRectangle`, itself a thin pass-through
             * to `IEngine::DrawFilledRectangle` — the same underlying call
             * the whole-screen `screen_fade` overlay already uses
             * internally, just exposed here as it's own
             * arbitrary-position/size/color primitive rather than that
             * fixed, full-window, single-alpha-value overlay.
             * @luaexample
             * -- a simple health bar: a dark background plate, then a colored fill
             * -- proportional to some fraction (0.0-1.0)
             * draw_filled_rectangle( x, y, width, height, 40, 40, 40, 255 )
             * draw_filled_rectangle( x, y, math.floor( width * fraction ), height, 220, 40, 40, 255 )
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
             * @brief Query which host platform Scarab was compiled for
             * (see LuaAppApi::Platform) - a pure compile-time fact, not a
             * runtime probe, so this never changes for the lifetime of a
             * given built executable. Resolved via the same
             * _WIN32/__APPLE__/__linux__ preprocessor macros every other
             * cross-platform C++ project in this ecosystem (raylib,
             * sunlight's own CMakeLists.txt) already relies on - no new
             * platform-detection mechanism invented here.
             *
             * @luaname{app_get_platform()}
             * @luadoc
             * Query which host platform Scarab was compiled for —
             * `PLATFORM_WINDOWS`, `PLATFORM_MACOS`, `PLATFORM_LINUX`, or
             * `PLATFORM_UNKNOWN` (compiled for something other than those
             * three). A pure compile-time fact
             * (`_WIN32`/`__APPLE__`/`__linux__` preprocessor macros), not a
             * runtime probe — the result never changes for the lifetime of
             * a given built executable, and there's no dependency on
             * `IEngine`/raylib/GLFW or any other engine-owned state.
             *
             * Useful for any platform-conditional Lua behavior a game
             * needs — for example, picking which `app_set_fullscreen`
             * strategy to default to per platform. `FULLSCREEN_STRATEGY_REAL`
             * causes a measured performance regression on Linux. Windows
             * isn't affected by that specific issue, but
             * `FULLSCREEN_STRATEGY_REAL` is kept macOS-only project-wide
             * rather than per-platform — Linux and Windows both use
             * `FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED` here by deliberate
             * choice, not because Windows itself has a problem with
             * `FULLSCREEN_STRATEGY_REAL`:
             * @luaexample
             * local platform = app_get_platform()
             *
             * if platform == PLATFORM_MACOS then
             *     app_set_fullscreen( true, FULLSCREEN_STRATEGY_REAL )
             * else
             *     app_set_fullscreen( true, FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED )
             * end
             */
            int LuaAppApi :: GetPlatform( lua_State *pLuaState )  {

                #if defined( _WIN32 )
                    lua_pushinteger( pLuaState, LuaAppApi :: PLATFORM_WINDOWS );
                #elif defined( __APPLE__ )
                    lua_pushinteger( pLuaState, LuaAppApi :: PLATFORM_MACOS );
                #elif defined( __linux__ )
                    lua_pushinteger( pLuaState, LuaAppApi :: PLATFORM_LINUX );
                #else
                    lua_pushinteger( pLuaState, LuaAppApi :: PLATFORM_UNKNOWN );
                #endif

                return 1;
            }

            /**
             * @luaname{app_quit()}
             * @luadoc
             * Requests the game close, the programmatic equivalent of the
             * player pressing the configured exit key (`ESC` by default)
             * or clicking the window's close button. Takes effect on the
             * next frame, not immediately — safe to call from anywhere
             * `on_update`/an engine callback reaches, including mid-frame;
             * whatever's already queued to draw this frame still finishes
             * drawing normally before the game actually closes.
             *
             * There was no programmatic way to quit at all before this —
             * every prior sample/tool worked around that by just telling
             * the player to close the window manually.
             * @luaexample
             * function on_update(dt)
             *   if quit_button_pressed then
             *     app_quit()
             *   end
             * end
             */
            int LuaAppApi :: Quit( lua_State *pLuaState )  {

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> RequestExit();

                return 0;
            }

            /**
             * @luaname{app_set_exit_key(key)}
             * @luagroup{exit_key}
             * @luadoc
             * Sets which key, when pressed, triggers the default
             * quit-the-application behavior (`app_quit()`'s own effect,
             * but from a raw keypress rather than a direct call) —
             * `KEY_ESCAPE` (see the "Keyboard keys" constants on
             * `input.md`) by default. Pass `KEY_NULL` to disable this
             * entirely, so
             * pressing `ESC` no longer closes the game on its own —
             * useful for a game that wants `ESC` to mean something else
             * instead (a back/cancel action in its own menu system, for
             * example), handled entirely in Lua by polling
             * `input_is_key_released(KEY_ESCAPE)` each frame, rather than
             * an un-interceptable quit racing ahead of it.
             * @luaexample
             * -- disable the default ESC-quits behavior; this game's own
             * -- Lua code decides what ESC does instead
             * app_set_exit_key( KEY_NULL )
             */
            int LuaAppApi :: SetExitKey( lua_State *pLuaState )  {

                int  nKey = ( int ) lua_tointeger( pLuaState, 1 );

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> SetExitKey( ( SunLight :: Input :: KeyboardKey ) nKey );

                return 0;
            }

            /**
             * @brief Query the key currently configured to trigger
             * quit-on-press (see @link SetExitKey).
             *
             * @luaname{app_get_exit_key()}
             * @luagroup{exit_key}
             */
            int LuaAppApi :: GetExitKey( lua_State *pLuaState )  {

                SunLight :: Input :: KeyboardKey  key = LuaEngineUtil :: GetDrawSurface( pLuaState ) -> GetExitKey();

                lua_pushinteger( pLuaState, ( lua_Integer ) key );

                return 1;
            }

            /**
             * @brief Register app_set_fullscreen's own optional strategy
             * argument (see @link SetFullscreen) as Lua globals, same
             * names as the underlying SunLight::Engines::IEngine enum -
             * mirrors LuaTilemapApi::RegisterEnums's own MAP_ALIGNMENT_*
             * pattern for a small, engine-defined enum exposed to Lua.
             */
            void LuaAppApi :: RegisterEnums( lua_State *pLuaState )  {

                /**
                 * @luaconstants{Fullscreen strategies}
                 * @luadoc
                 * The `strategy` argument `app_set_fullscreen` optionally
                 * takes — see that primitive's own doc for what each one
                 * actually does and why `FULLSCREEN_STRATEGY_REAL` is
                 * kept macOS-only project-wide.
                 */
                static const stNamedConstant  s_aFullscreenStrategies[] = {
                    { "FULLSCREEN_STRATEGY_REAL", SunLight :: Engines :: IEngine :: FULLSCREEN_STRATEGY_REAL },
                    { "FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED", SunLight :: Engines :: IEngine :: FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED },
                };

                LuaEngineUtil :: RegisterConstants( pLuaState, s_aFullscreenStrategies,
                    sizeof( s_aFullscreenStrategies ) / sizeof( s_aFullscreenStrategies[0] ) );

                /**
                 * @luaconstants{Platforms}
                 * @luadoc
                 * The values `app_get_platform()` returns.
                 */
                static const stNamedConstant  s_aPlatforms[] = {
                    { "PLATFORM_WINDOWS", LuaAppApi :: PLATFORM_WINDOWS },
                    { "PLATFORM_MACOS", LuaAppApi :: PLATFORM_MACOS },
                    { "PLATFORM_LINUX", LuaAppApi :: PLATFORM_LINUX },
                    { "PLATFORM_UNKNOWN", LuaAppApi :: PLATFORM_UNKNOWN },
                };

                LuaEngineUtil :: RegisterConstants( pLuaState, s_aPlatforms,
                    sizeof( s_aPlatforms ) / sizeof( s_aPlatforms[0] ) );
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
                lua_register( pLuaState, "app_get_platform", LuaAppApi :: GetPlatform );
                lua_register( pLuaState, "app_quit", LuaAppApi :: Quit );
                lua_register( pLuaState, "app_set_exit_key", LuaAppApi :: SetExitKey );
                lua_register( pLuaState, "app_get_exit_key", LuaAppApi :: GetExitKey );
            }
        }
    }
}
