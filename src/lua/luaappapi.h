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
             *
             * @luacategory{App}
             * @luadoc
             * Application-level primitives — concerns that belong to the
             * running app/game itself, not to any one engine subsystem
             * (camera, sound, sprites, ...).
             */
            class LuaAppApi  {

                static int SetAppName( lua_State *pLuaState );
                static int ScreenFade( lua_State *pLuaState );
                static int SetFullscreen( lua_State *pLuaState );
                static int GetFullscreen( lua_State *pLuaState );
                static int SetTargetFps( lua_State *pLuaState );
                static int GetTargetFps( lua_State *pLuaState );
                static int SetDrawFps( lua_State *pLuaState );
                static int GetDrawFps( lua_State *pLuaState );
                static int SetWindowResizeable( lua_State *pLuaState );
                static int GetWindowResizeable( lua_State *pLuaState );
                static int SetStretchToFill( lua_State *pLuaState );
                static int GetStretchToFill( lua_State *pLuaState );
                static int DrawFilledRectangle( lua_State *pLuaState );
                static int GetPlatform( lua_State *pLuaState );
                static int Quit( lua_State *pLuaState );

                static void RegisterEnums( lua_State *pLuaState );

                public:

                /**
                 * @brief Host platform Scarab itself was compiled for,
                 * exposed to Lua via app_get_platform() as PLATFORM_*
                 * globals (see RegisterEnums). Deliberately NOT owned by
                 * SunLight::Engines::IEngine (unlike FullscreenStrategy) -
                 * this is a pure compile-time fact (_WIN32/__APPLE__/
                 * __linux__), resolved once by the preprocessor, with no
                 * runtime OS probing and no dependency on raylib/GLFW or
                 * any other engine-owned window/render state, so it
                 * doesn't belong at that layer the way a real runtime
                 * window operation like SetFullscreen does.
                 */
                enum Platform  {
                    PLATFORM_WINDOWS = 0,
                    PLATFORM_MACOS   = 1,
                    PLATFORM_LINUX   = 2,
                    PLATFORM_UNKNOWN = 3  // compiled on something other than the three platforms above
                };

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUAAPPAPI_H__ */
