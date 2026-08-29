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

            int LuaCameraApi :: MoveCameraUp( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> MoveCameraUp();

                return 0;
            }

            int LuaCameraApi :: MoveCameraDown( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> MoveCameraDown();

                return 0;
            }

            int LuaCameraApi :: MoveCameraLeft( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> MoveCameraLeft();

                return 0;
            }

            int LuaCameraApi :: MoveCameraRight( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> MoveCameraRight();

                return 0;
            }

            int LuaCameraApi :: ResetCamera( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> ResetCamera();

                return 0;
            }

            int LuaCameraApi :: SetCameraPosition( lua_State *pLuaState )  {

                int  nX = ( int ) lua_tointeger( pLuaState, 1 );
                int  nY = ( int ) lua_tointeger( pLuaState, 2 );

                LuaEngineUtil :: GetTileMap( pLuaState ) -> SetCameraPosition( nX, nY );

                return 0;
            }

            int LuaCameraApi :: GetCameraPosition( lua_State *pLuaState )  {

                int  nX, nY;

                LuaEngineUtil :: GetTileMap( pLuaState ) -> GetCameraPosition( nX, nY );

                lua_pushinteger( pLuaState, nX );
                lua_pushinteger( pLuaState, nY );

                return 2;
            }

            int LuaCameraApi :: ZoomIn( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> ZoomIn();

                return 0;
            }

            int LuaCameraApi :: ZoomOut( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> ZoomOut();

                return 0;
            }

            int LuaCameraApi :: ResetZoom( lua_State *pLuaState )  {

                LuaEngineUtil :: GetTileMap( pLuaState ) -> ResetZoom();

                return 0;
            }

            int LuaCameraApi :: GetViewportDimension( lua_State *pLuaState )  {

                SunLight :: TileMap :: stDimension2D&  dim = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetViewport().GetDimension2D();

                lua_pushinteger( pLuaState, dim.pos.x );
                lua_pushinteger( pLuaState, dim.pos.y );
                lua_pushinteger( pLuaState, dim.size.nWidth );
                lua_pushinteger( pLuaState, dim.size.nHeight );

                return 4;
            }

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
