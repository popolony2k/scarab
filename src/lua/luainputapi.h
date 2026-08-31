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

#ifndef __LUAINPUTAPI_H__
#define __LUAINPUTAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Input polling Lua primitives, thin wrapper over IInputHandler.
             *
             * @luacategory{Input}
             * @luadoc
             * All input functions poll current state once per call —
             * there's no event/callback model for input (unlike
             * collisions, which do have a callback — see [collision.md](collision.md)).
             * Call these from your own per-frame update function.
             * @luaoutro
             * ## Constants
             *
             * Every constant below is registered as a plain Lua global
             * (an integer) — no `require`, no table lookup, just
             * reference the name directly. They match raylib's own
             * `KeyboardKey`/`GamepadButton`/`GamepadAxis`/`ControllerType`
             * enums 1:1 (full lists in `src/lua/luainputapi.cpp` if you
             * need one not shown here).
             *
             * **Keyboard** (`KEY_*`) — the full alphabet `KEY_A`..`KEY_Z`,
             * digits `KEY_ZERO`..`KEY_NINE`, function keys
             * `KEY_F1`..`KEY_F12`, arrows
             * `KEY_UP`/`KEY_DOWN`/`KEY_LEFT`/`KEY_RIGHT`, and common named
             * keys: `KEY_SPACE`, `KEY_ENTER`, `KEY_ESCAPE`, `KEY_TAB`,
             * `KEY_BACKSPACE`, `KEY_LEFT_SHIFT`/`KEY_RIGHT_SHIFT`,
             * `KEY_LEFT_CONTROL`/`KEY_RIGHT_CONTROL`.
             *
             * **Gamepad buttons** (`GAMEPAD_BUTTON_*`):
             * `GAMEPAD_BUTTON_LEFT_FACE_UP`/`_DOWN`/`_LEFT`/`_RIGHT`
             * (D-pad), `GAMEPAD_BUTTON_RIGHT_FACE_UP`/`_DOWN`/`_LEFT`/
             * `_RIGHT` (face buttons),
             * `GAMEPAD_BUTTON_LEFT_TRIGGER_1`/`_2`,
             * `GAMEPAD_BUTTON_RIGHT_TRIGGER_1`/`_2`,
             * `GAMEPAD_BUTTON_LEFT_THUMB`/`GAMEPAD_BUTTON_RIGHT_THUMB`,
             * `GAMEPAD_BUTTON_MIDDLE_LEFT`/`_MIDDLE`/`_MIDDLE_RIGHT`.
             *
             * **Gamepad axes** (`GAMEPAD_AXIS_*`):
             * `GAMEPAD_AXIS_LEFT_X`/`_LEFT_Y`,
             * `GAMEPAD_AXIS_RIGHT_X`/`_RIGHT_Y`,
             * `GAMEPAD_AXIS_LEFT_TRIGGER`/`_RIGHT_TRIGGER`.
             *
             * **Controller types** (`CONTROLLER_*`): `CONTROLLER_KEYBOARD`,
             * `CONTROLLER_GAMEPAD`, `CONTROLLER_MOUSE`, `CONTROLLER_TOUCH`,
             * `CONTROLLER_NULL`.
             */
            class LuaInputApi  {

                static int IsKeyDown( lua_State *pLuaState );
                static int IsKeyUp( lua_State *pLuaState );
                static int IsKeyReleased( lua_State *pLuaState );
                static int IsGamepadButtonDown( lua_State *pLuaState );
                static int IsGamepadButtonUp( lua_State *pLuaState );
                static int GetGamepadAxisMovement( lua_State *pLuaState );
                static int AddGamepad( lua_State *pLuaState );

                static void RegisterEnums( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUAINPUTAPI_H__ */
