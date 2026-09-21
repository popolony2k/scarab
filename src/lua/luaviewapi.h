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

#ifndef __LUAVIEWAPI_H__
#define __LUAVIEWAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief View-addressed camera/zoom/viewport Lua primitives.
             *
             * @luacategory{Views}
             * @luadoc
             * A **view** is a camera position, a viewport rectangle, a zoom and a scroll step.
             * The renderer owns one built-in view, the **default view** (id `0`, from
             * `renderer_get_default_view`), and it is the one the older global `camera_*`,
             * `zoom_*` and `viewport_*` functions act on — those keep working unchanged. The
             * `view_*` functions here take the view *first*, so that when more views exist
             * (several views in one window: a minimap, picture-in-picture, split screen) the
             * same calls address any of them. `view_create` adds further views over the same world
             * (a minimap, a close-up, picture-in-picture, split screen), each with its own
             * rectangle, camera and zoom, and each drawn every frame.
             *
             * View handles are opaque integers. Every function reports failure as
             * `nil, "message"` (never a Lua error) — an unknown view is
             * `nil, "unknown view N"`.
             *
             * **Zoom is always a *factor*** — a whole multiple of `ZOOM_STEP` (`0.0625`) between
             * `ZOOM_FACTOR_MIN` and `ZOOM_FACTOR_MAX`; `3.8125` is valid, `3.8` is rejected with
             * the nearest valid factor named. **Changing a view's zoom or viewport does not move
             * its camera:** the camera position is not re-clamped, so set it again afterwards
             * (`view_set_camera_position`).
             *
             * **What an extra view is for, and its limits.** Every view shows the *same* map,
             * sprites and layers — there is one world seen through several windows. A view can
             * hide layers (`view_show_layer`, `view_show_only_layers`), be hidden
             * (`view_set_visible`), be drawn above or below another (`view_set_draw_order`) and
             * have its own backdrop (`view_set_clear_background`, `view_set_background_color`).
             * Each visible extra view costs one more full pass over the map per frame. What is
             * *not* per-view: collisions and input stay in the default view's space, and an
             * ordinary sprite's position is relative to the view it is drawn in — a sprite meant
             * to appear at the same map position in every view needs `sprite_set_world_space`.
             * Image layers are drawn only in the default view.
             */
            class LuaViewApi  {

                static int ViewCreate( lua_State *pLuaState );
                static int ViewDestroy( lua_State *pLuaState );
                static int ViewSetZoom( lua_State *pLuaState );
                static int ViewGetZoom( lua_State *pLuaState );
                static int ViewSetPreferredZoom( lua_State *pLuaState );
                static int ViewGetPreferredZoom( lua_State *pLuaState );
                static int ViewSetZoomLimits( lua_State *pLuaState );
                static int ViewGetZoomLimits( lua_State *pLuaState );
                static int ViewSetZoomEnabled( lua_State *pLuaState );
                static int ViewGetZoomEnabled( lua_State *pLuaState );
                static int ViewZoomIn( lua_State *pLuaState );
                static int ViewZoomOut( lua_State *pLuaState );
                static int ViewZoomReset( lua_State *pLuaState );
                static int ViewSetDimension( lua_State *pLuaState );
                static int ViewGetDimension( lua_State *pLuaState );
                static int ViewSetScrollStep( lua_State *pLuaState );
                static int ViewGetScrollStep( lua_State *pLuaState );
                static int ViewSetCameraPosition( lua_State *pLuaState );
                static int ViewGetCameraPosition( lua_State *pLuaState );
                static int ViewMoveCameraUp( lua_State *pLuaState );
                static int ViewMoveCameraDown( lua_State *pLuaState );
                static int ViewMoveCameraLeft( lua_State *pLuaState );
                static int ViewMoveCameraRight( lua_State *pLuaState );
                static int ViewResetCamera( lua_State *pLuaState );
                static int ViewSetVisible( lua_State *pLuaState );
                static int ViewGetVisible( lua_State *pLuaState );
                static int ViewSetDrawOrder( lua_State *pLuaState );
                static int ViewGetDrawOrder( lua_State *pLuaState );
                static int ViewSetClearBackground( lua_State *pLuaState );
                static int ViewGetClearBackground( lua_State *pLuaState );
                static int ViewSetBackgroundColor( lua_State *pLuaState );
                static int ViewUseMapBackgroundColor( lua_State *pLuaState );
                static int ViewShowLayer( lua_State *pLuaState );
                static int ViewShowOnlyLayers( lua_State *pLuaState );
                static int ViewShowAllLayers( lua_State *pLuaState );
                static int ViewIsLayerShown( lua_State *pLuaState );
                static int ViewFitToMap( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUAVIEWAPI_H__ */
