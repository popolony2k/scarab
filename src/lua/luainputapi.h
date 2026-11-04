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

#include <string>

extern "C"  {
  #include "lua.h"
}

#include "input/iinputhandler.h"
#include "input/icontrollermanager.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Input polling Lua primitives, thin wrapper over IInputHandler.
             *
             * @luacategory{Input}
             * @luadoc
             * The keys, buttons and sticks are polled: each function reads the
             * current state once per call, so call them from your own per-frame
             * update. The one event is a gamepad being connected or disconnected
             * (`input_set_gamepad_connection_handler`). Player slots map a
             * game's own player numbers to gamepads.
             *
             * Every constant below (see the bottom of this page) is
             * registered as a plain Lua global (an integer) — no
             * `require`, no table lookup, just reference the name
             * directly. They match raylib's own
             * `KeyboardKey`/`GamepadButton`/`GamepadAxis`/`ControllerType`
             * enums 1:1.
             */
            class LuaInputApi  {

                static int IsKeyDown( lua_State* pLuaState );
                static int IsKeyUp( lua_State* pLuaState );
                static int IsKeyReleased( lua_State* pLuaState );
                static int IsGamepadButtonDown( lua_State* pLuaState );
                static int IsGamepadButtonUp( lua_State* pLuaState );
                static int GetGamepadAxisMovement( lua_State* pLuaState );
                static int GetGamepadAxisRaw( lua_State* pLuaState );
                static int AddGamepad( lua_State* pLuaState );
                static int GetConnectedGamepads( lua_State* pLuaState );
                static int IsGamepadAvailable( lua_State* pLuaState );
                static int GetGamepadName( lua_State* pLuaState );
                static int SetGamepadConnectionHandler( lua_State* pLuaState );
                static int SetAxisDeadzone( lua_State* pLuaState );
                static int GetAxisDeadzone( lua_State* pLuaState );
                static int ClaimPlayerSlot( lua_State* pLuaState );
                static int ReleasePlayerSlot( lua_State* pLuaState );
                static int GetGamepadForSlot( lua_State* pLuaState );
                static int GetSlotForGamepad( lua_State* pLuaState );
                static int BindActionKey( lua_State* pLuaState );
                static int BindActionButton( lua_State* pLuaState );
                static int BindActionAxis( lua_State* pLuaState );
                static int UnbindAction( lua_State* pLuaState );
                static int GetActionBinding( lua_State* pLuaState );
                static int IsActionActive( lua_State* pLuaState );
                static int SaveActionMap( lua_State* pLuaState );
                static int LoadActionMap( lua_State* pLuaState );
                static int IsRumbleSupported( lua_State* pLuaState );
                static int SetRumble( lua_State* pLuaState );
                static int StopRumble( lua_State* pLuaState );

                static void RegisterEnums( lua_State* pLuaState );

                public:

                // The Lua global LuaEngine::Init stashes its PlayerSlotManager in (a light userdata).
                static constexpr const char*  PLAYER_SLOTS_GLOBAL = "playerSlotsPtr";

                // The Lua globals LuaEngine::Init stashes its ActionMap in, and the HOLDER of its rumble
                // backend (a std::unique_ptr<IGamepadHaptics>, empty until the renderer exists).
                static constexpr const char*  ACTION_MAP_GLOBAL = "actionMapPtr";
                static constexpr const char*  HAPTICS_GLOBAL    = "hapticsHolderPtr";

                static void Register( lua_State* pLuaState );
                static float ApplyDeadzone( float fValue, float fDeadzone );
                static bool IsValidActionName( const std :: string& strName, std :: string& strError );
                static void InstallConnectionHandler( lua_State* pLuaState, SunLight :: Input :: IControllerManager& controllerManager );
                static bool ReadKeyArg( lua_State* pLuaState, int nIndex, const char* szName,
                                        SunLight :: Input :: KeyboardKey& key, std :: string& strError );
            };
        }
    }
}

#endif /* __LUAINPUTAPI_H__ */
