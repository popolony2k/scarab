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

#include "lua/luablendapi.h"
#include "lua/luaengineutil.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief Read a BLEND_MODE_* value from the Lua stack, strictly: anything but one of
             * the four integer constants is an error with the reason. Shared with LuaSpriteApi.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nIndex Stack index of the value;
             * @param mode Receives the mode on success;
             * @param strError Receives the reason on failure;
             * @return true if the value is a valid mode;
             */
            bool LuaBlendApi :: ReadBlendMode( lua_State* pLuaState, int nIndex, SunLight :: Base :: BlendMode& mode, std :: string& strError )  {

                if( !lua_isinteger( pLuaState, nIndex ) )  {
                    strError = "expected a BLEND_MODE_* constant";
                    return false;
                }

                lua_Integer  nValue = lua_tointeger( pLuaState, nIndex );

                if( ( nValue < SunLight :: Base :: BLEND_MODE_NORMAL ) || ( nValue > SunLight :: Base :: BLEND_MODE_SUBTRACT ) )  {
                    strError = "unknown blend mode " + std :: to_string( nValue );
                    return false;
                }

                mode = ( SunLight :: Base :: BlendMode ) nValue;

                return true;
            }

            /**
             * @luaname{draw_begin_blend_mode(mode) -> true | nil, message}
             * @luagroup{scoped_blend}
             * @luaheading{Blending what a script draws}
             * @luadoc
             * Draw everything from here to `draw_end_blend_mode()` by `mode` — what
             * `draw_filled_rectangle` and `draw_text` draw from `on_update`. Lua draws after
             * the map and the sprites, so this blends over the whole scene: an `ADD` rectangle
             * is a light, a `MULTIPLY` one a tint or a night filter. Sprites are drawn before
             * `on_update` and are not affected — give a sprite its own mode with
             * `sprite_set_blend_mode`.
             *
             * The calls do **not** nest: a second `draw_begin_blend_mode` replaces the first one's
             * mode, and `draw_end_blend_mode()` always goes back to `BLEND_MODE_NORMAL`. Every
             * frame starts in normal and the screen fade always draws in normal, so a forgotten
             * `draw_end_blend_mode()` costs one frame at most. An unknown mode returns
             * `nil, "unknown blend mode N"` and changes nothing.
             * @luaexample
             * function on_update( dt )
             *     draw_begin_blend_mode( BLEND_MODE_ADD )
             *     draw_filled_rectangle( 400, 300, 200, 200, 255, 160, 60, 120 )   -- a warm light
             *     draw_end_blend_mode()
             *
             *     draw_text( "HUD", 20, 20, 20, 20, 20, 20, 255 )                   -- normal again
             * end
             */
            int LuaBlendApi :: BeginBlendMode( lua_State* pLuaState )  {

                SunLight :: Base :: BlendMode  mode = SunLight :: Base :: BLEND_MODE_NORMAL;
                std :: string                  strError;

                if( !ReadBlendMode( pLuaState, 1, mode, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> BeginBlendMode( mode );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{draw_end_blend_mode() -> true}
             * @luagroup{scoped_blend}
             */
            int LuaBlendApi :: EndBlendMode( lua_State* pLuaState )  {

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> EndBlendMode();

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Register the BLEND_MODE_* constants - sunlight's SunLight::Base::BlendMode,
             * one to one.
             */
            void LuaBlendApi :: RegisterEnums( lua_State* pLuaState )  {

                /**
                 * @luaconstants{Blend modes}
                 * @luadoc
                 * The `mode` that `draw_begin_blend_mode` and `sprite_set_blend_mode` take, and
                 * `sprite_get_blend_mode` returns — see the formulas at the top of this page.
                 */
                static const stNamedConstant  s_aBlendModes[] = {
                    { "BLEND_MODE_NORMAL", SunLight :: Base :: BLEND_MODE_NORMAL },
                    { "BLEND_MODE_ADD", SunLight :: Base :: BLEND_MODE_ADD },
                    { "BLEND_MODE_MULTIPLY", SunLight :: Base :: BLEND_MODE_MULTIPLY },
                    { "BLEND_MODE_SUBTRACT", SunLight :: Base :: BLEND_MODE_SUBTRACT },
                };

                LuaEngineUtil :: RegisterConstants( pLuaState, s_aBlendModes,
                                                    sizeof( s_aBlendModes ) / sizeof( s_aBlendModes[0] ) );
            }

            /**
             * @brief Register the blend-mode Lua-callable functions and constants. Needs no
             * renderer: the draw functions reach it only when called.
             */
            void LuaBlendApi :: Register( lua_State* pLuaState )  {

                lua_register( pLuaState, "draw_begin_blend_mode", LuaBlendApi :: BeginBlendMode );
                lua_register( pLuaState, "draw_end_blend_mode", LuaBlendApi :: EndBlendMode );

                RegisterEnums( pLuaState );
            }
        }
    }
}
