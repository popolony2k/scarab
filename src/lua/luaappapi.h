/*
 * luaappapi.h
 *
 *  Created on: Jul 11, 2026
 *      Author: popolony2k
 */

#ifndef __LUAAPPAPI_H__
#define __LUAAPPAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Application-level Lua primitives - concerns that belong
             * to the running app/game itself, not to any one engine
             * subsystem (camera, sound, sprites, ...).
             */
            class LuaAppApi  {

                static int SetAppName( lua_State *pLuaState );
                static int ScreenFade( lua_State *pLuaState );
                static int SetFullscreen( lua_State *pLuaState );
                static int GetFullscreen( lua_State *pLuaState );
                static int SetDrawFps( lua_State *pLuaState );
                static int GetDrawFps( lua_State *pLuaState );
                static int SetWindowResizeable( lua_State *pLuaState );
                static int GetWindowResizeable( lua_State *pLuaState );
                static int SetStretchToFill( lua_State *pLuaState );
                static int GetStretchToFill( lua_State *pLuaState );
                static int DrawFilledRectangle( lua_State *pLuaState );

                static void RegisterEnums( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUAAPPAPI_H__ */
