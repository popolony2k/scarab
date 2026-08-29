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

#include "lua/luainputapi.h"
#include "lua/luaengineutil.h"
#include "input/iinputhandler.h"

using namespace SunLight :: Input;

#define __CONST( name )  { #name, ( int ) name }


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            static const stNamedConstant  s_aKeyboardKeys[] = {
                __CONST( KEY_NULL ),
                __CONST( KEY_APOSTROPHE ),
                __CONST( KEY_COMMA ),
                __CONST( KEY_MINUS ),
                __CONST( KEY_PERIOD ),
                __CONST( KEY_SLASH ),
                __CONST( KEY_ZERO ),
                __CONST( KEY_ONE ),
                __CONST( KEY_TWO ),
                __CONST( KEY_THREE ),
                __CONST( KEY_FOUR ),
                __CONST( KEY_FIVE ),
                __CONST( KEY_SIX ),
                __CONST( KEY_SEVEN ),
                __CONST( KEY_EIGHT ),
                __CONST( KEY_NINE ),
                __CONST( KEY_SEMICOLON ),
                __CONST( KEY_EQUAL ),
                __CONST( KEY_A ), __CONST( KEY_B ), __CONST( KEY_C ), __CONST( KEY_D ), __CONST( KEY_E ),
                __CONST( KEY_F ), __CONST( KEY_G ), __CONST( KEY_H ), __CONST( KEY_I ), __CONST( KEY_J ),
                __CONST( KEY_K ), __CONST( KEY_L ), __CONST( KEY_M ), __CONST( KEY_N ), __CONST( KEY_O ),
                __CONST( KEY_P ), __CONST( KEY_Q ), __CONST( KEY_R ), __CONST( KEY_S ), __CONST( KEY_T ),
                __CONST( KEY_U ), __CONST( KEY_V ), __CONST( KEY_W ), __CONST( KEY_X ), __CONST( KEY_Y ), __CONST( KEY_Z ),
                __CONST( KEY_LEFT_BRACKET ),
                __CONST( KEY_BACKSLASH ),
                __CONST( KEY_RIGHT_BRACKET ),
                __CONST( KEY_GRAVE ),
                __CONST( KEY_SPACE ),
                __CONST( KEY_ESCAPE ),
                __CONST( KEY_ENTER ),
                __CONST( KEY_TAB ),
                __CONST( KEY_BACKSPACE ),
                __CONST( KEY_INSERT ),
                __CONST( KEY_DELETE ),
                __CONST( KEY_RIGHT ),
                __CONST( KEY_LEFT ),
                __CONST( KEY_DOWN ),
                __CONST( KEY_UP ),
                __CONST( KEY_PAGE_UP ),
                __CONST( KEY_PAGE_DOWN ),
                __CONST( KEY_HOME ),
                __CONST( KEY_END ),
                __CONST( KEY_CAPS_LOCK ),
                __CONST( KEY_SCROLL_LOCK ),
                __CONST( KEY_NUM_LOCK ),
                __CONST( KEY_PRINT_SCREEN ),
                __CONST( KEY_PAUSE ),
                __CONST( KEY_F1 ), __CONST( KEY_F2 ), __CONST( KEY_F3 ), __CONST( KEY_F4 ),
                __CONST( KEY_F5 ), __CONST( KEY_F6 ), __CONST( KEY_F7 ), __CONST( KEY_F8 ),
                __CONST( KEY_F9 ), __CONST( KEY_F10 ), __CONST( KEY_F11 ), __CONST( KEY_F12 ),
                __CONST( KEY_LEFT_SHIFT ),
                __CONST( KEY_LEFT_CONTROL ),
                __CONST( KEY_LEFT_ALT ),
                __CONST( KEY_LEFT_SUPER ),
                __CONST( KEY_RIGHT_SHIFT ),
                __CONST( KEY_RIGHT_CONTROL ),
                __CONST( KEY_RIGHT_ALT ),
                __CONST( KEY_RIGHT_SUPER ),
                __CONST( KEY_KB_MENU ),
                __CONST( KEY_KP_0 ), __CONST( KEY_KP_1 ), __CONST( KEY_KP_2 ), __CONST( KEY_KP_3 ), __CONST( KEY_KP_4 ),
                __CONST( KEY_KP_5 ), __CONST( KEY_KP_6 ), __CONST( KEY_KP_7 ), __CONST( KEY_KP_8 ), __CONST( KEY_KP_9 ),
                __CONST( KEY_KP_DECIMAL ),
                __CONST( KEY_KP_DIVIDE ),
                __CONST( KEY_KP_MULTIPLY ),
                __CONST( KEY_KP_SUBTRACT ),
                __CONST( KEY_KP_ADD ),
                __CONST( KEY_KP_ENTER ),
                __CONST( KEY_KP_EQUAL ),
                __CONST( KEY_BACK ),
                __CONST( KEY_MENU ),
                __CONST( KEY_VOLUME_UP ),
                __CONST( KEY_VOLUME_DOWN ),
            };

            static const stNamedConstant  s_aGamepadButtons[] = {
                __CONST( GAMEPAD_BUTTON_UNKNOWN ),
                __CONST( GAMEPAD_BUTTON_LEFT_FACE_UP ),
                __CONST( GAMEPAD_BUTTON_LEFT_FACE_RIGHT ),
                __CONST( GAMEPAD_BUTTON_LEFT_FACE_DOWN ),
                __CONST( GAMEPAD_BUTTON_LEFT_FACE_LEFT ),
                __CONST( GAMEPAD_BUTTON_RIGHT_FACE_UP ),
                __CONST( GAMEPAD_BUTTON_RIGHT_FACE_RIGHT ),
                __CONST( GAMEPAD_BUTTON_RIGHT_FACE_DOWN ),
                __CONST( GAMEPAD_BUTTON_RIGHT_FACE_LEFT ),
                __CONST( GAMEPAD_BUTTON_LEFT_TRIGGER_1 ),
                __CONST( GAMEPAD_BUTTON_LEFT_TRIGGER_2 ),
                __CONST( GAMEPAD_BUTTON_RIGHT_TRIGGER_1 ),
                __CONST( GAMEPAD_BUTTON_RIGHT_TRIGGER_2 ),
                __CONST( GAMEPAD_BUTTON_MIDDLE_LEFT ),
                __CONST( GAMEPAD_BUTTON_MIDDLE ),
                __CONST( GAMEPAD_BUTTON_MIDDLE_RIGHT ),
                __CONST( GAMEPAD_BUTTON_LEFT_THUMB ),
                __CONST( GAMEPAD_BUTTON_RIGHT_THUMB ),
            };

            static const stNamedConstant  s_aGamepadAxis[] = {
                __CONST( GAMEPAD_AXIS_LEFT_X ),
                __CONST( GAMEPAD_AXIS_LEFT_Y ),
                __CONST( GAMEPAD_AXIS_RIGHT_X ),
                __CONST( GAMEPAD_AXIS_RIGHT_Y ),
                __CONST( GAMEPAD_AXIS_LEFT_TRIGGER ),
                __CONST( GAMEPAD_AXIS_RIGHT_TRIGGER ),
            };

            static const stNamedConstant  s_aControllerTypes[] = {
                __CONST( CONTROLLER_NULL ),
                __CONST( CONTROLLER_KEYBOARD ),
                __CONST( CONTROLLER_GAMEPAD ),
                __CONST( CONTROLLER_MOUSE ),
                __CONST( CONTROLLER_TOUCH ),
            };

            int LuaInputApi :: IsKeyDown( lua_State *pLuaState )  {

                KeyboardKey  key     = ( KeyboardKey ) lua_tointeger( pLuaState, 1 );
                bool         bResult = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler().IsKeyDown( key );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaInputApi :: IsKeyUp( lua_State *pLuaState )  {

                KeyboardKey  key     = ( KeyboardKey ) lua_tointeger( pLuaState, 1 );
                bool         bResult = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler().IsKeyUp( key );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaInputApi :: IsKeyReleased( lua_State *pLuaState )  {

                KeyboardKey  key     = ( KeyboardKey ) lua_tointeger( pLuaState, 1 );
                bool         bResult = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler().IsKeyReleased( key );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaInputApi :: IsGamepadButtonDown( lua_State *pLuaState )  {

                int            nGamePadId = ( int ) lua_tointeger( pLuaState, 1 );
                GamepadButton  button     = ( GamepadButton ) lua_tointeger( pLuaState, 2 );
                bool           bResult    = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler().IsGamepadButtonDown( nGamePadId, button );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaInputApi :: IsGamepadButtonUp( lua_State *pLuaState )  {

                int            nGamePadId = ( int ) lua_tointeger( pLuaState, 1 );
                GamepadButton  button     = ( GamepadButton ) lua_tointeger( pLuaState, 2 );
                bool           bResult    = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler().IsGamepadButtonUp( nGamePadId, button );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaInputApi :: GetGamepadAxisMovement( lua_State *pLuaState )  {

                int         nGamePadId = ( int ) lua_tointeger( pLuaState, 1 );
                GamepadAxis axis       = ( GamepadAxis ) lua_tointeger( pLuaState, 2 );
                float       fMovement  = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler().GetGamepadAxisMovement( nGamePadId, axis );

                lua_pushnumber( pLuaState, fMovement );

                return 1;
            }

            /**
             * @brief Register a gamepad id with the tile map so it's polled for
             * button/axis state - mirrors the old EngineHost::InitializeGamePads.
             */
            int LuaInputApi :: AddGamepad( lua_State *pLuaState )  {

                int  nGamePadId = ( int ) lua_tointeger( pLuaState, 1 );

                LuaEngineUtil :: GetTileMap( pLuaState ) -> AddGamePad( nGamePadId );

                return 0;
            }

            void LuaInputApi :: RegisterEnums( lua_State *pLuaState )  {

                LuaEngineUtil :: RegisterConstants( pLuaState, s_aKeyboardKeys, sizeof( s_aKeyboardKeys ) / sizeof( s_aKeyboardKeys[0] ) );
                LuaEngineUtil :: RegisterConstants( pLuaState, s_aGamepadButtons, sizeof( s_aGamepadButtons ) / sizeof( s_aGamepadButtons[0] ) );
                LuaEngineUtil :: RegisterConstants( pLuaState, s_aGamepadAxis, sizeof( s_aGamepadAxis ) / sizeof( s_aGamepadAxis[0] ) );
                LuaEngineUtil :: RegisterConstants( pLuaState, s_aControllerTypes, sizeof( s_aControllerTypes ) / sizeof( s_aControllerTypes[0] ) );
            }

            /**
             * @brief Register the input polling Lua-callable functions and enums.
             */
            void LuaInputApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "input_is_key_down", LuaInputApi :: IsKeyDown );
                lua_register( pLuaState, "input_is_key_up", LuaInputApi :: IsKeyUp );
                lua_register( pLuaState, "input_is_key_released", LuaInputApi :: IsKeyReleased );
                lua_register( pLuaState, "input_is_gamepad_button_down", LuaInputApi :: IsGamepadButtonDown );
                lua_register( pLuaState, "input_is_gamepad_button_up", LuaInputApi :: IsGamepadButtonUp );
                lua_register( pLuaState, "input_get_gamepad_axis", LuaInputApi :: GetGamepadAxisMovement );
                lua_register( pLuaState, "input_add_gamepad", LuaInputApi :: AddGamepad );

                RegisterEnums( pLuaState );
            }
        }
    }
}
