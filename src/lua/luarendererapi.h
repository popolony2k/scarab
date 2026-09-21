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

#ifndef __LUARENDERERAPI_H__
#define __LUARENDERERAPI_H__

#include <string>

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Renderer lifecycle Lua primitives: create the process's one renderer from
             * a script, inspect it, ask it to stop.
             *
             * @luacategory{Renderer}
             * @luadoc
             * By default a game never has to think about the renderer: the first call that
             * needs a window (`app_set_name`, `set_font`, `tilemap_load_map`, ...) creates one
             * with Scarab's default configuration, and a script that needs none still gets it
             * when it finishes. `renderer_create` is for a game that wants to choose its own
             * window/render settings up front.
             *
             * **Ordering rule — `renderer_create` must be the first window-needing call in the
             * entry script.** The entry script runs *before* any window exists, and
             * `renderer_create` opens the window the moment it succeeds. Anything that needs a
             * window before it has run creates the default renderer first, and a later
             * `renderer_create` then fails with `nil, "renderer already created: ..."` naming
             * the call that did it. Calls that need no window are fine before it:
             * `load_json`, `dofile`, `sound_*`, `sp_*`, `pool_*`, the `sprite_*` handle calls,
             * `set_timer`, `collision_set_handler`, `crypto_*`, `pack_*`, `app_get_platform`.
             *
             * One renderer per process run. Every function here reports failure as
             * `nil, "message"` — it never raises a Lua error. Handles are opaque integers.
             */
            class LuaRendererApi  {

                static int RendererCreate( lua_State *pLuaState );
                static int RendererDestroy( lua_State *pLuaState );
                static int RendererGetCurrent( lua_State *pLuaState );
                static int RendererGetConfig( lua_State *pLuaState );
                static int RendererGetDefaultView( lua_State *pLuaState );
                static int RendererGetBackend( lua_State *pLuaState );
                static int RendererGetViewCount( lua_State *pLuaState );

                static void RegisterEnums( lua_State *pLuaState );

                public:

                static bool CheckHandle( lua_State *pLuaState, int nIndex, std :: string &strError );
                static bool ValidateZoomFactor( double fFactor, unsigned &nZoomPos, std :: string &strError );
                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUARENDERERAPI_H__ */
