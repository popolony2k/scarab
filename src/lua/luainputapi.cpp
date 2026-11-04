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
#include "input/actionmap.h"
#include "input/igamepadhaptics.h"
#include "filesystem/filepath.h"
#include "engine/savedirectory.h"
#include "lua/luaappapi.h"
#include <memory>
#include <climits>
#include <cstdio>
#include <mutex>
#include <vector>


#define __CONST( name )  { #name, ( int ) SunLight :: Input :: name }

/** The Lua global input_set_gamepad_connection_handler keeps its function in. */
#define __CONNECTION_HANDLER_GLOBAL  "__gamepad_connection_handler"

/** A dead zone is a distance from the stick's centre: 0 (none) to 1 (the whole travel). */
#define __MAX_AXIS_DEADZONE          1.0

/** How far past centre a bound axis must move to make its action active, unless the call says. */
#define __DEFAULT_ACTION_THRESHOLD   0.5

/** A motor strength: 0 (off) to 1 (full). */
#define __MAX_MOTOR_STRENGTH         1.0

/** The characters an action's name may hold besides letters and digits. */
#define __ACTION_NAME_EXTRA          "-_."


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
             * @luaconstants{Action binding types}
             * @luadoc
             * What `input_get_action_binding` reports an action is bound to.
             */
            static const stNamedConstant  s_aActionBindingTypes[] = {
                __CONST( ACTION_BINDING_NONE ),
                __CONST( ACTION_BINDING_KEY ),
                __CONST( ACTION_BINDING_GAMEPAD_BUTTON ),
                __CONST( ACTION_BINDING_GAMEPAD_AXIS ),
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
             * @brief The engine's action map, stashed by LuaEngine::Init as ACTION_MAP_GLOBAL.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The action map;
             */
            static SunLight :: Input :: ActionMap* GetActionMap( lua_State* pLuaState )  {

                return static_cast<SunLight :: Input :: ActionMap *>( LuaEngineUtil :: GetGlobalPointer( pLuaState, LuaInputApi :: ACTION_MAP_GLOBAL ) );
            }

            /**
             * @brief Whether an action's name is acceptable: letters, digits and "-_." only - it is written
             * as "name=..." on its own line in the bindings file, so "=", a line break or a "#" (a comment
             * there) would corrupt it.
             *
             * @param strName The name;
             * @param strError Receives the reason when it is not acceptable;
             * @return true if it is acceptable;
             */
            bool LuaInputApi :: IsValidActionName( const std :: string& strName, std :: string& strError )  {

                if( strName.empty() )  {
                    strError = "argument 'name' can't be empty";
                    return false;
                }

                for( char c : strName )  {
                    bool  bLetterOrDigit = ( ( c >= 'a' ) && ( c <= 'z' ) ) || ( ( c >= 'A' ) && ( c <= 'Z' ) ) || ( ( c >= '0' ) && ( c <= '9' ) );

                    if( !bLetterOrDigit && ( std :: string( __ACTION_NAME_EXTRA ).find( c ) == std :: string :: npos ) )  {
                        strError = "argument 'name' may hold only letters, digits and '" __ACTION_NAME_EXTRA "' - '" + strName + "' doesn't";
                        return false;
                    }
                }

                return true;
            }

            /** @brief Read an action's name (argument 1), checked by IsValidActionName. */
            static bool ReadActionNameArg( lua_State* pLuaState, std :: string& strName, std :: string& strError )  {

                return LuaEngineUtil :: ReadStringArg( pLuaState, 1, "name", strName, strError ) &&
                       LuaInputApi :: IsValidActionName( strName, strError );
            }

            /**
             * @brief Push the result of a bind: true, or the reason sunlight refused it - the input is
             * already bound to another action (one input, one action).
             */
            static int PushBindResult( lua_State* pLuaState, bool bBound, const char* szInput )  {

                if( !bBound )
                    return LuaEngineUtil :: PushError( pLuaState, std :: string( "that " ) + szInput +
                                                       " is already bound to another action - unbind that action first" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{input_bind_action_key(name, key) -> true | nil, message}
             * @luagroup{actions}
             * @luaheading{Actions (rebindable controls)}
             * @luadoc
             * An action is a name the game reads ("jump", "fire") instead of a
             * fixed key, so a player can rebind it. Each action is bound to one
             * input at a time - a key, a gamepad button, or a stick or trigger
             * moved one way (`direction` `1` or `-1`); binding it again replaces
             * the old binding. One input serves one action: binding a key that
             * another action already has is refused. `name` is letters, digits
             * and `-_.`. No window is needed to bind, save or load.
             *
             * `input_is_action_active(name[, gamepadId[, threshold]])` reads
             * whether its input is held now: a button or axis binding on
             * `gamepadId` (default `0`), an axis past `threshold` (default
             * `0.5`); a key binding ignores the pad. An unbound action is never
             * active.
             *
             * `input_save_action_map(path)` and `input_load_action_map(path)`
             * keep the bindings in a text file (sunlight's own format). Save the
             * player's bindings under a plain path: it is the game's save
             * directory (`app_set_save_directory`), and loading the same plain
             * path reads it back from there. Ship defaults under `APP_DIR`
             * (inside the `.zip` for a packed game, even an encrypted one) and
             * load them when the player has no save yet, as below. A load that
             * fails leaves the bindings as they were.
             * @luaexample
             * input_bind_action_key(name, key)
             * input_bind_action_button(name, button)
             * input_bind_action_axis(name, axis, direction)   -- direction 1 or -1
             * input_unbind_action(name)
             * input_get_action_binding(name) -> type, code, direction
             * input_is_action_active(name[, gamepadId[, threshold]]) -> active
             * input_save_action_map(path) / input_load_action_map(path)
             *
             * -- the player's own bindings if they saved some, else the game's shipped defaults
             * assert(app_set_save_directory("MyGame"))
             * if not input_load_action_map("bindings.txt") then
             *   assert(input_load_action_map(APP_DIR .. "/default_bindings.txt"))
             * end
             *
             * -- after the player rebinds "jump" in the options menu
             * input_bind_action_key("jump", KEY_W)
             * assert(input_save_action_map("bindings.txt"))
             *
             * function on_update(dt)
             *   if input_is_action_active("jump") then player_jump() end
             * end
             */
            int LuaInputApi :: BindActionKey( lua_State* pLuaState )  {

                std :: string                      strError;
                std :: string                      strName;
                SunLight :: Input :: KeyboardKey   key = SunLight :: Input :: KeyboardKey :: KEY_NULL;

                if( !ReadActionNameArg( pLuaState, strName, strError ) || !ReadKeyArg( pLuaState, 2, "key", key, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                return PushBindResult( pLuaState, GetActionMap( pLuaState ) -> BindKey( strName, key ), "key" );
            }

            /**
             * @luaname{input_bind_action_button(name, button) -> true | nil, message}
             * @luagroup{actions}
             */
            int LuaInputApi :: BindActionButton( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strName;
                int            nButton = 0;

                if( !ReadActionNameArg( pLuaState, strName, strError ) ||
                    !LuaEngineUtil :: ReadConstantArg( pLuaState, 2, "button", s_aGamepadButtons,
                                                       sizeof( s_aGamepadButtons ) / sizeof( s_aGamepadButtons[0] ),
                                                       "GAMEPAD_BUTTON_*", nButton, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                return PushBindResult( pLuaState, GetActionMap( pLuaState ) -> BindGamepadButton( strName, ( SunLight :: Input :: GamepadButton ) nButton ),
                                       "button" );
            }

            /**
             * @luaname{input_bind_action_axis(name, axis, direction) -> true | nil, message}
             * @luagroup{actions}
             */
            int LuaInputApi :: BindActionAxis( lua_State* pLuaState )  {

                std :: string                      strError;
                std :: string                      strName;
                SunLight :: Input :: GamepadAxis   axis       = SunLight :: Input :: GAMEPAD_AXIS_LEFT_X;
                lua_Integer                        nDirection = 0;

                if( !ReadActionNameArg( pLuaState, strName, strError ) || !ReadAxisArg( pLuaState, 2, axis, strError ) ||
                    !LuaEngineUtil :: ReadIntegerArgInRange( pLuaState, 3, "direction", -1, 1, nDirection, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( nDirection == 0 )
                    return LuaEngineUtil :: PushError( pLuaState, "argument 'direction' must be 1 or -1" );

                return PushBindResult( pLuaState, GetActionMap( pLuaState ) -> BindGamepadAxis( strName, axis, ( int ) nDirection ),
                                       "axis direction" );
            }

            /**
             * @brief Remove an action's binding (nothing to do if it has none).
             *
             * @luaname{input_unbind_action(name) -> true | nil, message}
             * @luagroup{actions}
             */
            int LuaInputApi :: UnbindAction( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strName;

                if( !ReadActionNameArg( pLuaState, strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                GetActionMap( pLuaState ) -> Unbind( strName );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief An action's binding: its type (ACTION_BINDING_*), its code (a KEY_*, GAMEPAD_BUTTON_* or
             * GAMEPAD_AXIS_* value) and, for an axis, its direction (1 or -1; 0 otherwise).
             *
             * @luaname{input_get_action_binding(name) -> type, code, direction | nil, message}
             * @luagroup{actions}
             */
            int LuaInputApi :: GetActionBinding( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strName;

                if( !ReadActionNameArg( pLuaState, strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: Input :: stActionBinding  binding = GetActionMap( pLuaState ) -> GetBinding( strName );

                lua_pushinteger( pLuaState, ( lua_Integer ) binding.type );
                lua_pushinteger( pLuaState, binding.nCode );
                lua_pushinteger( pLuaState, binding.nDirection );

                return 3;
            }

            /**
             * @brief Whether an action's input is held now - reads the input, so it needs the window.
             *
             * @luaname{input_is_action_active(name[, gamepadId[, threshold]]) -> active | nil, message}
             * @luagroup{actions}
             */
            int LuaInputApi :: IsActionActive( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strName;
                lua_Integer    nGamePadId = 0;
                double         fThreshold = __DEFAULT_ACTION_THRESHOLD;

                if( !ReadActionNameArg( pLuaState, strName, strError ) ||
                    !LuaEngineUtil :: ReadOptionalIntegerArg( pLuaState, 2, "gamepadId", 0, nGamePadId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( ( nGamePadId < 0 ) || ( nGamePadId > MAX_GAMEPAD_COUNT - 1 ) )
                    return LuaEngineUtil :: PushError( pLuaState, "argument 'gamepadId' must be between 0 and " + std :: to_string( MAX_GAMEPAD_COUNT - 1 ) );

                if( !lua_isnoneornil( pLuaState, 3 ) &&
                    !LuaEngineUtil :: ReadNumberArgInRange( pLuaState, 3, "threshold", 0.0, __MAX_AXIS_DEADZONE, fThreshold, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushboolean( pLuaState, GetActionMap( pLuaState ) -> IsActionActive( strName, LuaEngineUtil :: GetTileMap( pLuaState ) -> GetInputHandler(),
                                                                                        ( int ) nGamePadId, ( float ) fThreshold ) );

                return 1;
            }

            /**
             * @brief Save every binding to a file. A plain (virtual) path needs a save directory: the
             * FileSystem writes a virtual path into it, and saying so is clearer than sunlight's plain false.
             *
             * @luaname{input_save_action_map(path) -> true | nil, message}
             * @luagroup{actions}
             */
            int LuaInputApi :: SaveActionMap( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strPath;
                std :: string  strRest;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "path", strPath, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                Engine :: SaveDirectory*  pSaveDirectory = static_cast<Engine :: SaveDirectory *>(
                                              LuaEngineUtil :: GetGlobalPointer( pLuaState, LuaAppApi :: SAVE_DIRECTORY_GLOBAL ) );

                if( ( SunLight :: FileSystem :: FilePath :: SplitPath( strPath, strRest ) == SunLight :: FileSystem :: FilePath :: PATH_SCHEME_VIRTUAL ) &&
                    pSaveDirectory -> GetPath().empty() )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot save '" + strPath + "': no save directory (app_set_save_directory, "
                                                                  "or \"save_directory\" in the project file)" );

                if( !GetActionMap( pLuaState ) -> Save( strPath ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot write '" + strPath + "'" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Replace the bindings with a file's; a failure leaves them as they were.
             *
             * @luaname{input_load_action_map(path) -> true | nil, message}
             * @luagroup{actions}
             */
            int LuaInputApi :: LoadActionMap( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strPath;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "path", strPath, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !GetActionMap( pLuaState ) -> Load( strPath ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot read or parse '" + strPath + "' - the bindings are unchanged" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief The rumble backend, made with the renderer (LuaEngine::AttachHaptics) - so this creates
             * the default renderer first if none exists yet.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The rumble backend, never null;
             */
            static SunLight :: Input :: IGamepadHaptics* GetHaptics( lua_State* pLuaState )  {

                std :: unique_ptr<SunLight :: Input :: IGamepadHaptics>*  pHolder =
                    static_cast<std :: unique_ptr<SunLight :: Input :: IGamepadHaptics> *>( LuaEngineUtil :: GetGlobalPointer( pLuaState, LuaInputApi :: HAPTICS_GLOBAL ) );

                if( !*pHolder )
                    LuaEngineUtil :: EnsureRenderer( pLuaState );

                return pHolder -> get();
            }

            /**
             * @luaname{input_is_rumble_supported(gamepadId) -> supported | nil, message}
             * @luagroup{rumble}
             * @luaheading{Rumble}
             * @luadoc
             * Make a gamepad vibrate: `left` and `right` are the two motors'
             * strengths, `0` (off) to `1` (full), for `seconds` (more than `0`);
             * it stops by itself, or now with `input_stop_rumble`. A pad that
             * can't vibrate is an error from `input_set_rumble` - check
             * `input_is_rumble_supported` first to skip the effect. Headless runs
             * see no pad, so nothing rumbles there.
             * @luaexample
             * input_is_rumble_supported(gamepadId) -> supported
             * input_set_rumble(gamepadId, left, right, seconds)
             * input_stop_rumble(gamepadId)
             *
             * -- a short, strong hit when the ship is damaged
             * if input_is_rumble_supported(pad) then
             *   input_set_rumble(pad, 0.8, 1.0, 0.25)
             * end
             */
            int LuaInputApi :: IsRumbleSupported( lua_State* pLuaState )  {

                std :: string  strError;
                int            nGamePadId = 0;

                if( !ReadGamepadIdArg( pLuaState, 1, nGamePadId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushboolean( pLuaState, GetHaptics( pLuaState ) -> IsSupported( nGamePadId ) );

                return 1;
            }

            /**
             * @luaname{input_set_rumble(gamepadId, left, right, seconds) -> true | nil, message}
             * @luagroup{rumble}
             */
            int LuaInputApi :: SetRumble( lua_State* pLuaState )  {

                std :: string  strError;
                int            nGamePadId = 0;
                double         fLeft      = 0.0;
                double         fRight     = 0.0;
                double         fSeconds   = 0.0;

                if( !ReadGamepadIdArg( pLuaState, 1, nGamePadId, strError ) ||
                    !LuaEngineUtil :: ReadNumberArgInRange( pLuaState, 2, "left", 0.0, __MAX_MOTOR_STRENGTH, fLeft, strError ) ||
                    !LuaEngineUtil :: ReadNumberArgInRange( pLuaState, 3, "right", 0.0, __MAX_MOTOR_STRENGTH, fRight, strError ) ||
                    !LuaEngineUtil :: ReadNumberArg( pLuaState, 4, "seconds", fSeconds, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !( fSeconds > 0.0 ) )
                    return LuaEngineUtil :: PushError( pLuaState, "argument 'seconds' must be more than 0" );

                SunLight :: Input :: IGamepadHaptics*  pHaptics = GetHaptics( pLuaState );

                if( !pHaptics -> IsSupported( nGamePadId ) )
                    return LuaEngineUtil :: PushError( pLuaState, "gamepad " + std :: to_string( nGamePadId ) + " can't rumble "
                                                                  "(not connected, or no motors)" );

                pHaptics -> SetVibration( nGamePadId, ( float ) fLeft, ( float ) fRight, ( float ) fSeconds );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Stop a gamepad's rumble now (nothing to do if it isn't rumbling).
             *
             * @luaname{input_stop_rumble(gamepadId) -> true | nil, message}
             * @luagroup{rumble}
             */
            int LuaInputApi :: StopRumble( lua_State* pLuaState )  {

                std :: string  strError;
                int            nGamePadId = 0;

                if( !ReadGamepadIdArg( pLuaState, 1, nGamePadId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                GetHaptics( pLuaState ) -> Stop( nGamePadId );

                return LuaEngineUtil :: PushTrue( pLuaState );
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
                LuaEngineUtil :: RegisterConstants( pLuaState, s_aActionBindingTypes, sizeof( s_aActionBindingTypes ) / sizeof( s_aActionBindingTypes[0] ) );
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
                lua_register( pLuaState, "input_bind_action_key", LuaInputApi :: BindActionKey );
                lua_register( pLuaState, "input_bind_action_button", LuaInputApi :: BindActionButton );
                lua_register( pLuaState, "input_bind_action_axis", LuaInputApi :: BindActionAxis );
                lua_register( pLuaState, "input_unbind_action", LuaInputApi :: UnbindAction );
                lua_register( pLuaState, "input_get_action_binding", LuaInputApi :: GetActionBinding );
                lua_register( pLuaState, "input_is_action_active", LuaInputApi :: IsActionActive );
                lua_register( pLuaState, "input_save_action_map", LuaInputApi :: SaveActionMap );
                lua_register( pLuaState, "input_load_action_map", LuaInputApi :: LoadActionMap );
                lua_register( pLuaState, "input_is_rumble_supported", LuaInputApi :: IsRumbleSupported );
                lua_register( pLuaState, "input_set_rumble", LuaInputApi :: SetRumble );
                lua_register( pLuaState, "input_stop_rumble", LuaInputApi :: StopRumble );

                RegisterEnums( pLuaState );
            }
        }
    }
}
