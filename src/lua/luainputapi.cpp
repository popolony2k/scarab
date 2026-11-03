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
#include "input/playerslotmanager.h"
#include <climits>
#include <cstdio>
#include <mutex>
#include <vector>


#define __CONST( name )  { #name, ( int ) SunLight :: Input :: name }

/** The Lua global input_set_gamepad_connection_handler keeps its function in. */
#define __CONNECTION_HANDLER_GLOBAL  "__gamepad_connection_handler"

/** A dead zone is a distance from the stick's centre: 0 (none) to 1 (the whole travel). */
#define __MAX_AXIS_DEADZONE          1.0


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @luaconstants{Keyboard keys}
             * @luadoc
             * The `key` argument `input_is_key_down`/`input_is_key_up`/
             * `input_is_key_released` take - every one raylib itself
             * supports, with raylib's own values. `KEY_BACK`, `KEY_MENU`,
             * `KEY_VOLUME_UP` and `KEY_VOLUME_DOWN` are Android buttons: a
             * desktop keyboard never sends them.
             */
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

            /**
             * @luaconstants{Gamepad buttons}
             * @luadoc
             * The `button` argument `input_is_gamepad_button_down`/
             * `input_is_gamepad_button_up` take. **Naming gotcha**:
             * raylib's own gamepad button naming (mirrored here exactly)
             * calls the D-pad `LEFT_FACE_UP`/`DOWN`/`LEFT`/`RIGHT` - the
             * face buttons (A/B/X/Y or equivalent) are `RIGHT_FACE_*`.
             * Easy to mix up the two at a glance.
             */
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

            /**
             * @luaconstants{Gamepad axes}
             * @luadoc
             * The `axis` argument `input_get_gamepad_axis` takes.
             */
            static const stNamedConstant  s_aGamepadAxis[] = {
                __CONST( GAMEPAD_AXIS_LEFT_X ),
                __CONST( GAMEPAD_AXIS_LEFT_Y ),
                __CONST( GAMEPAD_AXIS_RIGHT_X ),
                __CONST( GAMEPAD_AXIS_RIGHT_Y ),
                __CONST( GAMEPAD_AXIS_LEFT_TRIGGER ),
                __CONST( GAMEPAD_AXIS_RIGHT_TRIGGER ),
            };

            /**
             * @luaconstants{Controller types}
             * @luadoc
             * Mirrors raylib's own `ControllerType` for completeness —
             * no primitive in this file actually takes one as an
             * argument today.
             */
            static const stNamedConstant  s_aControllerTypes[] = {
                __CONST( CONTROLLER_NULL ),
                __CONST( CONTROLLER_KEYBOARD ),
                __CONST( CONTROLLER_GAMEPAD ),
                __CONST( CONTROLLER_MOUSE ),
                __CONST( CONTROLLER_TOUCH ),
            };

            /**
             * @brief Read a key argument (a KEY_* constant) - shared with app_set_exit_key.
             */
            bool LuaInputApi :: ReadKeyArg( lua_State* pLuaState, int nIndex, const char* szName,
                                            SunLight :: Input :: KeyboardKey& key, std :: string& strError )  {

                int  nKey = 0;

                if( !LuaEngineUtil :: ReadConstantArg( pLuaState, nIndex, szName, s_aKeyboardKeys,
                                                       sizeof( s_aKeyboardKeys ) / sizeof( s_aKeyboardKeys[0] ), "KEY_*", nKey, strError ) )
                    return false;

                key = ( SunLight :: Input :: KeyboardKey ) nKey;

                return true;
            }

            /** @brief Read a gamepad id argument: 0 to MAX_GAMEPAD_COUNT - 1. */
            static bool ReadGamepadIdArg( lua_State* pLuaState, int nIndex, int& nGamePadId, std :: string& strError )  {

                lua_Integer  nValue = 0;

                if( !LuaEngineUtil :: ReadIntegerArgInRange( pLuaState, nIndex, "gamepadId", 0, MAX_GAMEPAD_COUNT - 1, nValue, strError ) )
                    return false;

                nGamePadId = ( int ) nValue;

                return true;
            }

            /**
             * @brief Read a GAMEPAD_AXIS_* constant argument.
             */
            static bool ReadAxisArg( lua_State* pLuaState, int nIndex, SunLight :: Input :: GamepadAxis& axis, std :: string& strError )  {

                int  nAxis = 0;

                if( !LuaEngineUtil :: ReadConstantArg( pLuaState, nIndex, "axis", s_aGamepadAxis, sizeof( s_aGamepadAxis ) / sizeof( s_aGamepadAxis[0] ),
                                                       "GAMEPAD_AXIS_*", nAxis, strError ) )
                    return false;

                axis = ( SunLight :: Input :: GamepadAxis ) nAxis;

                return true;
            }

            /**
             * @brief Whether an axis is a stick's (LEFT_X/Y, RIGHT_X/Y) - the ones a dead zone applies
             * to. A trigger rests at -1, not at the centre, so a dead zone around 0 would cut its travel
             * in the middle; sunlight applies its dead zones to the sticks only, too.
             */
            static bool IsStickAxis( SunLight :: Input :: GamepadAxis axis )  {

                return ( axis == SunLight :: Input :: GAMEPAD_AXIS_LEFT_X ) || ( axis == SunLight :: Input :: GAMEPAD_AXIS_LEFT_Y ) ||
                       ( axis == SunLight :: Input :: GAMEPAD_AXIS_RIGHT_X ) || ( axis == SunLight :: Input :: GAMEPAD_AXIS_RIGHT_Y );
            }

            /** @brief Read a gamepad id and a stick axis (arguments 1 and 2) - a trigger axis is refused. */
            static bool ReadStickArgs( lua_State* pLuaState, int& nGamePadId, SunLight :: Input :: GamepadAxis& axis, std :: string& strError )  {

                if( !ReadGamepadIdArg( pLuaState, 1, nGamePadId, strError ) || !ReadAxisArg( pLuaState, 2, axis, strError ) )
                    return false;

                if( !IsStickAxis( axis ) )  {
                    strError = "argument 'axis' must be a stick axis (GAMEPAD_AXIS_LEFT_X/_Y, GAMEPAD_AXIS_RIGHT_X/_Y): "
                               "a dead zone does not apply to a trigger";
                    return false;
                }

                return true;
            }

            /** @brief Read a player slot number argument: 0 or more (the game's own numbering). */
            static bool ReadSlotArg( lua_State* pLuaState, int nIndex, int& nSlot, std :: string& strError )  {

                lua_Integer  nValue = 0;

                if( !LuaEngineUtil :: ReadIntegerArgInRange( pLuaState, nIndex, "slot", 0, INT_MAX, nValue, strError ) )
                    return false;

                nSlot = ( int ) nValue;

                return true;
            }

            /**
             * @brief The engine's player slots, stashed by LuaEngine::Init as the light userdata global
             * PLAYER_SLOTS_GLOBAL.
             *
             * @param pLuaState Lua state to be used by engine call;
             */
            static SunLight :: Input :: PlayerSlotManager* GetPlayerSlots( lua_State* pLuaState )  {

                return static_cast<SunLight :: Input :: PlayerSlotManager *>( LuaEngineUtil :: GetGlobalPointer( pLuaState, LuaInputApi :: PLAYER_SLOTS_GLOBAL ) );
            }

            /** @brief Read a gamepad id and a GAMEPAD_BUTTON_* constant (arguments 1 and 2). */
            static bool ReadGamepadButtonArgs( lua_State* pLuaState, int& nGamePadId, SunLight :: Input :: GamepadButton& button, std :: string& strError )  {

                int  nButton = 0;

                if( !ReadGamepadIdArg( pLuaState, 1, nGamePadId, strError ) ||
                    !LuaEngineUtil :: ReadConstantArg( pLuaState, 2, "button", s_aGamepadButtons,
                                                       sizeof( s_aGamepadButtons ) / sizeof( s_aGamepadButtons[0] ),
                                                       "GAMEPAD_BUTTON_*", nButton, strError ) )
                    return false;

                button = ( SunLight :: Input :: GamepadButton ) nButton;

                return true;
            }

            /**
             * @luaname{input_is_key_down(key) -> down | nil, message}
             * @luagroup{keyboard}
             * @luaheading{Keyboard}
             * @luadoc
             * `key` must be one of the `KEY_*` constants: anything else
             * (a number no key has, a string, `nil` — eg. a misspelt
             * constant name, which Lua reads as `nil`) returns
             * `nil, "message"` instead of quietly reading "not pressed".
             * The gamepad functions check their arguments the same way:
             * `gamepadId` is `0` to `9`, and `button`/`axis` must be
             * `GAMEPAD_BUTTON_*`/`GAMEPAD_AXIS_*` constants.
             * @luaexample
             * input_is_key_down(key) -> down       -- true every frame the key is held
             * input_is_key_up(key) -> up           -- true every frame the key is NOT held
             * input_is_key_released(key) -> released -- true only on the frame the key transitions from down to up
             *
             * function Player.update(dt)
             *   if input_is_key_down(KEY_LEFT) then
             *     -- move left
             *   end
             * end
             */
            int LuaInputApi :: IsKeyDown( lua_State* pLuaState )  {

                std :: string                       strError;
                SunLight :: Input :: KeyboardKey    key = SunLight :: Input :: KeyboardKey :: KEY_NULL;

                if( !ReadKeyArg( pLuaState, 1, "key", key, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushboolean( pLuaState, LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler().IsKeyDown( key ) );

                return 1;
            }

            /**
             * @luaname{input_is_key_up(key) -> up | nil, message}
             * @luagroup{keyboard}
             */
            int LuaInputApi :: IsKeyUp( lua_State* pLuaState )  {

                std :: string                       strError;
                SunLight :: Input :: KeyboardKey    key = SunLight :: Input :: KeyboardKey :: KEY_NULL;

                if( !ReadKeyArg( pLuaState, 1, "key", key, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushboolean( pLuaState, LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler().IsKeyUp( key ) );

                return 1;
            }

            /**
             * @luaname{input_is_key_released(key) -> released | nil, message}
             * @luagroup{keyboard}
             */
            int LuaInputApi :: IsKeyReleased( lua_State* pLuaState )  {

                std :: string                       strError;
                SunLight :: Input :: KeyboardKey    key = SunLight :: Input :: KeyboardKey :: KEY_NULL;

                if( !ReadKeyArg( pLuaState, 1, "key", key, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushboolean( pLuaState, LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler().IsKeyReleased( key ) );

                return 1;
            }

            /**
             * @luaname{input_is_gamepad_button_down(gamepadId, button) -> down | nil, message}
             * @luagroup{gamepad}
             * @luaheading{Gamepad}
             * @luadoc
             * Any connected pad can be read by its id (`0` to `9`) - no
             * registration needed. A stick read through
             * `input_get_gamepad_axis` has the pad's **dead zone** applied: a
             * worn stick at rest reads a little off centre (`0.04`, `-0.07`),
             * and a value strictly inside the dead zone (default `0.1`) reads
             * exactly `0`, so a game needs no threshold of its own.
             * `input_get_gamepad_axis_raw` gives the value as the pad reports
             * it, for a game that filters it itself (a round dead zone, a
             * curve, calibration). Triggers are always returned as they are:
             * they rest at `-1`, not at the centre.
             * @luaexample
             * input_is_gamepad_button_down(gamepadId, button) -> down
             * input_is_gamepad_button_up(gamepadId, button) -> up
             * input_get_gamepad_axis(gamepadId, axis) -> value       -- -1.0 .. 1.0, the dead zone applied
             * input_get_gamepad_axis_raw(gamepadId, axis) -> value   -- -1.0 .. 1.0, as the pad reports it
             *
             * function Player.update(dt)
             *   if input_is_gamepad_button_down(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) then
             *     -- move left
             *   end
             *
             *   -- a stick at rest reads 0 here, whatever its wear
             *   if input_get_gamepad_axis(0, GAMEPAD_AXIS_LEFT_X) < 0 then
             *     -- move left
             *   end
             * end
             */
            int LuaInputApi :: IsGamepadButtonDown( lua_State* pLuaState )  {

                std :: string                        strError;
                int                                  nGamePadId = 0;
                SunLight :: Input :: GamepadButton   button     = ( SunLight :: Input :: GamepadButton ) 0;

                if( !ReadGamepadButtonArgs( pLuaState, nGamePadId, button, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushboolean( pLuaState, LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler().IsGamepadButtonDown( nGamePadId, button ) );

                return 1;
            }

            /**
             * @luaname{input_is_gamepad_button_up(gamepadId, button) -> up | nil, message}
             * @luagroup{gamepad}
             */
            int LuaInputApi :: IsGamepadButtonUp( lua_State* pLuaState )  {

                std :: string                        strError;
                int                                  nGamePadId = 0;
                SunLight :: Input :: GamepadButton   button     = ( SunLight :: Input :: GamepadButton ) 0;

                if( !ReadGamepadButtonArgs( pLuaState, nGamePadId, button, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushboolean( pLuaState, LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler().IsGamepadButtonUp( nGamePadId, button ) );

                return 1;
            }

            /**
             * @brief A stick value with a dead zone applied - sunlight's own rule (ControllerManager's
             * dispatch): strictly inside (-fDeadzone, fDeadzone) reads 0, exactly at the edge or past
             * it the value is returned unchanged (no rescaling). A dead zone of 0 changes nothing.
             *
             * @param fValue The stick value, -1 to 1;
             * @param fDeadzone The dead zone, 0 to 1;
             * @return The value the automatic read returns;
             */
            float LuaInputApi :: ApplyDeadzone( float fValue, float fDeadzone )  {

                return ( ( fValue > -fDeadzone ) && ( fValue < fDeadzone ) ) ? 0.0f : fValue;
            }

            /**
             * @brief A stick or trigger, with the pad's dead zone applied to a stick (plans/MASTER_PLAN.md
             * item C1a): a value strictly inside the dead zone reads 0, sunlight's own rule
             * (ControllerManager's dispatch); outside it, the value is returned unchanged (no rescaling).
             *
             * @luaname{input_get_gamepad_axis(gamepadId, axis) -> value | nil, message}
             * @luagroup{gamepad}
             */
            int LuaInputApi :: GetGamepadAxisMovement( lua_State* pLuaState )  {

                std :: string                      strError;
                int                                nGamePadId = 0;
                SunLight :: Input :: GamepadAxis   axis       = SunLight :: Input :: GAMEPAD_AXIS_LEFT_X;

                if( !ReadGamepadIdArg( pLuaState, 1, nGamePadId, strError ) || !ReadAxisArg( pLuaState, 2, axis, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                float  fValue = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler().GetGamepadAxisMovement( nGamePadId, axis );

                if( IsStickAxis( axis ) )
                    fValue = ApplyDeadzone( fValue, LuaEngineUtil :: GetControllerManager( pLuaState ) -> GetAxisDeadzone( nGamePadId, axis ) );

                lua_pushnumber( pLuaState, fValue );

                return 1;
            }

            /**
             * @brief A stick or trigger exactly as the pad reports it - no dead zone.
             *
             * @luaname{input_get_gamepad_axis_raw(gamepadId, axis) -> value | nil, message}
             * @luagroup{gamepad}
             */
            int LuaInputApi :: GetGamepadAxisRaw( lua_State* pLuaState )  {

                std :: string                      strError;
                int                                nGamePadId = 0;
                SunLight :: Input :: GamepadAxis   axis       = SunLight :: Input :: GAMEPAD_AXIS_LEFT_X;

                if( !ReadGamepadIdArg( pLuaState, 1, nGamePadId, strError ) || !ReadAxisArg( pLuaState, 2, axis, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushnumber( pLuaState, LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler().GetGamepadAxisMovement( nGamePadId, axis ) );

                return 1;
            }

            /**
             * @brief Add a gamepad id to sunlight's own input dispatch (the handlers
             * `use_default_key_handler` installs) - mirrors the old EngineHost::InitializeGamePads.
             * Reading a pad through the functions above needs no registration.
             *
             * @luaname{input_add_gamepad(gamepadId) -> true | nil, message}
             * @luagroup{gamepad}
             * @luadoc
             * Adds a pad to the engine's own input dispatch, which only the
             * default key handler (`renderer_create{ use_default_key_handler = true }`)
             * uses. Reading a pad with the functions above does not need it.
             */
            int LuaInputApi :: AddGamepad( lua_State* pLuaState )  {

                std :: string  strError;
                int            nGamePadId = 0;

                if( !ReadGamepadIdArg( pLuaState, 1, nGamePadId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                LuaEngineUtil :: GetTileMap( pLuaState ) -> AddGamePad( nGamePadId );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Every gamepad connected right now, as an array of { id, name } tables.
             *
             * @luaname{input_get_connected_gamepads() -> pads}
             * @luagroup{pads}
             * @luaheading{Connected gamepads}
             * @luadoc
             * Which pads are plugged in, and when that changes. A pad id is
             * `0` to `9`; `name` is what the pad's driver calls it. Headless
             * runs never see a pad.
             *
             * `input_set_gamepad_connection_handler(fn)` calls
             * `fn(gamepadId, connected)` once each time a pad is plugged in
             * (`connected` true) or unplugged (false), on the main thread,
             * at the start of a frame before `on_update`. The first frame
             * reports every pad that was already connected at start-up, so
             * one handler set from the entry script covers both (one set later
             * only hears later changes - read `input_get_connected_gamepads`
             * then). Registering it needs no window, like
             * `collision_set_handler`; a second call replaces the first.
             * @luaexample
             * input_get_connected_gamepads() -> pads    -- { { id = 0, name = "Xbox Controller" }, ... }
             * input_is_gamepad_available(gamepadId) -> connected
             * input_get_gamepad_name(gamepadId) -> name -- "" when nothing is connected there
             *
             * input_set_gamepad_connection_handler(function(gamepadId, connected)
             *   if connected then
             *     print("pad " .. gamepadId .. " connected: " .. input_get_gamepad_name(gamepadId))
             *   else
             *     print("pad " .. gamepadId .. " lost")    -- a good moment to pause the game
             *   end
             * end)
             */
            int LuaInputApi :: GetConnectedGamepads( lua_State* pLuaState )  {

                std :: vector<SunLight :: Input :: stGamepadInfo>  pads = LuaEngineUtil :: GetControllerManager( pLuaState ) -> GetConnectedGamepads();

                lua_createtable( pLuaState, ( int ) pads.size(), 0 );

                for( size_t nPad = 0; nPad < pads.size(); nPad++ )  {
                    lua_createtable( pLuaState, 0, 2 );
                    lua_pushinteger( pLuaState, pads[nPad].nGamePadId );
                    lua_setfield( pLuaState, -2, "id" );
                    lua_pushstring( pLuaState, pads[nPad].szName ? pads[nPad].szName : "" );
                    lua_setfield( pLuaState, -2, "name" );
                    lua_rawseti( pLuaState, -2, ( lua_Integer ) nPad + 1 );
                }

                return 1;
            }

            /**
             * @luaname{input_is_gamepad_available(gamepadId) -> connected | nil, message}
             * @luagroup{pads}
             */
            int LuaInputApi :: IsGamepadAvailable( lua_State* pLuaState )  {

                std :: string  strError;
                int            nGamePadId = 0;

                if( !ReadGamepadIdArg( pLuaState, 1, nGamePadId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushboolean( pLuaState, LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler().IsGamepadAvailable( nGamePadId ) );

                return 1;
            }

            /**
             * @brief A pad's name as its driver reports it, or "" if nothing is connected at that id.
             *
             * @luaname{input_get_gamepad_name(gamepadId) -> name | nil, message}
             * @luagroup{pads}
             */
            int LuaInputApi :: GetGamepadName( lua_State* pLuaState )  {

                std :: string  strError;
                int            nGamePadId = 0;

                if( !ReadGamepadIdArg( pLuaState, 1, nGamePadId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: Input :: IInputHandler&  input  = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler();
                const char*                          szName = input.IsGamepadAvailable( nGamePadId ) ? input.GetGamepadName( nGamePadId ) : nullptr;

                lua_pushstring( pLuaState, szName ? szName : "" );

                return 1;
            }

            /**
             * @brief Keep the Lua function input_set_gamepad_connection_handler names; the C++
             * handler that calls it is installed once per renderer (InstallConnectionHandler).
             *
             * @luaname{input_set_gamepad_connection_handler(fn) -> true | nil, message}
             * @luagroup{pads}
             */
            int LuaInputApi :: SetGamepadConnectionHandler( lua_State* pLuaState )  {

                std :: string  strError;

                if( !LuaEngineUtil :: CheckFunctionArg( pLuaState, 1, "fn", strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushvalue( pLuaState, 1 );
                lua_setglobal( pLuaState, __CONNECTION_HANDLER_GLOBAL );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Make a renderer's controller manager report pad connections to the Lua handler
             * (if one is set). sunlight calls it from ControllerManager::Update, on the main thread,
             * between frames' Lua calls - not inside one - so it takes the Lua mutex itself, like every
             * other C++ entry into the Lua state (a set_timer callback may be waiting for it).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param controllerManager The renderer's controller manager;
             */
            void LuaInputApi :: InstallConnectionHandler( lua_State* pLuaState, SunLight :: Input :: IControllerManager& controllerManager )  {

                controllerManager.SetGamepadConnectionHandler( [pLuaState]( int nGamePadId, bool bConnected )  {
                    std :: lock_guard<std :: mutex>  lock( LuaEngineUtil :: s_LuaMutex );

                    lua_getglobal( pLuaState, __CONNECTION_HANDLER_GLOBAL );

                    if( !lua_isfunction( pLuaState, -1 ) )  {
                        lua_pop( pLuaState, 1 );
                        return;
                    }

                    lua_pushinteger( pLuaState, nGamePadId );
                    lua_pushboolean( pLuaState, bConnected );

                    if( lua_pcall( pLuaState, 2, 0, 0 ) != 0 )  {
                        fprintf( stderr, "Error calling gamepad connection handler: %s\n", lua_tostring( pLuaState, -1 ) );
                        lua_pop( pLuaState, 1 );
                    }
                } );
            }

            /**
             * @luaname{input_set_axis_deadzone(gamepadId, axis, deadzone) -> true | nil, message}
             * @luagroup{deadzone}
             * @luaheading{Stick dead zones}
             * @luadoc
             * The dead zone `input_get_gamepad_axis` applies to one stick axis
             * of one pad: a value strictly inside `(-deadzone, deadzone)`
             * reads `0`. `deadzone` is `0` (none - the automatic read is then
             * the raw value) to `1`; the default is `0.1`. Stick axes only
             * (`GAMEPAD_AXIS_LEFT_X/_Y`, `GAMEPAD_AXIS_RIGHT_X/_Y`): a trigger
             * is refused, since it rests at `-1`. A game's "stick
             * sensitivity" setting is one call per axis.
             * @luaexample
             * for _, axis in ipairs({ GAMEPAD_AXIS_LEFT_X, GAMEPAD_AXIS_LEFT_Y }) do
             *   input_set_axis_deadzone(0, axis, 0.15)     -- a worn stick
             * end
             * print(input_get_axis_deadzone(0, GAMEPAD_AXIS_LEFT_X))   --> 0.15
             */
            int LuaInputApi :: SetAxisDeadzone( lua_State* pLuaState )  {

                std :: string                      strError;
                int                                nGamePadId = 0;
                SunLight :: Input :: GamepadAxis   axis       = SunLight :: Input :: GAMEPAD_AXIS_LEFT_X;
                double                             fDeadzone  = 0.0;

                if( !ReadStickArgs( pLuaState, nGamePadId, axis, strError ) ||
                    !LuaEngineUtil :: ReadNumberArgInRange( pLuaState, 3, "deadzone", 0.0, __MAX_AXIS_DEADZONE, fDeadzone, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                LuaEngineUtil :: GetControllerManager( pLuaState ) -> SetAxisDeadzone( nGamePadId, axis, ( float ) fDeadzone );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief A stick axis's dead zone, as the shortest decimal of the float sunlight keeps
             * (so 0.15 reads back 0.15).
             *
             * @luaname{input_get_axis_deadzone(gamepadId, axis) -> deadzone | nil, message}
             * @luagroup{deadzone}
             */
            int LuaInputApi :: GetAxisDeadzone( lua_State* pLuaState )  {

                std :: string                      strError;
                int                                nGamePadId = 0;
                SunLight :: Input :: GamepadAxis   axis       = SunLight :: Input :: GAMEPAD_AXIS_LEFT_X;

                if( !ReadStickArgs( pLuaState, nGamePadId, axis, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushnumber( pLuaState, LuaEngineUtil :: ShortestDecimal( LuaEngineUtil :: GetControllerManager( pLuaState ) -> GetAxisDeadzone( nGamePadId, axis ) ) );

                return 1;
            }

            /**
             * @luaname{input_claim_player_slot(slot, gamepadId) -> true | nil, message}
             * @luagroup{slots}
             * @luaheading{Player slots}
             * @luadoc
             * A slot is the game's own player number (`0` or more - `1` for
             * player 1, say); claiming one ties a pad to it. A pad belongs to
             * one slot at a time: claiming a pad another slot holds is an
             * error naming that slot. Claiming a new pad for a slot that holds
             * one frees the old pad; claiming the same pair again is fine.
             * Unplugging a pad does NOT free its slot - plugged back in, it is
             * still that player; free it with `input_release_player_slot`
             * (eg. from the connection handler, if your game wants that). No
             * window is needed.
             *
             * `input_get_gamepad_for_slot` and `input_get_slot_for_gamepad`
             * return `false` when there is none.
             *
             * **Don't hard-code "pad 0 = player 1"**: the system numbers pads
             * in the order they connect, so the same pad can get a different
             * id on the next launch (seen with two pads: an Xbox pad was 0 in
             * one run and 1 in the next). Let a button press claim the slot,
             * as below.
             * @luaexample
             * -- "press A to join": the first pad that presses A becomes player 1, the next player 2
             * local nextPlayer = 1
             *
             * function on_update(dt)
             *   for _, pad in ipairs(input_get_connected_gamepads()) do
             *     if input_get_slot_for_gamepad(pad.id) == false
             *        and input_is_gamepad_button_down(pad.id, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) then
             *       input_claim_player_slot(nextPlayer, pad.id)
             *       nextPlayer = nextPlayer + 1
             *     end
             *   end
             * end
             */
            int LuaInputApi :: ClaimPlayerSlot( lua_State* pLuaState )  {

                std :: string  strError;
                int            nSlot      = 0;
                int            nGamePadId = 0;

                if( !ReadSlotArg( pLuaState, 1, nSlot, strError ) || !ReadGamepadIdArg( pLuaState, 2, nGamePadId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: Input :: PlayerSlotManager*  pSlots = GetPlayerSlots( pLuaState );

                if( !pSlots -> ClaimSlot( nSlot, nGamePadId ) )
                    return LuaEngineUtil :: PushError( pLuaState, "gamepad " + std :: to_string( nGamePadId ) + " is already claimed by slot " +
                                                                  std :: to_string( pSlots -> GetSlotForGamepad( nGamePadId ) ) );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Free the pad a slot holds (nothing to do if it holds none).
             *
             * @luaname{input_release_player_slot(slot) -> true | nil, message}
             * @luagroup{slots}
             */
            int LuaInputApi :: ReleasePlayerSlot( lua_State* pLuaState )  {

                std :: string  strError;
                int            nSlot = 0;

                if( !ReadSlotArg( pLuaState, 1, nSlot, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                GetPlayerSlots( pLuaState ) -> ReleaseSlot( nSlot );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{input_get_gamepad_for_slot(slot) -> gamepadId | false | nil, message}
             * @luagroup{slots}
             */
            int LuaInputApi :: GetGamepadForSlot( lua_State* pLuaState )  {

                std :: string  strError;
                int            nSlot = 0;

                if( !ReadSlotArg( pLuaState, 1, nSlot, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                int  nGamePadId = GetPlayerSlots( pLuaState ) -> GetGamepadForSlot( nSlot );

                if( nGamePadId == NO_SLOT_ASSOCIATION )
                    lua_pushboolean( pLuaState, 0 );
                else
                    lua_pushinteger( pLuaState, nGamePadId );

                return 1;
            }

            /**
             * @luaname{input_get_slot_for_gamepad(gamepadId) -> slot | false | nil, message}
             * @luagroup{slots}
             */
            int LuaInputApi :: GetSlotForGamepad( lua_State* pLuaState )  {

                std :: string  strError;
                int            nGamePadId = 0;

                if( !ReadGamepadIdArg( pLuaState, 1, nGamePadId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                int  nSlot = GetPlayerSlots( pLuaState ) -> GetSlotForGamepad( nGamePadId );

                if( nSlot == NO_SLOT_ASSOCIATION )
                    lua_pushboolean( pLuaState, 0 );
                else
                    lua_pushinteger( pLuaState, nSlot );

                return 1;
            }

            /**
             * @brief Register this module's constant groups as Lua globals (see each group's @luaconstants).
             * @param pLuaState Lua state to be used by engine call;
             */
            void LuaInputApi :: RegisterEnums( lua_State* pLuaState )  {

                LuaEngineUtil :: RegisterConstants( pLuaState, s_aKeyboardKeys, sizeof( s_aKeyboardKeys ) / sizeof( s_aKeyboardKeys[0] ) );
                LuaEngineUtil :: RegisterConstants( pLuaState, s_aGamepadButtons, sizeof( s_aGamepadButtons ) / sizeof( s_aGamepadButtons[0] ) );
                LuaEngineUtil :: RegisterConstants( pLuaState, s_aGamepadAxis, sizeof( s_aGamepadAxis ) / sizeof( s_aGamepadAxis[0] ) );
                LuaEngineUtil :: RegisterConstants( pLuaState, s_aControllerTypes, sizeof( s_aControllerTypes ) / sizeof( s_aControllerTypes[0] ) );
            }

            /**
             * @brief Register the input polling Lua-callable functions and enums.
             */
            void LuaInputApi :: Register( lua_State* pLuaState )  {

                lua_register( pLuaState, "input_is_key_down", LuaInputApi :: IsKeyDown );
                lua_register( pLuaState, "input_is_key_up", LuaInputApi :: IsKeyUp );
                lua_register( pLuaState, "input_is_key_released", LuaInputApi :: IsKeyReleased );
                lua_register( pLuaState, "input_is_gamepad_button_down", LuaInputApi :: IsGamepadButtonDown );
                lua_register( pLuaState, "input_is_gamepad_button_up", LuaInputApi :: IsGamepadButtonUp );
                lua_register( pLuaState, "input_get_gamepad_axis", LuaInputApi :: GetGamepadAxisMovement );
                lua_register( pLuaState, "input_get_gamepad_axis_raw", LuaInputApi :: GetGamepadAxisRaw );
                lua_register( pLuaState, "input_add_gamepad", LuaInputApi :: AddGamepad );
                lua_register( pLuaState, "input_get_connected_gamepads", LuaInputApi :: GetConnectedGamepads );
                lua_register( pLuaState, "input_is_gamepad_available", LuaInputApi :: IsGamepadAvailable );
                lua_register( pLuaState, "input_get_gamepad_name", LuaInputApi :: GetGamepadName );
                lua_register( pLuaState, "input_set_gamepad_connection_handler", LuaInputApi :: SetGamepadConnectionHandler );
                lua_register( pLuaState, "input_set_axis_deadzone", LuaInputApi :: SetAxisDeadzone );
                lua_register( pLuaState, "input_get_axis_deadzone", LuaInputApi :: GetAxisDeadzone );
                lua_register( pLuaState, "input_claim_player_slot", LuaInputApi :: ClaimPlayerSlot );
                lua_register( pLuaState, "input_release_player_slot", LuaInputApi :: ReleasePlayerSlot );
                lua_register( pLuaState, "input_get_gamepad_for_slot", LuaInputApi :: GetGamepadForSlot );
                lua_register( pLuaState, "input_get_slot_for_gamepad", LuaInputApi :: GetSlotForGamepad );

                RegisterEnums( pLuaState );
            }
        }
    }
}
