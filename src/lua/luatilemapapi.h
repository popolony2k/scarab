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

#ifndef __LUATILEMAPAPI_H__
#define __LUATILEMAPAPI_H__

extern "C"  {
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Tile map management Lua primitives, thin wrapper over the full ITileMap surface.
             *
             * @luacategory{Tile map}
             * @luadoc
             * Wraps sunlight's `ITileMap` (Tiled `.tmx` maps via libtmx).
             *
             * ## Error reporting
             *
             * Every tilemap function reports a failure as `nil, "message"`
             * and never raises a Lua error; one with nothing else to return
             * returns `true`. With no map loaded, the queries fail with
             * `"no map is loaded"`; an unknown layer is
             * `"no layer N in the loaded map"`.
             * @luaoutro
             * ## Tiled custom properties
             *
             * sunlight reads a few **custom properties** a map author adds in Tiled: select the
             * object or layer, open the *Properties* panel, choose *Add Property*, and give it
             * exactly the name and type below. Tiled saves them in the map (eg.
             * `<property name="screen_space" type="bool" value="true"/>`) but gives them no meaning
             * of its own; only sunlight uses them. Nothing in Lua sets them: they are part of the map.
             *
             * | Property | Type | Set on | What it does |
             * |---|---|---|---|
             * | `screen_space` | bool | an image layer | When `true`, the layer is drawn in **screen space**: at its offset in window pixels, at its own size, not moved by the camera, the zoom or the layer's parallax, and not cut to the viewport - for a full-screen picture such as a title or cover screen. It keeps its place in the layer order (sprites on it and above it draw over it), its opacity and its tint. Like every image layer, it is drawn in the default view only. Without it (or `false`), an image layer is drawn in map space, like the rest of the map. |
             * | `line_width` | int or float | a rectangle, ellipse, polyline or polygon object | Draws its outline this many map pixels wide (`1` when absent), scaled with the zoom. |
             * | `point_size` | int or float | a point object | Draws the point this size (`1` when absent). |
             * | `opacity` | int or float, `0` to `1` | any object | The object's own opacity, instead of its layer's (Tiled has no per-object opacity). A value of another type or outside `0` to `1` is ignored, with a warning, and the layer's opacity is used. |
             *
             * Image layers changed with sunlight's L1: every image layer used to be drawn in screen
             * space, and is drawn in map space since (moved by the camera, scaled by the zoom,
             * offset by the viewport). A map made for the old drawing - a full-window cover image,
             * say - sets `screen_space` on that layer. `samples/backdrop` shows one.
             */
            class LuaTilemapApi  {

                static int LoadMap( lua_State* pLuaState );
                static int UnloadMap( lua_State* pLuaState );
                static int GetMapInfo( lua_State* pLuaState );
                static int GetLayer( lua_State* pLuaState );
                static int GetLayerByName( lua_State* pLuaState );
                static int SetLayer( lua_State* pLuaState );
                static int SetLayerByName( lua_State* pLuaState );
                static int GetLayerTint( lua_State* pLuaState );
                static int GetLayerTintByName( lua_State* pLuaState );
                static int SetLayerTint( lua_State* pLuaState );
                static int SetLayerTintByName( lua_State* pLuaState );
                static int GetLayerParallax( lua_State* pLuaState );
                static int GetLayerParallaxByName( lua_State* pLuaState );
                static int SetLayerParallax( lua_State* pLuaState );
                static int SetLayerParallaxByName( lua_State* pLuaState );
                static int GetParallaxOrigin( lua_State* pLuaState );
                static int GetTile( lua_State* pLuaState );
                static int ToTileMatrix( lua_State* pLuaState );
                static int GetObjectByName( lua_State* pLuaState );
                static int GetTileFrame( lua_State* pLuaState );
                static int GetObjectTileFrame( lua_State* pLuaState );

                static void RegisterEnums( lua_State* pLuaState );

                public:

                static void Register( lua_State* pLuaState );
            };
        }
    }
}

#endif /* __LUATILEMAPAPI_H__ */
