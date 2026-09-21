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

#include "lua/luaviewapi.h"
#include "lua/luaengineutil.h"
#include "lua/luarendererapi.h"
#include "tilemap/iview.h"
#include "base/viewport.h"
#include "base/color.h"
#include <climits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

extern "C"
{
  #include "lauxlib.h"
}

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

            static int PushTrue( lua_State *pLuaState )  {

                lua_pushboolean( pLuaState, 1 );

                return 1;
            }

            /**
             * @brief The zoom limits (inclusive positions) of a view, read straight from its viewport.
             */
            static std :: pair<unsigned, unsigned> GetZoomLimitsOf( SunLight :: TileMap :: IView &view )  {

                unsigned  nMinPos = 0;
                unsigned  nMaxPos = 0;

                view.GetViewport().GetZoomLimits( nMinPos, nMaxPos );

                return std :: make_pair( nMinPos, nMaxPos );
            }

            /** @brief factor(p) = (p + 1) x ZOOM_STEP - see sunlight's ZOOM_* constants. */
            static double FactorOfPosition( unsigned nZoomPos )  {

                return ( ( double ) nZoomPos + 1.0 ) * SunLight :: Base :: ZOOM_STEP;
            }

            /**
             * @brief Format a number the way a user would write it (3.8125, 16).
             */
            static std :: string FormatNumber( double fValue )  {

                char  szBuffer[64];

                snprintf( szBuffer, sizeof( szBuffer ), "%g", fValue );

                return szBuffer;
            }

            /**
             * @brief Resolve argument nIndex to a view, without ever creating a renderer: a view
             * handle can only exist once a renderer does, so asking about one first is just an
             * unknown view.
             */
            static std :: shared_ptr<SunLight :: TileMap :: IView> FindView( lua_State *pLuaState, int nIndex, std :: string &strError )  {

                if( lua_type( pLuaState, nIndex ) != LUA_TNUMBER || !lua_isinteger( pLuaState, nIndex ) )  {
                    strError = "expected a view handle (an integer - see renderer_get_default_view)";
                    return std :: shared_ptr<SunLight :: TileMap :: IView>();
                }

                lua_Integer                    nViewId   = lua_tointeger( pLuaState, nIndex );
                Engine :: IRendererProvider   *pProvider = LuaEngineUtil :: GetRendererProvider( pLuaState );

                if( pProvider -> GetOrigin() != Engine :: IRendererProvider :: ORIGIN_NONE )  {
                    std :: shared_ptr<SunLight :: TileMap :: IView>  pView = pProvider -> GetTileMap( "view" ) -> GetView( ( int ) nViewId );

                    if( pView )
                        return pView;
                }

                strError = "unknown view " + std :: to_string( nViewId );

                return std :: shared_ptr<SunLight :: TileMap :: IView>();
            }

            /**
             * @brief Whether a map is loaded. view_move_camera_up/left bound the move by the loaded
             * map's size and dereference it, so with no map loaded sunlight crashed the whole
             * process (found 2026-09-21); they check this first and report an error instead.
             */
            static bool IsMapLoaded( lua_State *pLuaState )  {

                SunLight :: TileMap :: stMapInfo  mapInfo;

                return LuaEngineUtil :: GetRendererProvider( pLuaState ) -> GetTileMap( "view" ) -> GetMapInfo( mapInfo );
            }

            /** @brief Read a required integer argument (a number with an exact integer value). */
            static bool ReadIntegerArg( lua_State *pLuaState, int nIndex, const char *szName, lua_Integer &nOut, std :: string &strError )  {

                int  bIsInteger = 0;

                if( lua_type( pLuaState, nIndex ) == LUA_TNUMBER )
                    nOut = lua_tointegerx( pLuaState, nIndex, &bIsInteger );

                if( !bIsInteger )
                    strError = std :: string( "argument '" ) + szName + "' must be an integer";

                return bIsInteger != 0;
            }

            /** @brief Read a zoom factor argument and convert it to a zoom position. */
            static bool ReadZoomArg( lua_State *pLuaState, int nIndex, const char *szName, unsigned &nZoomPos, std :: string &strError )  {

                if( lua_type( pLuaState, nIndex ) != LUA_TNUMBER )  {
                    strError = std :: string( "argument '" ) + szName + "' must be a number (a zoom factor, eg. 3.8125)";
                    return false;
                }

                if( !LuaRendererApi :: ValidateZoomFactor( lua_tonumber( pLuaState, nIndex ), nZoomPos, strError ) )  {
                    strError = std :: string( szName ) + ": " + strError;
                    return false;
                }

                return true;
            }

            /**
             * @brief Read four integer arguments (x, y, w, h) starting at nFirst as a viewport
             * rectangle and check it against the render area - the rules view_create and
             * view_set_dimension share.
             */
            static bool ReadViewRect( lua_State *pLuaState, int nFirst, SunLight :: TileMap :: stDimension2D &rect, std :: string &strError )  {

                lua_Integer                aRect[4]    = { 0, 0, 0, 0 };
                static const char * const  aszNames[4] = { "x", "y", "w", "h" };

                for( int nArg = 0; nArg < 4; nArg++ )
                    if( !ReadIntegerArg( pLuaState, nFirst + nArg, aszNames[nArg], aRect[nArg], strError ) )
                        return false;

                if( aRect[0] < 0 || aRect[1] < 0 )  {
                    strError = "x and y must be at least 0";
                    return false;
                }

                if( aRect[2] < 1 || aRect[3] < 1 )  {
                    strError = "w and h must be at least 1";
                    return false;
                }

                SunLight :: Renderer :: RendererConfig  config = LuaEngineUtil :: GetRendererProvider( pLuaState ) -> GetEffectiveConfig();

                if( aRect[0] + aRect[2] > ( lua_Integer ) config.fWidth || aRect[1] + aRect[3] > ( lua_Integer ) config.fHeight )  {
                    strError = "the viewport must fit the render area: x+w=" + std :: to_string( aRect[0] + aRect[2] ) +
                        " and y+h=" + std :: to_string( aRect[1] + aRect[3] ) + " must not exceed width " +
                        std :: to_string( ( int ) config.fWidth ) + " and height " + std :: to_string( ( int ) config.fHeight );
                    return false;
                }

                rect.pos.x        = ( int ) aRect[0];
                rect.pos.y        = ( int ) aRect[1];
                rect.size.nWidth  = ( int ) aRect[2];
                rect.size.nHeight = ( int ) aRect[3];

                return true;
            }

            /** @brief Read a required boolean argument. */
            static bool ReadBooleanArg( lua_State *pLuaState, int nIndex, const char *szName, bool &bOut, std :: string &strError )  {

                if( lua_type( pLuaState, nIndex ) != LUA_TBOOLEAN )  {
                    strError = std :: string( "argument '" ) + szName + "' must be a boolean";
                    return false;
                }

                bOut = lua_toboolean( pLuaState, nIndex ) != 0;

                return true;
            }

            /**
             * @luaname{view_create(renderer, x, y, w, h) -> view | nil, message}
             * @luadoc
             * Create an additional view over the same world and return its handle (an integer
             * of at least `1`; `0` is the default view). The rectangle `[x, x + w) × [y, y + h)`
             * is where in the render area the view is drawn: all four are integers, `x`/`y` at
             * least `0`, `w`/`h` at least `1`, and it must fit the render area
             * (`x + w <= width`, `y + h <= height`) — otherwise `nil` and the reason.
             *
             * A new view starts **visible**, with its camera at the map's origin, zoom `1.0`,
             * scroll step the map's tile size, every layer shown, an opaque backdrop in the
             * map's own background colour, and a draw order equal to its own id (so it is drawn
             * on top of the default view, and later views on top of earlier ones). Configure it
             * with the other `view_*` functions — for a minimap that is usually
             * `view_fit_to_map( view )` once the map is loaded. Ids are never reused: a
             * destroyed view's id answers `nil, "unknown view N"` from then on.
             *
             * Each visible extra view costs one more full pass over the map per frame.
             * @luaexample
             * local renderer = renderer_get_current()
             * local minimap, err = view_create( renderer, 1000, 20, 240, 240 )
             * assert( minimap, err )
             * view_fit_to_map( minimap )   -- once a map is loaded
             */
            int LuaViewApi :: ViewCreate( lua_State *pLuaState )  {

                std :: string                          strError;
                SunLight :: TileMap :: stDimension2D   rect;

                if( !LuaRendererApi :: CheckHandle( pLuaState, 1, strError ) || !ReadViewRect( pLuaState, 2, rect, strError ) )
                    return PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = LuaEngineUtil :: GetRendererProvider( pLuaState ) -> GetTileMap( "view_create" ) -> CreateView( rect );

                if( !pView )
                    return PushError( pLuaState, "this renderer's backend could not create the view" );

                lua_pushinteger( pLuaState, pView -> GetId() );

                return 1;
            }

            /**
             * @luaname{view_destroy(view) -> true | nil, message}
             * @luadoc
             * Remove a view: it is no longer drawn and no longer counted
             * (`renderer_get_view_count`). The default view can never be removed
             * (`nil, "the default view cannot be removed"`), and a view that was already
             * destroyed is `nil, "unknown view N"`.
             * @luaexample
             * local ok, err = view_destroy( minimap )
             */
            int LuaViewApi :: ViewDestroy( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( pView -> GetId() == 0 )
                    return PushError( pLuaState, "the default view cannot be removed" );

                if( !LuaEngineUtil :: GetRendererProvider( pLuaState ) -> GetTileMap( "view_destroy" ) -> RemoveView( pView ) )
                    return PushError( pLuaState, "unknown view " + std :: to_string( pView -> GetId() ) );

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_set_zoom(view, factor) -> true | nil, message}
             * @luagroup{view_zoom}
             * @luadoc
             * Set (or read) a view's current zoom **factor**. The factor must be a whole
             * multiple of `ZOOM_STEP` (`0.0625`) within the view's zoom limits (by default
             * `ZOOM_FACTOR_MIN` to `ZOOM_FACTOR_MAX`); anything else returns `nil` and the reason,
             * naming the nearest valid factor. This sets only the *current* zoom —
             * `view_zoom_reset` still returns to the *preferred* zoom (`view_set_preferred_zoom`).
             * The camera is not moved.
             * @luaexample
             * local ok, err = view_set_zoom( view, 3.8125 )
             * assert( ok, err )
             * assert( view_get_zoom( view ) == 3.8125 )
             */
            int LuaViewApi :: ViewSetZoom( lua_State *pLuaState )  {

                std :: string                  strError;
                unsigned                       nZoomPos = 0;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView    = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( !ReadZoomArg( pLuaState, 2, "factor", nZoomPos, strError ) )
                    return PushError( pLuaState, strError );

                std :: pair<unsigned, unsigned>  limits = GetZoomLimitsOf( *pView );

                if( nZoomPos < limits.first || nZoomPos > limits.second )
                    return PushError( pLuaState, "zoom factor " + FormatNumber( lua_tonumber( pLuaState, 2 ) ) +
                        " is outside this view's zoom limits [" + FormatNumber( FactorOfPosition( limits.first ) ) + ", " +
                        FormatNumber( FactorOfPosition( limits.second ) ) + "]" );

                pView -> GetViewport().SetZoom( nZoomPos );

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_get_zoom(view) -> factor | nil, message}
             * @luagroup{view_zoom}
             */
            int LuaViewApi :: ViewGetZoom( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                lua_pushnumber( pLuaState, pView -> GetViewport().GetZoomProperties().fZoomFactor );

                return 1;
            }

            /**
             * @luaname{view_set_preferred_zoom(view, factor) -> true | nil, message}
             * @luagroup{view_preferred_zoom}
             * @luadoc
             * Set (or read) the zoom `view_zoom_reset` returns to. Validated exactly like
             * `view_set_zoom` (a valid factor, within the view's zoom limits).
             * @luaexample
             * view_set_preferred_zoom( view, 1.0 )
             * view_zoom_reset( view )   -- back to 1.0
             */
            int LuaViewApi :: ViewSetPreferredZoom( lua_State *pLuaState )  {

                std :: string                  strError;
                unsigned                       nZoomPos = 0;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView    = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( !ReadZoomArg( pLuaState, 2, "factor", nZoomPos, strError ) )
                    return PushError( pLuaState, strError );

                std :: pair<unsigned, unsigned>  limits = GetZoomLimitsOf( *pView );

                if( nZoomPos < limits.first || nZoomPos > limits.second )
                    return PushError( pLuaState, "zoom factor " + FormatNumber( lua_tonumber( pLuaState, 2 ) ) +
                        " is outside this view's zoom limits [" + FormatNumber( FactorOfPosition( limits.first ) ) + ", " +
                        FormatNumber( FactorOfPosition( limits.second ) ) + "]" );

                pView -> GetViewport().SetPreferredZoom( nZoomPos );

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_get_preferred_zoom(view) -> factor | nil, message}
             * @luagroup{view_preferred_zoom}
             */
            int LuaViewApi :: ViewGetPreferredZoom( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                lua_pushnumber( pLuaState, FactorOfPosition( pView -> GetViewport().GetZoomProperties().nPreferredZoomPos ) );

                return 1;
            }

            /**
             * @luaname{view_set_zoom_limits(view, min_factor, max_factor) -> true | nil, message}
             * @luagroup{view_zoom_limits}
             * @luadoc
             * Set (or read) the lowest and highest zoom **factor** a view may be at (both
             * inclusive). Each must be a valid zoom factor and `min_factor` must not exceed
             * `max_factor`. Limits can be widened again as well as narrowed. Narrowing them
             * clamps the view's current and preferred zoom into the new range. `view_get_zoom_limits`
             * returns both.
             * @luaexample
             * view_set_zoom_limits( view, 1.0, 4.0 )
             * local lo, hi = view_get_zoom_limits( view )   -- 1.0, 4.0
             */
            int LuaViewApi :: ViewSetZoomLimits( lua_State *pLuaState )  {

                std :: string                  strError;
                unsigned                       nMinPos = 0;
                unsigned                       nMaxPos = 0;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView   = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( !ReadZoomArg( pLuaState, 2, "min_factor", nMinPos, strError ) ||
                    !ReadZoomArg( pLuaState, 3, "max_factor", nMaxPos, strError ) )
                    return PushError( pLuaState, strError );

                if( nMinPos > nMaxPos )
                    return PushError( pLuaState, "min_factor " + FormatNumber( lua_tonumber( pLuaState, 2 ) ) +
                        " must not exceed max_factor " + FormatNumber( lua_tonumber( pLuaState, 3 ) ) );

                /*
                 * sunlight rejects a limit that would cross the other one, so the order matters:
                 * when the new minimum is above the current maximum the maximum has to be raised
                 * first, otherwise the minimum can go first. Either way the second call is then
                 * valid, since the new minimum never exceeds the new maximum.
                 */
                std :: pair<unsigned, unsigned>  current = GetZoomLimitsOf( *pView );
                SunLight :: Base :: Viewport    &viewport = pView -> GetViewport();

                if( nMinPos > current.second )  {
                    viewport.SetMaxZoom( nMaxPos );
                    viewport.SetMinZoom( nMinPos );
                }
                else  {
                    viewport.SetMinZoom( nMinPos );
                    viewport.SetMaxZoom( nMaxPos );
                }

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_get_zoom_limits(view) -> min_factor, max_factor | nil, message}
             * @luagroup{view_zoom_limits}
             */
            int LuaViewApi :: ViewGetZoomLimits( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                std :: pair<unsigned, unsigned>  limits = GetZoomLimitsOf( *pView );

                lua_pushnumber( pLuaState, FactorOfPosition( limits.first ) );
                lua_pushnumber( pLuaState, FactorOfPosition( limits.second ) );

                return 2;
            }

            /**
             * @luaname{view_set_zoom_enabled(view, enabled) -> true | nil, message}
             * @luagroup{view_zoom_enabled}
             * @luadoc
             * Enable or disable a view's zoom *stepping* — `view_zoom_in`/`view_zoom_out` (and the
             * global `zoom_in`/`zoom_out` on the default view) do nothing while it is off.
             * Setting an exact zoom with `view_set_zoom` and `view_zoom_reset` still work.
             * `view_get_zoom_enabled` reads it back.
             * @luaexample
             * view_set_zoom_enabled( view, false )   -- lock the zoom against zoom_in/zoom_out
             */
            int LuaViewApi :: ViewSetZoomEnabled( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( lua_type( pLuaState, 2 ) != LUA_TBOOLEAN )
                    return PushError( pLuaState, "argument 'enabled' must be a boolean" );

                pView -> GetViewport().SetEnableUserZoom( lua_toboolean( pLuaState, 2 ) != 0 );

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_get_zoom_enabled(view) -> enabled | nil, message}
             * @luagroup{view_zoom_enabled}
             */
            int LuaViewApi :: ViewGetZoomEnabled( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                lua_pushboolean( pLuaState, pView -> GetViewport().GetEnableUserZoom() ? 1 : 0 );

                return 1;
            }

            /**
             * @luaname{view_zoom_in(view) -> true | nil, message}
             * @luadoc
             * Step a view's zoom in by one `ZOOM_STEP` (a no-op at the zoom limit, or while zoom
             * is disabled — see `view_set_zoom_enabled`).
             * @luaexample
             * view_zoom_in( view )
             */
            int LuaViewApi :: ViewZoomIn( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                pView -> ZoomIn();

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_zoom_out(view) -> true | nil, message}
             * @luadoc
             * Step a view's zoom out by one `ZOOM_STEP` (a no-op at the zoom limit, or while zoom
             * is disabled — see `view_set_zoom_enabled`).
             * @luaexample
             * view_zoom_out( view )
             */
            int LuaViewApi :: ViewZoomOut( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                pView -> ZoomOut();

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_zoom_reset(view) -> true | nil, message}
             * @luadoc
             * Return a view's zoom to its *preferred* zoom (see `view_set_preferred_zoom`) — not
             * to whatever zoom the view was created with.
             * @luaexample
             * view_zoom_reset( view )
             */
            int LuaViewApi :: ViewZoomReset( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                pView -> ResetZoom();

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_set_dimension(view, x, y, w, h) -> true | nil, message}
             * @luagroup{view_dimension}
             * @luadoc
             * Set (or read) a view's viewport: the rectangle `[x, x + w) × [y, y + h)` of the
             * render area it is shown in — `w` and `h` are a real width and height. All four
             * must be integers, `x`/`y` at least `0`, `w`/`h` at least `1`, and the rectangle
             * must fit the render area (`x + w <= width`, `y + h <= height`, the `width`/`height`
             * the renderer was created with). The camera is not moved: set it again
             * (`view_set_camera_position`) if the new rectangle needs a different one.
             * `view_get_dimension` returns `x, y, w, h`.
             * @luaexample
             * local ok, err = view_set_dimension( view, 10, 10, 1240, 900 )
             * assert( ok, err )
             */
            int LuaViewApi :: ViewSetDimension( lua_State *pLuaState )  {

                std :: string                          strError;
                SunLight :: TileMap :: stDimension2D   rect;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( !ReadViewRect( pLuaState, 2, rect, strError ) )
                    return PushError( pLuaState, strError );

                pView -> GetViewport().SetDimension2D( rect );

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_get_dimension(view) -> x, y, w, h | nil, message}
             * @luagroup{view_dimension}
             */
            int LuaViewApi :: ViewGetDimension( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                SunLight :: TileMap :: stDimension2D  &dim = pView -> GetViewport().GetDimension2D();

                lua_pushinteger( pLuaState, dim.pos.x );
                lua_pushinteger( pLuaState, dim.pos.y );
                lua_pushinteger( pLuaState, dim.size.nWidth );
                lua_pushinteger( pLuaState, dim.size.nHeight );

                return 4;
            }

            /**
             * @luaname{view_set_scroll_step(view, w, h) -> true | nil, message}
             * @luagroup{view_scroll_step}
             * @luadoc
             * Set (or read) how many pixels one `view_move_camera_*` call moves a view's camera:
             * `w` horizontally, `h` vertically, both integers of at least `1`.
             * `view_get_scroll_step` returns `w, h` — the step the next move would use. It is
             * `-1` only while no step was ever set *and* no map is loaded yet (the default is the
             * loaded map's tile size).
             * @luaexample
             * view_set_scroll_step( view, 4, 4 )
             * view_move_camera_down( view )   -- moves 4 pixels
             */
            int LuaViewApi :: ViewSetScrollStep( lua_State *pLuaState )  {

                std :: string                  strError;
                lua_Integer                    nStepWidth  = 0;
                lua_Integer                    nStepHeight = 0;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( !ReadIntegerArg( pLuaState, 2, "w", nStepWidth, strError ) ||
                    !ReadIntegerArg( pLuaState, 3, "h", nStepHeight, strError ) )
                    return PushError( pLuaState, strError );

                if( nStepWidth < 1 || nStepHeight < 1 )
                    return PushError( pLuaState, "w and h must be at least 1" );

                pView -> SetScrollStepSize( ( int ) nStepWidth, ( int ) nStepHeight );

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_get_scroll_step(view) -> w, h | nil, message}
             * @luagroup{view_scroll_step}
             */
            int LuaViewApi :: ViewGetScrollStep( lua_State *pLuaState )  {

                std :: string                  strError;
                int                            nStepWidth  = 0;
                int                            nStepHeight = 0;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                pView -> GetScrollStepSize( nStepWidth, nStepHeight );

                lua_pushinteger( pLuaState, nStepWidth );
                lua_pushinteger( pLuaState, nStepHeight );

                return 2;
            }

            /**
             * @luaname{view_set_camera_position(view, x, y) -> true | nil, message}
             * @luagroup{view_camera_position}
             * @luadoc
             * Jump a view's camera so the given world-space coordinate is shown at the top-left of
             * its viewport, and (with `view_get_camera_position`) read it back. Both integers.
             * Unlike the `view_move_camera_*` calls this does **not** clamp to the map, and it is
             * not re-clamped when the view's zoom or viewport changes — set it again afterwards.
             * @luaexample
             * view_set_camera_position( view, 0, 0 )
             * local x, y = view_get_camera_position( view )
             */
            int LuaViewApi :: ViewSetCameraPosition( lua_State *pLuaState )  {

                std :: string                  strError;
                lua_Integer                    nX = 0;
                lua_Integer                    nY = 0;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( !ReadIntegerArg( pLuaState, 2, "x", nX, strError ) || !ReadIntegerArg( pLuaState, 3, "y", nY, strError ) )
                    return PushError( pLuaState, strError );

                pView -> SetCameraPosition( ( int ) nX, ( int ) nY );

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_get_camera_position(view) -> x, y | nil, message}
             * @luagroup{view_camera_position}
             */
            int LuaViewApi :: ViewGetCameraPosition( lua_State *pLuaState )  {

                std :: string                  strError;
                int                            nX = 0;
                int                            nY = 0;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                pView -> GetCameraPosition( nX, nY );

                lua_pushinteger( pLuaState, nX );
                lua_pushinteger( pLuaState, nY );

                return 2;
            }

            /**
             * @luaname{view_move_camera_up(view) -> true | nil, message}
             * @luagroup{view_pan}
             * @luaheading{Moving the camera}
             * @luadoc
             * Move a view's camera up by its scroll step, stopping at the map's edge (unlike
             * `view_set_camera_position`). Also `view_move_camera_down`, `view_move_camera_left`
             * and `view_move_camera_right`. Up and left need a loaded map (their limit is its
             * size): with none, they return `nil, "no map is loaded..."`.
             * @luaexample
             * view_move_camera_up( view )
             */
            int LuaViewApi :: ViewMoveCameraUp( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( !IsMapLoaded( pLuaState ) )
                    return PushError( pLuaState, "no map is loaded: view_move_camera_up needs one (its limit is the map's edge)" );

                pView -> MoveCameraUp();

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_move_camera_down(view) -> true | nil, message}
             * @luagroup{view_pan}
             */
            int LuaViewApi :: ViewMoveCameraDown( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                pView -> MoveCameraDown();

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_move_camera_left(view) -> true | nil, message}
             * @luagroup{view_pan}
             */
            int LuaViewApi :: ViewMoveCameraLeft( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( !IsMapLoaded( pLuaState ) )
                    return PushError( pLuaState, "no map is loaded: view_move_camera_left needs one (its limit is the map's edge)" );

                pView -> MoveCameraLeft();

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_move_camera_right(view) -> true | nil, message}
             * @luagroup{view_pan}
             */
            int LuaViewApi :: ViewMoveCameraRight( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                pView -> MoveCameraRight();

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_reset_camera(view) -> true | nil, message}
             * @luadoc
             * Return a view's camera to the origin of the map.
             * @luaexample
             * view_reset_camera( view )
             */
            int LuaViewApi :: ViewResetCamera( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                pView -> ResetCamera();

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_set_visible(view, visible) -> true | nil, message}
             * @luagroup{view_visible}
             * @luadoc
             * Show or hide a view (and read it back with `view_get_visible`). A hidden view keeps
             * all its state — camera, zoom, layer mask — and is simply skipped by the frame; the
             * sprites on a layer that only hidden views show are not advanced either, exactly as
             * for a hidden layer. Every view is visible when created. Hiding the *default* view
             * hides the game's main picture, which is rarely what you want.
             * @luaexample
             * view_set_visible( minimap, false )   -- toggle the minimap off
             */
            int LuaViewApi :: ViewSetVisible( lua_State *pLuaState )  {

                std :: string                  strError;
                bool                           bVisible = false;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( !ReadBooleanArg( pLuaState, 2, "visible", bVisible, strError ) )
                    return PushError( pLuaState, strError );

                pView -> SetVisible( bVisible );

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_get_visible(view) -> visible | nil, message}
             * @luagroup{view_visible}
             */
            int LuaViewApi :: ViewGetVisible( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                lua_pushboolean( pLuaState, pView -> GetVisible() ? 1 : 0 );

                return 1;
            }

            /**
             * @luaname{view_set_draw_order(view, order) -> true | nil, message}
             * @luagroup{view_draw_order}
             * @luadoc
             * Set the order views are drawn in (and read it back with `view_get_draw_order`):
             * ascending, so a view with a higher order is drawn later — on top of — one with a
             * lower order; views with the same order are drawn by ascending id. `order` is an
             * integer. The default view starts at `0` and a view made by `view_create` starts at
             * its own id, so by default extra views are on top of the default one, later ones on
             * top of earlier ones.
             * @luaexample
             * view_set_draw_order( closeup, 10 )   -- above every view with a lower order
             */
            int LuaViewApi :: ViewSetDrawOrder( lua_State *pLuaState )  {

                std :: string                  strError;
                lua_Integer                    nOrder = 0;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( !ReadIntegerArg( pLuaState, 2, "order", nOrder, strError ) )
                    return PushError( pLuaState, strError );

                if( nOrder < INT_MIN || nOrder > INT_MAX )
                    return PushError( pLuaState, "argument 'order' is out of range" );

                pView -> SetDrawOrder( ( int ) nOrder );

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_get_draw_order(view) -> order | nil, message}
             * @luagroup{view_draw_order}
             */
            int LuaViewApi :: ViewGetDrawOrder( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                lua_pushinteger( pLuaState, pView -> GetDrawOrder() );

                return 1;
            }

            /**
             * @luaname{view_set_clear_background(view, clear) -> true | nil, message}
             * @luagroup{view_clear_background}
             * @luadoc
             * Choose whether a view's rectangle is filled with its background colour before its
             * layers are drawn (and read it back with `view_get_clear_background`). It is on by
             * default. Turn it off for a transparent overlay; leave it on (with an opaque
             * colour, see `view_set_background_color`) so the scene underneath does not show
             * through, which is what a minimap normally wants. For the **default view** this is
             * the whole frame's background, cleared once before any view is drawn.
             * @luaexample
             * view_set_clear_background( hud, false )   -- draw the view over what is below it
             */
            int LuaViewApi :: ViewSetClearBackground( lua_State *pLuaState )  {

                std :: string                  strError;
                bool                           bClear = false;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( !ReadBooleanArg( pLuaState, 2, "clear", bClear, strError ) )
                    return PushError( pLuaState, strError );

                pView -> SetClearBackground( bClear );

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_get_clear_background(view) -> clear | nil, message}
             * @luagroup{view_clear_background}
             */
            int LuaViewApi :: ViewGetClearBackground( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                lua_pushboolean( pLuaState, pView -> GetClearBackground() ? 1 : 0 );

                return 1;
            }

            /**
             * @luaname{view_set_background_color(view, r, g, b [, a]) -> true | nil, message}
             * @luadoc
             * Fill a view's backdrop with this colour instead of the map's own. Each of `r`, `g`,
             * `b` and the optional `a` (default `255`, opaque) is an integer `0`–`255`; an alpha
             * below `255` blends the backdrop over what is already drawn there. It only matters
             * while `view_set_clear_background` is on. There is no getter. For the default view
             * this overrides the whole frame's background colour.
             * @luaexample
             * view_set_background_color( minimap, 0, 0, 0, 160 )   -- translucent black
             */
            int LuaViewApi :: ViewSetBackgroundColor( lua_State *pLuaState )  {

                std :: string                  strError;
                lua_Integer                    aChannel[4]    = { 0, 0, 0, 255 };
                static const char * const      aszNames[4]    = { "r", "g", "b", "a" };
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                int  nChannels = lua_isnoneornil( pLuaState, 5 ) ? 3 : 4;

                for( int nChannel = 0; nChannel < nChannels; nChannel++ )  {
                    if( !ReadIntegerArg( pLuaState, 2 + nChannel, aszNames[nChannel], aChannel[nChannel], strError ) )
                        return PushError( pLuaState, strError );

                    if( aChannel[nChannel] < 0 || aChannel[nChannel] > UCHAR_MAX )
                        return PushError( pLuaState, std :: string( "argument '" ) + aszNames[nChannel] + "' must be between 0 and " + std :: to_string( UCHAR_MAX ) );
                }

                SunLight :: Base :: stColor  color;

                color.nRed   = ( unsigned char ) aChannel[0];
                color.nGreen = ( unsigned char ) aChannel[1];
                color.nBlue  = ( unsigned char ) aChannel[2];
                color.nAlpha = ( unsigned char ) aChannel[3];

                pView -> SetBackgroundColor( color );

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_use_map_background_color(view) -> true | nil, message}
             * @luadoc
             * Go back to the default backdrop colour after `view_set_background_color`: the
             * loaded map's own background colour, or the window's when no map is loaded.
             * @luaexample
             * view_use_map_background_color( minimap )
             */
            int LuaViewApi :: ViewUseMapBackgroundColor( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                pView -> UseMapBackgroundColor();

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_show_layer(view, layer, show) -> true | nil, message}
             * @luadoc
             * Show or hide one layer **in this view only** (the layer itself, and what every
             * other view shows, are untouched). `layer` is the layer's integer id, or its name
             * as a string — a name needs a loaded map with a layer of that name, otherwise
             * `nil` and the reason; an id may be set before the map is loaded (the mask is kept
             * by id, and an id that names no layer is simply never matched). `show` is a
             * boolean. Every layer is shown by default.
             *
             * A hidden layer's sprites are not drawn in this view either — sprites belong to
             * their layer. A hidden **group** layer hides everything inside it whatever its
             * children's own settings, so to show a child, its group must be shown too.
             * @luaexample
             * view_show_layer( minimap, "clouds", false )
             * view_show_layer( minimap, 3, true )
             */
            int LuaViewApi :: ViewShowLayer( lua_State *pLuaState )  {

                std :: string                  strError;
                bool                           bShow = false;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( !ReadBooleanArg( pLuaState, 3, "show", bShow, strError ) )
                    return PushError( pLuaState, strError );

                if( lua_type( pLuaState, 2 ) == LUA_TSTRING )  {
                    const char  *szName = lua_tostring( pLuaState, 2 );

                    if( !pView -> ShowLayer( szName, bShow ) )
                        return PushError( pLuaState, std :: string( "no layer named '" ) + szName + "' (a name needs a loaded map that has it; a layer id works without one)" );

                    return PushTrue( pLuaState );
                }

                lua_Integer  nLayerId = 0;

                if( lua_type( pLuaState, 2 ) != LUA_TNUMBER || !ReadIntegerArg( pLuaState, 2, "layer", nLayerId, strError ) )
                    return PushError( pLuaState, "argument 'layer' must be a layer id (an integer) or a layer name (a string)" );

                pView -> ShowLayer( ( int ) nLayerId, bShow );

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_show_only_layers(view, layer_ids) -> true | nil, message}
             * @luadoc
             * Show **only** the layers whose ids are in the table `layer_ids` (a list of
             * integers) in this view; every other layer — including any added to the map later —
             * is hidden in it. Remember that a hidden group hides its children, so list a
             * child's group as well. An empty table hides every layer. `view_show_all_layers`
             * undoes it.
             * @luaexample
             * view_show_only_layers( minimap, { 1, 2, 5 } )
             */
            int LuaViewApi :: ViewShowOnlyLayers( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: vector<int>             layerIds;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( lua_type( pLuaState, 2 ) != LUA_TTABLE )
                    return PushError( pLuaState, "argument 'layer_ids' must be a table of layer ids" );

                lua_Integer  nCount = ( lua_Integer ) luaL_len( pLuaState, 2 );

                for( lua_Integer nIndex = 1; nIndex <= nCount; nIndex++ )  {
                    int          bIsInteger = 0;
                    lua_Integer  nLayerId   = 0;

                    lua_geti( pLuaState, 2, nIndex );

                    if( lua_type( pLuaState, -1 ) == LUA_TNUMBER )
                        nLayerId = lua_tointegerx( pLuaState, -1, &bIsInteger );

                    lua_pop( pLuaState, 1 );

                    if( !bIsInteger )
                        return PushError( pLuaState, "layer_ids[" + std :: to_string( nIndex ) + "] must be an integer layer id" );

                    layerIds.push_back( ( int ) nLayerId );
                }

                pView -> ShowOnlyLayers( layerIds );

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_show_all_layers(view) -> true | nil, message}
             * @luadoc
             * Clear the view's layer mask: every layer is shown again.
             * @luaexample
             * view_show_all_layers( minimap )
             */
            int LuaViewApi :: ViewShowAllLayers( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                pView -> ShowAllLayers();

                return PushTrue( pLuaState );
            }

            /**
             * @luaname{view_is_layer_shown(view, layer_id) -> shown | nil, message}
             * @luadoc
             * Whether this view's layer mask shows the layer with this integer id. It reports
             * only what the mask says — a layer's own `visible` flag (`tilemap_set_layer`) is a
             * separate thing and applies to every view.
             * @luaexample
             * if not view_is_layer_shown( minimap, 3 ) then print( "layer 3 is masked out" ) end
             */
            int LuaViewApi :: ViewIsLayerShown( lua_State *pLuaState )  {

                std :: string                  strError;
                lua_Integer                    nLayerId = 0;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( !ReadIntegerArg( pLuaState, 2, "layer_id", nLayerId, strError ) )
                    return PushError( pLuaState, strError );

                lua_pushboolean( pLuaState, pView -> IsLayerShown( ( int ) nLayerId ) ? 1 : 0 );

                return 1;
            }

            /**
             * @luaname{view_fit_to_map(view) -> true | nil, message}
             * @luadoc
             * Zoom the view to the largest zoom at which the **whole** loaded map fits inside its
             * rectangle, and put its camera at the map's top-left — the usual setup for a
             * minimap. The zoom is clamped to the view's zoom limits, so a map too large to fit
             * even at the lowest zoom shows as much as that zoom allows. Nothing else about the
             * view changes. With no map loaded it returns `nil, "no map is loaded"`.
             * @luaexample
             * assert( tilemap_load_map( map_path, MAP_ALIGNMENT_TOP_LEFT ) )
             * view_fit_to_map( minimap )
             */
            int LuaViewApi :: ViewFitToMap( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( !pView -> FitToMap() )
                    return PushError( pLuaState, "no map is loaded" );

                return PushTrue( pLuaState );
            }

            /**
             * @brief Register the view-addressed Lua-callable functions.
             */
            void LuaViewApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "view_create", LuaViewApi :: ViewCreate );
                lua_register( pLuaState, "view_destroy", LuaViewApi :: ViewDestroy );
                lua_register( pLuaState, "view_set_zoom", LuaViewApi :: ViewSetZoom );
                lua_register( pLuaState, "view_get_zoom", LuaViewApi :: ViewGetZoom );
                lua_register( pLuaState, "view_set_preferred_zoom", LuaViewApi :: ViewSetPreferredZoom );
                lua_register( pLuaState, "view_get_preferred_zoom", LuaViewApi :: ViewGetPreferredZoom );
                lua_register( pLuaState, "view_set_zoom_limits", LuaViewApi :: ViewSetZoomLimits );
                lua_register( pLuaState, "view_get_zoom_limits", LuaViewApi :: ViewGetZoomLimits );
                lua_register( pLuaState, "view_set_zoom_enabled", LuaViewApi :: ViewSetZoomEnabled );
                lua_register( pLuaState, "view_get_zoom_enabled", LuaViewApi :: ViewGetZoomEnabled );
                lua_register( pLuaState, "view_zoom_in", LuaViewApi :: ViewZoomIn );
                lua_register( pLuaState, "view_zoom_out", LuaViewApi :: ViewZoomOut );
                lua_register( pLuaState, "view_zoom_reset", LuaViewApi :: ViewZoomReset );
                lua_register( pLuaState, "view_set_dimension", LuaViewApi :: ViewSetDimension );
                lua_register( pLuaState, "view_get_dimension", LuaViewApi :: ViewGetDimension );
                lua_register( pLuaState, "view_set_scroll_step", LuaViewApi :: ViewSetScrollStep );
                lua_register( pLuaState, "view_get_scroll_step", LuaViewApi :: ViewGetScrollStep );
                lua_register( pLuaState, "view_set_camera_position", LuaViewApi :: ViewSetCameraPosition );
                lua_register( pLuaState, "view_get_camera_position", LuaViewApi :: ViewGetCameraPosition );
                lua_register( pLuaState, "view_move_camera_up", LuaViewApi :: ViewMoveCameraUp );
                lua_register( pLuaState, "view_move_camera_down", LuaViewApi :: ViewMoveCameraDown );
                lua_register( pLuaState, "view_move_camera_left", LuaViewApi :: ViewMoveCameraLeft );
                lua_register( pLuaState, "view_move_camera_right", LuaViewApi :: ViewMoveCameraRight );
                lua_register( pLuaState, "view_reset_camera", LuaViewApi :: ViewResetCamera );
                lua_register( pLuaState, "view_set_visible", LuaViewApi :: ViewSetVisible );
                lua_register( pLuaState, "view_get_visible", LuaViewApi :: ViewGetVisible );
                lua_register( pLuaState, "view_set_draw_order", LuaViewApi :: ViewSetDrawOrder );
                lua_register( pLuaState, "view_get_draw_order", LuaViewApi :: ViewGetDrawOrder );
                lua_register( pLuaState, "view_set_clear_background", LuaViewApi :: ViewSetClearBackground );
                lua_register( pLuaState, "view_get_clear_background", LuaViewApi :: ViewGetClearBackground );
                lua_register( pLuaState, "view_set_background_color", LuaViewApi :: ViewSetBackgroundColor );
                lua_register( pLuaState, "view_use_map_background_color", LuaViewApi :: ViewUseMapBackgroundColor );
                lua_register( pLuaState, "view_show_layer", LuaViewApi :: ViewShowLayer );
                lua_register( pLuaState, "view_show_only_layers", LuaViewApi :: ViewShowOnlyLayers );
                lua_register( pLuaState, "view_show_all_layers", LuaViewApi :: ViewShowAllLayers );
                lua_register( pLuaState, "view_is_layer_shown", LuaViewApi :: ViewIsLayerShown );
                lua_register( pLuaState, "view_fit_to_map", LuaViewApi :: ViewFitToMap );
            }
        }
    }
}
