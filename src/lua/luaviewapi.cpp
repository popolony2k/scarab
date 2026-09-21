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
#include <map>
#include <memory>
#include <string>
#include <utility>

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
             * @brief The zoom limits (inclusive positions) each view was last given.
             *
             * sunlight keeps them private and has no getter, so view_get_zoom_limits could not
             * read them back; every change goes through view_set_zoom_limits, so tracking them
             * here stays accurate. Keyed by view id; a view never given limits has the whole
             * scale. Replace with a sunlight getter if one is added.
             */
            static std :: map<int, std :: pair<unsigned, unsigned> >  s_ZoomLimits;

            static std :: pair<unsigned, unsigned> GetZoomLimitsOf( int nViewId )  {

                std :: map<int, std :: pair<unsigned, unsigned> > :: const_iterator  it = s_ZoomLimits.find( nViewId );

                if( it != s_ZoomLimits.end() )
                    return it -> second;

                return std :: make_pair( SunLight :: Base :: ZOOM_POS_MIN, SunLight :: Base :: ZOOM_POS_MAX );
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
             * @luaname{view_create(renderer, x, y, w, h) -> view | nil, message}
             * @luadoc
             * Create an additional view over the same world. **Not available yet:** only the
             * default view exists, so this always returns `nil, "only one view supported yet"`.
             * The signature is here so scripts can be written against the final shape; more
             * arguments (which layers the view shows, whether it is visible) will be added when
             * views can actually be drawn.
             * @luaexample
             * local view, err = view_create( renderer, 1000, 20, 240, 240 )
             * if not view then print( "no minimap: " .. err ) end
             */
            int LuaViewApi :: ViewCreate( lua_State *pLuaState )  {

                lua_Integer  nHandle = 0;
                std :: string  strError;

                if( !ReadIntegerArg( pLuaState, 1, "renderer", nHandle, strError ) )
                    return PushError( pLuaState, strError );

                return PushError( pLuaState, "only one view supported yet" );
            }

            /**
             * @luaname{view_destroy(view) -> true | nil, message}
             * @luadoc
             * Remove a view. The default view can never be removed
             * (`nil, "the default view cannot be removed"`), and no other view exists yet.
             * @luaexample
             * local ok, err = view_destroy( view )
             */
            int LuaViewApi :: ViewDestroy( lua_State *pLuaState )  {

                std :: string                  strError;
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                if( pView -> GetId() == 0 )
                    return PushError( pLuaState, "the default view cannot be removed" );

                return PushError( pLuaState, "only one view supported yet" );
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

                std :: pair<unsigned, unsigned>  limits = GetZoomLimitsOf( pView -> GetId() );

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

                std :: pair<unsigned, unsigned>  limits = GetZoomLimitsOf( pView -> GetId() );

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
                std :: pair<unsigned, unsigned>  current = GetZoomLimitsOf( pView -> GetId() );
                SunLight :: Base :: Viewport    &viewport = pView -> GetViewport();

                if( nMinPos > current.second )  {
                    viewport.SetMaxZoom( nMaxPos );
                    viewport.SetMinZoom( nMinPos );
                }
                else  {
                    viewport.SetMinZoom( nMinPos );
                    viewport.SetMaxZoom( nMaxPos );
                }

                s_ZoomLimits[pView -> GetId()] = std :: make_pair( nMinPos, nMaxPos );

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

                std :: pair<unsigned, unsigned>  limits = GetZoomLimitsOf( pView -> GetId() );

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

                std :: string                  strError;
                lua_Integer                    aRect[4] = { 0, 0, 0, 0 };
                static const char * const      aszNames[4] = { "x", "y", "w", "h" };
                std :: shared_ptr<SunLight :: TileMap :: IView>  pView = FindView( pLuaState, 1, strError );

                if( !pView )
                    return PushError( pLuaState, strError );

                for( int nArg = 0; nArg < 4; nArg++ )
                    if( !ReadIntegerArg( pLuaState, 2 + nArg, aszNames[nArg], aRect[nArg], strError ) )
                        return PushError( pLuaState, strError );

                if( aRect[0] < 0 || aRect[1] < 0 )
                    return PushError( pLuaState, "x and y must be at least 0" );

                if( aRect[2] < 1 || aRect[3] < 1 )
                    return PushError( pLuaState, "w and h must be at least 1" );

                SunLight :: Renderer :: RendererConfig  config = LuaEngineUtil :: GetRendererProvider( pLuaState ) -> GetEffectiveConfig();

                if( aRect[0] + aRect[2] > ( lua_Integer ) config.fWidth || aRect[1] + aRect[3] > ( lua_Integer ) config.fHeight )
                    return PushError( pLuaState, "the viewport must fit the render area: x+w=" + std :: to_string( aRect[0] + aRect[2] ) +
                        " and y+h=" + std :: to_string( aRect[1] + aRect[3] ) + " must not exceed width " +
                        std :: to_string( ( int ) config.fWidth ) + " and height " + std :: to_string( ( int ) config.fHeight ) );

                SunLight :: TileMap :: stDimension2D  rect;

                rect.pos.x        = ( int ) aRect[0];
                rect.pos.y        = ( int ) aRect[1];
                rect.size.nWidth  = ( int ) aRect[2];
                rect.size.nHeight = ( int ) aRect[3];

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
             * @luaheading{view_move_camera_up}
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
             * @luaheading{view_move_camera_up}
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
             * @luaheading{view_move_camera_up}
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
            }
        }
    }
}
