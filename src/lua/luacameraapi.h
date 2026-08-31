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

#ifndef __LUACAMERAAPI_H__
#define __LUACAMERAAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Camera/viewport/zoom Lua primitives, thin wrappers over ITileMap.
             *
             * @luacategory{Camera}
             * @luadoc
             * All camera functions act on the currently loaded map's
             * viewport (`tilemap_load_map`) — there's no separate "camera
             * object" to create or select.
             */
            class LuaCameraApi  {

                static int MoveCameraUp( lua_State *pLuaState );
                static int MoveCameraDown( lua_State *pLuaState );
                static int MoveCameraLeft( lua_State *pLuaState );
                static int MoveCameraRight( lua_State *pLuaState );
                static int ResetCamera( lua_State *pLuaState );
                static int SetCameraPosition( lua_State *pLuaState );
                static int GetCameraPosition( lua_State *pLuaState );
                static int ZoomIn( lua_State *pLuaState );
                static int ZoomOut( lua_State *pLuaState );
                static int ResetZoom( lua_State *pLuaState );
                static int GetViewportDimension( lua_State *pLuaState );
                static int GetViewportZoomFactor( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUACAMERAAPI_H__ */
