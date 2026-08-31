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

#include "lua/luacameraapi.h"
#include "lua/luaengineutil.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @luaname{camera_move_up()}
             * @luagroup{panning}
             * @luaheading{Panning}
             * @luadoc
             * Each call moves the camera one fixed step in that direction
             * (the step size is configured on the C++ renderer, not from
             * Lua — see `TileMapRenderer::SetScrollStepSize` in
             * `main.cpp`).
             *
             * Caravellius drives a constant downward auto-scroll from
             * `caravellius/src/camera.lua`'s `Camera.update`, called
             * every frame via `Enemies.register_update` — that's
             * ordinary game script using `camera_move_down()`, not a
             * separate engine mechanism.
             * @luaexample
             * camera_move_up()
             * camera_move_down()
             * camera_move_left()
             * camera_move_right()
             *
             * -- src/camera.lua (simplified)
             * local MAX_FPS_PER_SCROLL = 2
             * local fpsCount = 1
             *
             * function Camera.update(dt)
             *   if fpsCount == MAX_FPS_PER_SCROLL then
             *     camera_move_down()
             *     fpsCount = 1
             *   else
             *     fpsCount = fpsCount + 1
             *   end
             * end
             */
            int LuaCameraApi :: MoveCameraUp( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> MoveCameraUp();

                return 0;
            }

            /**
             * @luaname{camera_move_down()}
             * @luagroup{panning}
             */
            int LuaCameraApi :: MoveCameraDown( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> MoveCameraDown();

                return 0;
            }

            /**
             * @luaname{camera_move_left()}
             * @luagroup{panning}
             */
            int LuaCameraApi :: MoveCameraLeft( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> MoveCameraLeft();

                return 0;
            }

            /**
             * @luaname{camera_move_right()}
             * @luagroup{panning}
             */
            int LuaCameraApi :: MoveCameraRight( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> MoveCameraRight();

                return 0;
            }

            /**
             * @luaname{camera_reset()}
             * @luadoc
             * Reset the camera to the map's default position.
             */
            int LuaCameraApi :: ResetCamera( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> ResetCamera();

                return 0;
            }

            /**
             * @luaname{camera_set_position(x, y)}
             * @luadoc
             * Jump the camera so the given world-space coordinate
             * (pixels) is shown at the top-left of the viewport — an
             * absolute move, unlike `camera_move_*`'s incremental
             * one-step nudges, and unlike `camera_reset()` (which only
             * returns to the map's original load-time position). Does
             * **not** clamp to map boundaries — the caller is responsible
             * for passing a valid target, e.g. a position read via
             * `tilemap_get_object_by_name`.
             * @luaexample
             * local x, y, w, h = tilemap_get_object_by_name("SubBossIntervalStart")
             * camera_set_position(x, y)
             */
            int LuaCameraApi :: SetCameraPosition( lua_State *pLuaState )  {

                int  nX = ( int ) lua_tointeger( pLuaState, 1 );
                int  nY = ( int ) lua_tointeger( pLuaState, 2 );

                LuaEngineUtil :: GetTileMap( pLuaState ) -> SetCameraPosition( nX, nY );

                return 0;
            }

            /**
             * @luaname{camera_get_position() -> x, y}
             * @luadoc
             * Read the world-space coordinate currently shown at the
             * top-left of the viewport — the exact inverse of
             * `camera_set_position`, and the only reliable way to know
             * the camera's current scroll position (there's no way to
             * derive it purely from map/viewport size — the load-time
             * alignment math isn't part of the public contract).
             * @luaexample
             * local x, y = camera_get_position()
             */
            int LuaCameraApi :: GetCameraPosition( lua_State *pLuaState )  {

                int  nX, nY;

                LuaEngineUtil :: GetTileMap( pLuaState ) -> GetCameraPosition( nX, nY );

                lua_pushinteger( pLuaState, nX );
                lua_pushinteger( pLuaState, nY );

                return 2;
            }

            /**
             * @luaname{zoom_in()}
             * @luagroup{zoom}
             * @luaheading{Zoom}
             * @luaexample
             * zoom_in()
             * zoom_out()
             * zoom_reset()
             */
            int LuaCameraApi :: ZoomIn( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> ZoomIn();

                return 0;
            }

            /**
             * @luaname{zoom_out()}
             * @luagroup{zoom}
             */
            int LuaCameraApi :: ZoomOut( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> ZoomOut();

                return 0;
            }

            /**
             * @luaname{zoom_reset()}
             * @luagroup{zoom}
             */
            int LuaCameraApi :: ResetZoom( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> ResetZoom();

                return 0;
            }

            /**
             * @luaname{viewport_get_dimension() -> x, y, width, height}
             * @luadoc
             * Read the viewport's current position and size (screen-space
             * pixels).
             * @luaexample
             * local x, y, w, h = viewport_get_dimension()
             * print(("viewport: %d,%d %dx%d"):format(x, y, w, h))
             */
            int LuaCameraApi :: GetViewportDimension( lua_State *pLuaState )  {

                SunLight :: TileMap :: stDimension2D&  dim = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetViewport().GetDimension2D();

                lua_pushinteger( pLuaState, dim.pos.x );
                lua_pushinteger( pLuaState, dim.pos.y );
                lua_pushinteger( pLuaState, dim.size.nWidth );
                lua_pushinteger( pLuaState, dim.size.nHeight );

                return 4;
            }

            /**
             * @luaname{viewport_get_zoom_factor() -> factor}
             * @luadoc
             * Read the current zoom multiplier (`1.0` = no zoom).
             * @luaexample
             * local zoom = viewport_get_zoom_factor()
             */
            int LuaCameraApi :: GetViewportZoomFactor( lua_State *pLuaState )  {

                float  fZoomFactor = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetViewport().GetZoomProperties().fZoomFactor;

                lua_pushnumber( pLuaState, fZoomFactor );

                return 1;
            }

            /**
             * @brief Register the camera/viewport Lua-callable functions.
             */
            void LuaCameraApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "camera_move_up", LuaCameraApi :: MoveCameraUp );
                lua_register( pLuaState, "camera_move_down", LuaCameraApi :: MoveCameraDown );
                lua_register( pLuaState, "camera_move_left", LuaCameraApi :: MoveCameraLeft );
                lua_register( pLuaState, "camera_move_right", LuaCameraApi :: MoveCameraRight );
                lua_register( pLuaState, "camera_reset", LuaCameraApi :: ResetCamera );
                lua_register( pLuaState, "camera_set_position", LuaCameraApi :: SetCameraPosition );
                lua_register( pLuaState, "camera_get_position", LuaCameraApi :: GetCameraPosition );
                lua_register( pLuaState, "zoom_in", LuaCameraApi :: ZoomIn );
                lua_register( pLuaState, "zoom_out", LuaCameraApi :: ZoomOut );
                lua_register( pLuaState, "zoom_reset", LuaCameraApi :: ResetZoom );
                lua_register( pLuaState, "viewport_get_dimension", LuaCameraApi :: GetViewportDimension );
                lua_register( pLuaState, "viewport_get_zoom_factor", LuaCameraApi :: GetViewportZoomFactor );
            }
        }
    }
}
