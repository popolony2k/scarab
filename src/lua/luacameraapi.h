/*
 * luacameraapi.h
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
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
             */
            class LuaCameraApi  {

                static int MoveCameraUp( lua_State *pLuaState );
                static int MoveCameraDown( lua_State *pLuaState );
                static int MoveCameraLeft( lua_State *pLuaState );
                static int MoveCameraRight( lua_State *pLuaState );
                static int ResetCamera( lua_State *pLuaState );
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
