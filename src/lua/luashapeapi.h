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

#ifndef __LUASHAPEAPI_H__
#define __LUASHAPEAPI_H__

#include <string>
#include <vector>
#include "base/primitives.h"

extern "C"  {
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Shape outlines and points for Lua (plans/MASTER_PLAN.md item C2), drawn through
             * sunlight's IShapeSurface - the same code that draws Tiled shape objects - in map units,
             * with or without a loaded map.
             *
             * @luacategory{Shapes}
             * @luadoc
             * Draw rectangle, ellipse, polyline and polygon outlines and filled points - for debug
             * hitboxes, paths, range rings, selection boxes or simple vector art. Unlike
             * `draw_filled_rectangle` and `draw_text` (screen space, for a HUD), a shape is in **map
             * units**: it follows the default view's camera and zoom, exactly like a Tiled shape
             * object, and the viewport clips it - map or no map, always the view's own camera (whatever
             * the map's layers' parallax). With several views, a shape is drawn once, in the default view.
             *
             * Call them from `on_update`, every frame you want the shape: each call draws once, over
             * the map and the sprites. Colours are `r, g, b, a`, each `0`..`255`, like
             * `draw_filled_rectangle`. The last argument is an optional table
             * `{ thickness = 1, rotation = 0 }`: `thickness` in map units (`1` is a hairline, more is
             * a stroked line), `rotation` in degrees, clockwise. An unknown key or a wrong value is
             * refused. Every function returns `true`, or `nil, "message"` for a wrong argument.
             *
             * ```lua
             * function on_update( dt )
             *     shape_draw_rectangle( enemy.x, enemy.y, 32, 32, 255, 0, 0, 255 )         -- a hitbox
             *     shape_draw_ellipse( tower.x - 96, tower.y - 96, 192, 192, 80, 200, 255, 255, { thickness = 2 } )
             *     shape_draw_polyline( 0, 0, { 10, 10, 120, 40, 200, 10 }, 255, 255, 0, 255 )
             * end
             * ```
             */
            class LuaShapeApi  {

                static int DrawRectangle( lua_State* pLuaState );
                static int DrawEllipse( lua_State* pLuaState );
                static int DrawPolyline( lua_State* pLuaState );
                static int DrawPolygon( lua_State* pLuaState );
                static int DrawPoint( lua_State* pLuaState );

                public:

                /** @brief The fewest points of an open path (a polyline) and of a closed one (a polygon). */
                static constexpr int  MIN_POLYLINE_POINTS = 2;
                static constexpr int  MIN_POLYGON_POINTS  = 3;

                static bool ReadPoints( lua_State* pLuaState, int nIndex, int nMinPoints,
                                        std :: vector<SunLight :: Base :: stVector2D>& points, std :: string& strError );
                static bool ReadOptions( lua_State* pLuaState, int nIndex, double& fThickness, double& fRotation, std :: string& strError );
                static void Register( lua_State* pLuaState );
            };
        }
    }
}

#endif /* __LUASHAPEAPI_H__ */
