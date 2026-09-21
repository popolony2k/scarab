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

#include "lua/luarendererapi.h"
#include "lua/luaengineutil.h"
#include "base/viewport.h"
#include "tilemap/iview.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

extern "C"
{
  #include "lauxlib.h"
}

using namespace SunLight :: Renderer;

/*
 * Named constants for the values this file would otherwise hardcode.
 */
#define __RENDERER_HANDLE              1     // the one renderer per process; opaque to scripts
#define __DEFAULT_VIEW_ID              0     // sunlight's view 0 - the renderer's own camera/viewport/zoom
#define __ZOOM_EXACT_TOLERANCE         1e-9  // factor/ZOOM_STEP is exact for valid factors; this only absorbs input noise
#define __ORIGIN_NAME_CREATED          "created"
#define __ORIGIN_NAME_LAZY_DEFAULT     "lazy_default"

namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief Push `nil, message` - the failure convention of every function in this file.
             */
            static int PushError( lua_State *pLuaState, const std :: string &strMessage )  {

                lua_pushnil( pLuaState );
                lua_pushstring( pLuaState, strMessage.c_str() );

                return 2;
            }

            /**
             * @brief Format a number the way a user would write it in a config file (3.8125, 16).
             */
            static std :: string FormatNumber( double fValue )  {

                char  szBuffer[64];

                snprintf( szBuffer, sizeof( szBuffer ), "%g", fValue );

                return szBuffer;
            }

            /**
             * @brief Check a zoom FACTOR and convert it to sunlight's zoom position.
             *
             * A valid factor is a whole multiple of ZOOM_STEP within [ZOOM_FACTOR_MIN,
             * ZOOM_FACTOR_MAX]; factor(p) = (p + 1) x ZOOM_STEP. ZOOM_STEP is a power of two, so
             * every valid factor is exact and this needs no rounding of what the script wrote:
             * 3.8125 is valid, 3.8 is not (it is almost certainly a typo, so it is rejected, not
             * silently rounded). The message names the nearest valid factor. Shared by
             * renderer_create's `zoom` option and the view zoom setters.
             *
             * @param fFactor The requested zoom factor;
             * @param nZoomPos Receives the zoom position on success;
             * @param strError Receives the reason on failure;
             */
            bool LuaRendererApi :: ValidateZoomFactor( double fFactor, unsigned &nZoomPos, std :: string &strError )  {

                double  fMin  = SunLight :: Base :: ZOOM_FACTOR_MIN;
                double  fMax  = SunLight :: Base :: ZOOM_FACTOR_MAX;

                if( !std :: isfinite( fFactor ) )  {
                    strError = "zoom factor must be a finite number";
                    return false;
                }

                double  fSteps        = fFactor / SunLight :: Base :: ZOOM_STEP;
                double  fWhole        = std :: round( fSteps );
                double  fNearestSteps = std :: fmin( std :: fmax( fWhole, 1.0 ), ( double ) SunLight :: Base :: ZOOM_POS_COUNT );
                double  fNearest      = fNearestSteps * SunLight :: Base :: ZOOM_STEP;

                if( std :: fabs( fSteps - fWhole ) > __ZOOM_EXACT_TOLERANCE )  {
                    strError = "zoom factor " + FormatNumber( fFactor ) + " is not a multiple of " +
                               FormatNumber( SunLight :: Base :: ZOOM_STEP ) + " - nearest valid is " + FormatNumber( fNearest );
                    return false;
                }

                if( fWhole < 1.0 || fWhole > ( double ) SunLight :: Base :: ZOOM_POS_COUNT )  {
                    strError = "zoom factor " + FormatNumber( fFactor ) + " is outside [" + FormatNumber( fMin ) + ", " +
                               FormatNumber( fMax ) + "] - nearest valid is " + FormatNumber( fNearest );
                    return false;
                }

                nZoomPos = ( unsigned ) fWhole - 1u;

                return true;
            }

            /**
             * @brief Read an integer option: a Lua number with an exact integer value (a JSON
             * `1260` arrives as an integer, `1260.0` is accepted too; `1260.5` and strings are not).
             */
            static bool ReadInteger( lua_State *pLuaState, int nIndex, const std :: string &strName,
                                     lua_Integer nMin, lua_Integer nMax, lua_Integer &nOut, std :: string &strError )  {

                int  bIsInteger = 0;

                if( lua_type( pLuaState, nIndex ) == LUA_TNUMBER )
                    nOut = lua_tointegerx( pLuaState, nIndex, &bIsInteger );

                if( !bIsInteger )  {
                    strError = "option '" + strName + "' must be an integer";
                    return false;
                }

                if( nOut < nMin || nOut > nMax )  {
                    strError = "option '" + strName + "' must be between " + std :: to_string( nMin ) + " and " +
                               std :: to_string( nMax ) + " (got " + std :: to_string( nOut ) + ")";
                    return false;
                }

                return true;
            }

            static bool ReadBoolean( lua_State *pLuaState, int nIndex, const std :: string &strName, bool &bOut, std :: string &strError )  {

                if( lua_type( pLuaState, nIndex ) != LUA_TBOOLEAN )  {
                    strError = "option '" + strName + "' must be a boolean";
                    return false;
                }

                bOut = lua_toboolean( pLuaState, nIndex ) != 0;

                return true;
            }

            /**
             * @brief Read a `{ a = <int>, b = <int>, ... }` sub-table option. Every listed field
             * is required and any other key is rejected, so a misspelt field is an error, not a
             * silently ignored one.
             */
            static bool ReadIntegerFields( lua_State *pLuaState, int nIndex, const std :: string &strName,
                                           const char * const *aszFields, size_t nFields,
                                           lua_Integer nMin, lua_Integer nMax, lua_Integer *aOut, std :: string &strError )  {

                if( lua_type( pLuaState, nIndex ) != LUA_TTABLE )  {
                    strError = "option '" + strName + "' must be a table";
                    return false;
                }

                int  nTable = lua_absindex( pLuaState, nIndex );

                lua_pushnil( pLuaState );

                while( lua_next( pLuaState, nTable ) != 0 )  {
                    bool  bKnown = false;

                    if( lua_type( pLuaState, -2 ) == LUA_TSTRING )  {
                        const char  *szKey = lua_tostring( pLuaState, -2 );

                        for( size_t nField = 0; nField < nFields; nField++ )
                            if( strcmp( szKey, aszFields[nField] ) == 0 )
                                bKnown = true;
                    }

                    if( !bKnown )  {
                        strError = "option '" + strName + "' has an unknown field";

                        if( lua_type( pLuaState, -2 ) == LUA_TSTRING )
                            strError = "option '" + strName + "' has an unknown field '" + lua_tostring( pLuaState, -2 ) + "'";

                        return false;
                    }

                    lua_pop( pLuaState, 1 );
                }

                for( size_t nField = 0; nField < nFields; nField++ )  {
                    std :: string  strField = strName + "." + aszFields[nField];

                    lua_getfield( pLuaState, nTable, aszFields[nField] );

                    if( lua_isnil( pLuaState, -1 ) )  {
                        strError = "option '" + strName + "' is missing field '" + aszFields[nField] + "'";
                        return false;
                    }

                    bool  bOk = ReadInteger( pLuaState, -1, strField, nMin, nMax, aOut[nField], strError );

                    lua_pop( pLuaState, 1 );

                    if( !bOk )
                        return false;
                }

                return true;
            }

            /**
             * @brief Fill a RendererConfig from a renderer_create options table, strictly: an
             * unknown option name, a wrong type or an out-of-range value is an error with the
             * exact reason. Options left out keep the value already in *pConfig (Scarab's
             * defaults). Also reports which of the null-backend-only options were given.
             */
            static bool ParseOptions( lua_State *pLuaState, int nTable, RendererConfig &config,
                                      bool &bNullOnlyGiven, std :: string &strError )  {

                static const char * const  aszViewport[]   = { "x", "y", "w", "h" };
                static const char * const  aszScrollStep[] = { "w", "h" };
                const lua_Integer          nIntMax         = 0x7FFFFFFF;

                lua_pushnil( pLuaState );

                while( lua_next( pLuaState, nTable ) != 0 )  {

                    if( lua_type( pLuaState, -2 ) != LUA_TSTRING )  {
                        strError = "option names must be strings";
                        return false;
                    }

                    std :: string  strKey = lua_tostring( pLuaState, -2 );
                    lua_Integer    nValue = 0;
                    bool           bOk    = true;

                    if( strKey == "backend" )  {
                        bOk = ReadInteger( pLuaState, -1, strKey, RENDERER_BACKEND_DEFAULT, RENDERER_BACKEND_LAST - 1, nValue, strError );

                        if( bOk )
                            config.backend = ( RendererBackend ) nValue;
                    }
                    else if( strKey == "width" )  {
                        bOk = ReadInteger( pLuaState, -1, strKey, 1, nIntMax, nValue, strError );

                        if( bOk )
                            config.fWidth = ( float ) nValue;
                    }
                    else if( strKey == "height" )  {
                        bOk = ReadInteger( pLuaState, -1, strKey, 1, nIntMax, nValue, strError );

                        if( bOk )
                            config.fHeight = ( float ) nValue;
                    }
                    else if( strKey == "title" )  {
                        if( lua_type( pLuaState, -1 ) != LUA_TSTRING )  {
                            strError = "option 'title' must be a string";
                            bOk = false;
                        }
                        else
                            config.strTitle = lua_tostring( pLuaState, -1 );
                    }
                    else if( strKey == "fps" )  {
                        bOk = ReadInteger( pLuaState, -1, strKey, 1, nIntMax, nValue, strError );

                        if( bOk )
                            config.nTargetFps = ( int ) nValue;
                    }
                    else if( strKey == "resizeable" )
                        bOk = ReadBoolean( pLuaState, -1, strKey, config.bResizeable, strError );
                    else if( strKey == "draw_fps" )
                        bOk = ReadBoolean( pLuaState, -1, strKey, config.bDrawFPS, strError );
                    else if( strKey == "use_default_key_handler" )
                        bOk = ReadBoolean( pLuaState, -1, strKey, config.bUseDefaultKeyHandler, strError );
                    else if( strKey == "stretch_to_fill" )
                        bOk = ReadBoolean( pLuaState, -1, strKey, config.bStretchToFill, strError );
                    else if( strKey == "exit_key" )  {
                        bOk = ReadInteger( pLuaState, -1, strKey, 0, nIntMax, nValue, strError );

                        if( bOk )
                            config.exitKey = ( SunLight :: Input :: KeyboardKey ) nValue;
                    }
                    else if( strKey == "view_control_mode" )  {
                        bOk = ReadInteger( pLuaState, -1, strKey, VIEW_CONTROL_MODE_ACTIVE, VIEW_CONTROL_MODE_REACTIVE, nValue, strError );

                        if( bOk )
                            config.viewControlMode = ( ViewControlMode ) nValue;
                    }
                    else if( strKey == "zoom" )  {
                        if( lua_type( pLuaState, -1 ) != LUA_TNUMBER )  {
                            strError = "option 'zoom' must be a number (a zoom factor, eg. 3.8125)";
                            bOk = false;
                        }
                        else  {
                            unsigned  nZoomPos = 0;

                            bOk = LuaRendererApi :: ValidateZoomFactor( lua_tonumber( pLuaState, -1 ), nZoomPos, strError );

                            if( bOk )
                                config.nZoomPos = nZoomPos;
                            else
                                strError = "option 'zoom': " + strError;
                        }
                    }
                    else if( strKey == "viewport" )  {
                        lua_Integer  aRect[4] = { 0, 0, 0, 0 };

                        bOk = ReadIntegerFields( pLuaState, -1, strKey, aszViewport, 4, 0, nIntMax, aRect, strError );

                        if( bOk )  {
                            SunLight :: TileMap :: stDimension2D  viewport;

                            viewport.pos.x        = ( int ) aRect[0];
                            viewport.pos.y        = ( int ) aRect[1];
                            viewport.size.nWidth  = ( int ) aRect[2];
                            viewport.size.nHeight = ( int ) aRect[3];
                            config.viewport       = viewport;
                        }
                    }
                    else if( strKey == "scroll_step" )  {
                        lua_Integer  aStep[2] = { 0, 0 };

                        bOk = ReadIntegerFields( pLuaState, -1, strKey, aszScrollStep, 2, 1, nIntMax, aStep, strError );

                        if( bOk )  {
                            config.nScrollStepWidth  = ( int ) aStep[0];
                            config.nScrollStepHeight = ( int ) aStep[1];
                        }
                    }
                    else if( strKey == "frame_pacing" )  {
                        bOk = ReadInteger( pLuaState, -1, strKey, FRAME_PACING_REAL_TIME, FRAME_PACING_UNLIMITED, nValue, strError );

                        if( bOk )  {
                            config.framePacing = ( FramePacing ) nValue;
                            bNullOnlyGiven     = true;
                        }
                    }
                    else if( strKey == "max_frames" )  {
                        bOk = ReadInteger( pLuaState, -1, strKey, 0, nIntMax, nValue, strError );

                        if( bOk )  {
                            config.nMaxFrames = ( unsigned ) nValue;
                            bNullOnlyGiven    = true;
                        }
                    }
                    else  {
                        strError = "unknown option '" + strKey + "'";
                        bOk = false;
                    }

                    if( !bOk )
                        return false;

                    lua_pop( pLuaState, 1 );
                }

                return true;
            }

            /**
             * @brief renderer_create's checks that need the whole configuration, not one option.
             */
            static bool ValidateConfig( const RendererConfig &config, bool bNullOnlyGiven, bool bNullBackend, std :: string &strError )  {

                if( bNullOnlyGiven && !bNullBackend )  {
                    strError = "frame_pacing/max_frames are only valid on the null backend";
                    return false;
                }

                if( config.viewport )  {
                    const SunLight :: TileMap :: stDimension2D  &vp = *config.viewport;

                    if( vp.size.nWidth < 1 || vp.size.nHeight < 1 )  {
                        strError = "option 'viewport' needs w and h of at least 1";
                        return false;
                    }

                    if( vp.pos.x + vp.size.nWidth > ( int ) config.fWidth || vp.pos.y + vp.size.nHeight > ( int ) config.fHeight )  {
                        strError = "the viewport must fit the render area: x+w=" + std :: to_string( vp.pos.x + vp.size.nWidth ) +
                                   " and y+h=" + std :: to_string( vp.pos.y + vp.size.nHeight ) + " must not exceed width " +
                                   std :: to_string( ( int ) config.fWidth ) + " and height " + std :: to_string( ( int ) config.fHeight ) +
                                   " (pass 'viewport' explicitly if you changed width/height - it defaults to the standard 10,10,1240,900)";
                        return false;
                    }
                }

                return config.Validate( &strError );
            }

            /**
             * @brief Check that argument nIndex names the current renderer.
             */
            bool LuaRendererApi :: CheckHandle( lua_State *pLuaState, int nIndex, std :: string &strError )  {

                Engine :: IRendererProvider  *pProvider = LuaEngineUtil :: GetRendererProvider( pLuaState );

                if( lua_type( pLuaState, nIndex ) != LUA_TNUMBER || !lua_isinteger( pLuaState, nIndex ) )  {
                    strError = "expected a renderer handle (the value renderer_create returned)";
                    return false;
                }

                lua_Integer  nHandle = lua_tointeger( pLuaState, nIndex );

                if( pProvider -> GetOrigin() == Engine :: IRendererProvider :: ORIGIN_NONE || nHandle != __RENDERER_HANDLE )  {
                    strError = "unknown renderer " + std :: to_string( nHandle );
                    return false;
                }

                return true;
            }

            /**
             * @luaname{renderer_create(options) -> renderer | nil, message}
             * @luadoc
             * Create the process's renderer with your own settings and open its window. Call it
             * **first** — see the ordering rule on this page. `options` is a table, and every
             * key is optional; what you leave out keeps Scarab's default (shown below):
             *
             * | option | meaning | default |
             * |---|---|---|
             * | `backend` | `RENDERER_BACKEND_DEFAULT`, `_RAYLIB` or `_NULL` | `_DEFAULT` |
             * | `width`, `height` | the render area, in pixels | `1260`, `920` |
             * | `title` | the window title | `"Scarab"` |
             * | `fps` | target frames per second | `60` |
             * | `resizeable` | may the user resize the window | `true` |
             * | `draw_fps` | draw the FPS counter | `false` |
             * | `viewport` | `{ x =, y =, w =, h = }` — the visible rectangle, which must fit the render area | `{ x = 10, y = 10, w = 1240, h = 900 }` |
             * | `zoom` | the starting zoom **factor** (a multiple of `ZOOM_STEP`, within `ZOOM_FACTOR_MIN..ZOOM_FACTOR_MAX`) | `3.8125` |
             * | `scroll_step` | `{ w =, h = }` — how far one `camera_move_*` call moves | `{ w = 1, h = 1 }` |
             * | `view_control_mode` | `VIEW_CONTROL_MODE_ACTIVE` or `_REACTIVE` | `_ACTIVE` |
             * | `use_default_key_handler` | sunlight's built-in scroll/zoom key bindings | `false` |
             * | `exit_key` | a `KEY_*` constant; `KEY_NULL` means no exit key | `KEY_ESCAPE` |
             * | `stretch_to_fill` | stretch the render area to fill the window | `false` |
             * | `frame_pacing` | `FRAME_PACING_REAL_TIME` or `_UNLIMITED` — null backend only | `_REAL_TIME` |
             * | `max_frames` | stop after this many frames, `0` = never — null backend only | `0` |
             *
             * The check is strict: an unknown option name, a wrong type, or an out-of-range value
             * returns `nil` and the exact reason (`"unknown option 'titel'"`,
             * `"zoom factor 3.8 is not a multiple of 0.0625 - nearest valid is 3.8125"`). Whole
             * JSON numbers loaded with `load_json` arrive as Lua integers, so a config file works
             * unchanged.
             *
             * Under `--headless` the null backend is forced: leave `backend` out (or pass
             * `RENDERER_BACKEND_NULL`); asking for another one fails, and `--fast`/`--max-frames`
             * from the command line win over `frame_pacing`/`max_frames`.
             *
             * Returns an opaque renderer handle. Fails if a renderer already exists — either from
             * an earlier `renderer_create` or created implicitly by an earlier window-needing call.
             * @luaexample
             * local config = load_json( APP_DIR .. "resources/configs/renderer.json" )
             * local renderer, err = renderer_create{
             *     title      = config.title,
             *     width      = config.width,
             *     height     = config.height,
             *     viewport   = { x = 10, y = 10, w = 1240, h = 900 },
             *     zoom       = 3.8125,
             *     resizeable = false,
             *     exit_key   = KEY_NULL,
             * }
             *
             * if not renderer then
             *     error( "cannot create the renderer: " .. err )
             * end
             */
            int LuaRendererApi :: RendererCreate( lua_State *pLuaState )  {

                Engine :: IRendererProvider  *pProvider = LuaEngineUtil :: GetRendererProvider( pLuaState );
                RendererConfig                config    = pProvider -> GetDefaultConfig();
                bool                          bNullOnlyGiven = false;
                std :: string                 strError;

                if( lua_gettop( pLuaState ) > 1 || ( lua_gettop( pLuaState ) == 1 && !lua_istable( pLuaState, 1 ) ) )
                    return PushError( pLuaState, "renderer_create expects one argument: an options table" );

                if( lua_gettop( pLuaState ) == 1 && !ParseOptions( pLuaState, 1, config, bNullOnlyGiven, strError ) )
                    return PushError( pLuaState, strError );

                bool  bNullBackend = pProvider -> IsHeadless() || config.backend == RENDERER_BACKEND_NULL;

                if( !ValidateConfig( config, bNullOnlyGiven, bNullBackend, strError ) )
                    return PushError( pLuaState, strError );

                if( !pProvider -> CreateRenderer( config, &strError ) )
                    return PushError( pLuaState, strError );

                lua_pushinteger( pLuaState, __RENDERER_HANDLE );

                return 1;
            }

            /**
             * @luaname{renderer_destroy(renderer) -> true | nil, message}
             * @luadoc
             * Ask the renderer to stop: the frame in progress finishes, `on_update` is not called
             * again, and the process exits. The same effect as `app_quit()`, addressed by handle.
             * There is one renderer per process run, so a second one cannot be created afterwards.
             * @luaexample
             * local renderer = renderer_get_current()
             * if renderer then renderer_destroy( renderer ) end
             */
            int LuaRendererApi :: RendererDestroy( lua_State *pLuaState )  {

                std :: string  strError;

                if( !CheckHandle( pLuaState, 1, strError ) )
                    return PushError( pLuaState, strError );

                LuaEngineUtil :: GetRendererProvider( pLuaState ) -> PeekDrawSurface() -> RequestExit();

                lua_pushboolean( pLuaState, 1 );

                return 1;
            }

            /**
             * @luaname{renderer_get_current() -> renderer, origin | nil}
             * @luadoc
             * The renderer, if one exists, and how it came to exist: `"created"` (by
             * `renderer_create`) or `"lazy_default"` (implicitly, by the first window-needing
             * call — the default configuration). Returns `nil` if no renderer exists yet. Never
             * creates one, so it is safe to call first — use it to check whether the ordering
             * rule has been broken.
             * @luaexample
             * local renderer, origin = renderer_get_current()
             * assert( renderer == nil, "a window call ran before renderer_create" )
             */
            int LuaRendererApi :: RendererGetCurrent( lua_State *pLuaState )  {

                Engine :: IRendererProvider  *pProvider = LuaEngineUtil :: GetRendererProvider( pLuaState );

                if( pProvider -> GetOrigin() == Engine :: IRendererProvider :: ORIGIN_NONE )  {
                    lua_pushnil( pLuaState );
                    return 1;
                }

                lua_pushinteger( pLuaState, __RENDERER_HANDLE );
                lua_pushstring( pLuaState, pProvider -> GetOrigin() == Engine :: IRendererProvider :: ORIGIN_CREATED
                                                ? __ORIGIN_NAME_CREATED : __ORIGIN_NAME_LAZY_DEFAULT );

                return 2;
            }

            static void SetIntegerField( lua_State *pLuaState, const char *szName, lua_Integer nValue )  {

                lua_pushinteger( pLuaState, nValue );
                lua_setfield( pLuaState, -2, szName );
            }

            static void SetBooleanField( lua_State *pLuaState, const char *szName, bool bValue )  {

                lua_pushboolean( pLuaState, bValue ? 1 : 0 );
                lua_setfield( pLuaState, -2, szName );
            }

            static void SetNumberField( lua_State *pLuaState, const char *szName, double fValue )  {

                lua_pushnumber( pLuaState, fValue );
                lua_setfield( pLuaState, -2, szName );
            }

            /**
             * @luaname{renderer_get_config(renderer) -> table | nil, message}
             * @luadoc
             * The renderer's effective settings as a table with the same names `renderer_create`
             * takes: `backend`, `width`, `height`, `title`, `fps`, `resizeable`, `draw_fps`,
             * `viewport` (`{ x, y, w, h }`), `zoom` (the current factor), `scroll_step`
             * (`{ w, h }`), `view_control_mode`, `use_default_key_handler`, `exit_key`,
             * `stretch_to_fill`, `frame_pacing` and `max_frames`. It reflects the *current*
             * values, not a snapshot from creation — the title after `app_set_name`, the zoom
             * after `zoom_in`, the exit key after `app_set_exit_key`. Use it at boot to assert
             * that the configuration you passed was really applied.
             * @luaexample
             * local config = renderer_get_config( renderer )
             * assert( config.title == "My Game" )
             * assert( config.zoom == 3.8125 )
             */
            int LuaRendererApi :: RendererGetConfig( lua_State *pLuaState )  {

                std :: string  strError;

                if( !CheckHandle( pLuaState, 1, strError ) )
                    return PushError( pLuaState, strError );

                Engine :: IRendererProvider           *pProvider = LuaEngineUtil :: GetRendererProvider( pLuaState );
                RendererConfig                         config    = pProvider -> GetEffectiveConfig();
                SunLight :: DrawSurface :: IDrawSurface  *pSurface  = pProvider -> PeekDrawSurface();
                SunLight :: TileMap :: ITileMap          *pTileMap  = pProvider -> GetTileMap( "renderer_get_config" );
                SunLight :: TileMap :: stDimension2D    &vp        = pTileMap -> GetViewport().GetDimension2D();

                lua_newtable( pLuaState );

                SetIntegerField( pLuaState, "backend", config.backend == RENDERER_BACKEND_DEFAULT ? RENDERER_BACKEND_RAYLIB : config.backend );
                SetIntegerField( pLuaState, "width", ( lua_Integer ) config.fWidth );
                SetIntegerField( pLuaState, "height", ( lua_Integer ) config.fHeight );
                lua_pushstring( pLuaState, config.strTitle.c_str() );
                lua_setfield( pLuaState, -2, "title" );
                SetIntegerField( pLuaState, "fps", pSurface -> GetTargetFPS() );
                SetBooleanField( pLuaState, "resizeable", pSurface -> GetWindowResizeable() );
                SetBooleanField( pLuaState, "draw_fps", pSurface -> GetDrawFPS() );

                lua_newtable( pLuaState );
                SetIntegerField( pLuaState, "x", vp.pos.x );
                SetIntegerField( pLuaState, "y", vp.pos.y );
                SetIntegerField( pLuaState, "w", vp.size.nWidth );
                SetIntegerField( pLuaState, "h", vp.size.nHeight );
                lua_setfield( pLuaState, -2, "viewport" );

                SetNumberField( pLuaState, "zoom", pTileMap -> GetViewport().GetZoomProperties().fZoomFactor );

                int  nStepWidth  = 0;
                int  nStepHeight = 0;

                pTileMap -> GetDefaultView().GetScrollStepSize( nStepWidth, nStepHeight );

                lua_newtable( pLuaState );
                SetIntegerField( pLuaState, "w", nStepWidth );
                SetIntegerField( pLuaState, "h", nStepHeight );
                lua_setfield( pLuaState, -2, "scroll_step" );

                SetIntegerField( pLuaState, "view_control_mode", config.viewControlMode );
                SetBooleanField( pLuaState, "use_default_key_handler", config.bUseDefaultKeyHandler );
                SetIntegerField( pLuaState, "exit_key", pSurface -> GetExitKey() );
                SetBooleanField( pLuaState, "stretch_to_fill", pSurface -> GetStretchToFill() );
                SetIntegerField( pLuaState, "frame_pacing", config.framePacing );
                SetIntegerField( pLuaState, "max_frames", config.nMaxFrames );

                return 1;
            }

            /**
             * @luaname{renderer_get_default_view(renderer) -> view | nil, message}
             * @luadoc
             * The renderer's default view — its own camera, viewport and zoom, the one every
             * `camera_*`, `zoom_*` and `viewport_*` function acts on. Always the view id `0`.
             * @luaexample
             * local view = renderer_get_default_view( renderer )
             */
            int LuaRendererApi :: RendererGetDefaultView( lua_State *pLuaState )  {

                std :: string  strError;

                if( !CheckHandle( pLuaState, 1, strError ) )
                    return PushError( pLuaState, strError );

                lua_pushinteger( pLuaState, __DEFAULT_VIEW_ID );

                return 1;
            }

            /**
             * @luaname{renderer_get_backend(renderer) -> backend | nil, message}
             * @luadoc
             * Which backend the renderer actually runs on: `RENDERER_BACKEND_RAYLIB` (a real
             * window) or `RENDERER_BACKEND_NULL` (headless). A `RENDERER_BACKEND_DEFAULT`
             * request reports as the backend it resolved to (today, raylib) — never as
             * `_DEFAULT` itself — so this is how a script tells whether it is running headless.
             * @luaexample
             * if renderer_get_backend( renderer ) == RENDERER_BACKEND_NULL then
             *     print( "running headless" )
             * end
             */
            int LuaRendererApi :: RendererGetBackend( lua_State *pLuaState )  {

                std :: string  strError;

                if( !CheckHandle( pLuaState, 1, strError ) )
                    return PushError( pLuaState, strError );

                RendererConfig  config = LuaEngineUtil :: GetRendererProvider( pLuaState ) -> GetEffectiveConfig();

                lua_pushinteger( pLuaState, config.backend == RENDERER_BACKEND_DEFAULT ? RENDERER_BACKEND_RAYLIB : config.backend );

                return 1;
            }

            /**
             * @luaname{renderer_get_view_count(renderer) -> count | nil, message}
             * @luadoc
             * How many views the renderer has, the default view included — so at least `1`.
             * `view_create` adds one, `view_destroy` removes one.
             * @luaexample
             * print( renderer_get_view_count( renderer ) )   -- 1 until view_create is used
             */
            int LuaRendererApi :: RendererGetViewCount( lua_State *pLuaState )  {

                std :: string  strError;

                if( !CheckHandle( pLuaState, 1, strError ) )
                    return PushError( pLuaState, strError );

                lua_pushinteger( pLuaState, LuaEngineUtil :: GetRendererProvider( pLuaState ) -> GetTileMap( "renderer_get_view_count" ) -> GetViewCount() );

                return 1;
            }

            /**
             * @brief Register the renderer/view constants exposed to Lua.
             */
            void LuaRendererApi :: RegisterEnums( lua_State *pLuaState )  {

                /**
                 * @luaconstants{Renderer backends}
                 * @luadoc
                 * The `backend` option of `renderer_create`, and what `renderer_get_backend`
                 * returns. `RENDERER_BACKEND_DEFAULT` means whatever this build was made with;
                 * `RENDERER_BACKEND_NULL` is the windowless, headless backend (see `--headless`).
                 */
                static const stNamedConstant  s_aBackends[] = {
                    { "RENDERER_BACKEND_DEFAULT", RENDERER_BACKEND_DEFAULT },
                    { "RENDERER_BACKEND_RAYLIB", RENDERER_BACKEND_RAYLIB },
                    { "RENDERER_BACKEND_NULL", RENDERER_BACKEND_NULL },
                };

                /**
                 * @luaconstants{View control modes}
                 * @luadoc
                 * The `view_control_mode` option of `renderer_create`.
                 */
                static const stNamedConstant  s_aViewControlModes[] = {
                    { "VIEW_CONTROL_MODE_ACTIVE", VIEW_CONTROL_MODE_ACTIVE },
                    { "VIEW_CONTROL_MODE_REACTIVE", VIEW_CONTROL_MODE_REACTIVE },
                };

                /**
                 * @luaconstants{Frame pacing}
                 * @luadoc
                 * The `frame_pacing` option of `renderer_create` (null backend only):
                 * `FRAME_PACING_REAL_TIME` runs at the target FPS; `FRAME_PACING_UNLIMITED` runs
                 * frames as fast as the game logic allows (the virtual clock is unchanged).
                 */
                static const stNamedConstant  s_aFramePacings[] = {
                    { "FRAME_PACING_REAL_TIME", FRAME_PACING_REAL_TIME },
                    { "FRAME_PACING_UNLIMITED", FRAME_PACING_UNLIMITED },
                };

                /**
                 * @luaconstants{Zoom positions}
                 * @luadoc
                 * sunlight's zoom scale has `256` positions, `ZOOM_POS_MIN` (`0`) to
                 * `ZOOM_POS_MAX` (`255`, the last *valid* position — never `256`). The factor of
                 * position `p` is `(p + 1) * ZOOM_STEP`. Zoom is set by **factor** everywhere in
                 * this API; these are for converting to and from a position.
                 */
                static const stNamedConstant  s_aZoomPositions[] = {
                    { "ZOOM_POS_MIN", ( int ) SunLight :: Base :: ZOOM_POS_MIN },
                    { "ZOOM_POS_MAX", ( int ) SunLight :: Base :: ZOOM_POS_MAX },
                };

                /**
                 * @luaconstants{Zoom factors}
                 * @luadoc
                 * `ZOOM_STEP` is `0.0625`; a valid zoom factor is a whole multiple of it,
                 * between `ZOOM_FACTOR_MIN` (`0.0625`, position `0`) and `ZOOM_FACTOR_MAX`
                 * (`16.0`, position `255`). Because the step is a power of two, every valid
                 * factor is exactly representable — `3.8125` is valid, `3.8` is not.
                 */
                static const stNamedRealConstant  s_aZoomFactors[] = {
                    { "ZOOM_STEP", SunLight :: Base :: ZOOM_STEP },
                    { "ZOOM_FACTOR_MIN", SunLight :: Base :: ZOOM_FACTOR_MIN },
                    { "ZOOM_FACTOR_MAX", SunLight :: Base :: ZOOM_FACTOR_MAX },
                };

                LuaEngineUtil :: RegisterConstants( pLuaState, s_aBackends, sizeof( s_aBackends ) / sizeof( s_aBackends[0] ) );
                LuaEngineUtil :: RegisterConstants( pLuaState, s_aViewControlModes, sizeof( s_aViewControlModes ) / sizeof( s_aViewControlModes[0] ) );
                LuaEngineUtil :: RegisterConstants( pLuaState, s_aFramePacings, sizeof( s_aFramePacings ) / sizeof( s_aFramePacings[0] ) );
                LuaEngineUtil :: RegisterConstants( pLuaState, s_aZoomPositions, sizeof( s_aZoomPositions ) / sizeof( s_aZoomPositions[0] ) );
                LuaEngineUtil :: RegisterRealConstants( pLuaState, s_aZoomFactors, sizeof( s_aZoomFactors ) / sizeof( s_aZoomFactors[0] ) );
            }

            /**
             * @brief Register the renderer lifecycle Lua-callable functions and constants.
             */
            void LuaRendererApi :: Register( lua_State *pLuaState )  {

                RegisterEnums( pLuaState );

                lua_register( pLuaState, "renderer_create", LuaRendererApi :: RendererCreate );
                lua_register( pLuaState, "renderer_destroy", LuaRendererApi :: RendererDestroy );
                lua_register( pLuaState, "renderer_get_current", LuaRendererApi :: RendererGetCurrent );
                lua_register( pLuaState, "renderer_get_config", LuaRendererApi :: RendererGetConfig );
                lua_register( pLuaState, "renderer_get_default_view", LuaRendererApi :: RendererGetDefaultView );
                lua_register( pLuaState, "renderer_get_backend", LuaRendererApi :: RendererGetBackend );
                lua_register( pLuaState, "renderer_get_view_count", LuaRendererApi :: RendererGetViewCount );
            }
        }
    }
}
