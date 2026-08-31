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

#ifndef __LUATIMERAPI_H__
#define __LUATIMERAPI_H__

#include <string>

extern "C"
{
  #include "lua.h"
  #include "lauxlib.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Background-thread periodic/delayed callback Lua
             * primitives (set_timer/reset_timer), backed by
             * SunLight::Concurrent::Timer. See LuaEngineUtil::s_LuaMutex's
             * own comment for the thread-safety invariant every timer
             * callback depends on.
             *
             * @luacategory{Timers}
             * @luadoc
             * Backed by `SunLight::Concurrent::Timer`. Unlike `sp_wait`
             * (which only pauses the `ScriptProcessor` queue), a timer
             * fires its callback repeatedly on its own schedule,
             * independent of the queue, from a **background thread** —
             * see the important thread-safety note below.
             * @luaoutro
             * ## Thread safety — read this before using timers
             *
             * A timer's callback runs on its **own background thread**,
             * not the main game thread. The engine already serializes
             * every one of its own Lua entry points (per-frame update,
             * wave dispatch, etc.) against timer callbacks internally, so
             * calling ordinary engine primitives (`sound_play`,
             * `sprite_get_pos`, ...) from inside a timer callback is safe.
             *
             * What *isn't* automatically safe: **don't call `reset_timer`
             * on a timer's own id from inside that same timer's
             * callback** — behavior in that specific case isn't
             * gracefully handled and this pattern hasn't been exercised.
             * If you need a one-shot timer, prefer a plain counter guard
             * inside the callback (do the work only once, then leave the
             * timer running idle, or cancel it from the main-thread
             * `on_update` instead of from within the callback itself):
             *
             * ```lua
             * local fired = false
             *
             * set_timer(2, 500, function()
             *   if fired then return end
             *   fired = true
             *   print("fires exactly once")
             * end)
             * ```
             */
            class LuaTimerApi  {

                static std :: string GetTimerName( int nId );
                static void DeleteGlobal( lua_State *pLuaState, const char *szVarName );

                static int SetTimer( lua_State *pLuaState );
                static int ResetTimer( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUATIMERAPI_H__ */
