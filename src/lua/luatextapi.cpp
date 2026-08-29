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

#include "lua/luatextapi.h"
#include "lua/luaengineutil.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief Load (or replace) the font used by @link DrawText.
             * Accepts any font file raylib itself supports (TrueType/
             * OpenType, or an AngelCode BMFont ".fnt" atlas, told apart by
             * the file's own extension - see IDrawSurface::SetFont). Until
             * this is called, DrawText uses the engine's own built-in
             * default font.
             */
            int LuaTextApi :: SetFont( lua_State *pLuaState )  {

                const char  *szFilePath = lua_tostring( pLuaState, 1 );
                bool        bResult     = LuaEngineUtil :: GetDrawSurface( pLuaState ) -> SetFont( szFilePath );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @brief Draw a line of text on screen, in screen space, using
             * whichever font is currently active (see @link SetFont).
             */
            int LuaTextApi :: DrawText( lua_State *pLuaState )  {

                const char     *szText  = lua_tostring( pLuaState, 1 );
                int            nPosX    = ( int ) lua_tointeger( pLuaState, 2 );
                int            nPosY    = ( int ) lua_tointeger( pLuaState, 3 );
                int            nSize    = ( int ) lua_tointeger( pLuaState, 4 );
                unsigned char  nRed     = ( unsigned char ) lua_tointeger( pLuaState, 5 );
                unsigned char  nGreen   = ( unsigned char ) lua_tointeger( pLuaState, 6 );
                unsigned char  nBlue    = ( unsigned char ) lua_tointeger( pLuaState, 7 );
                unsigned char  nAlpha   = ( unsigned char ) lua_tointeger( pLuaState, 8 );

                LuaEngineUtil :: GetDrawSurface( pLuaState ) -> DrawText( szText, nPosX, nPosY, nSize,
                                                                       nRed, nGreen, nBlue, nAlpha );

                return 0;
            }

            /**
             * @brief Measure how wide a line of text would render, in
             * pixels, at a given font size - meant for HUD layout (e.g.
             * right-aligning a score/lives readout against @link
             * GetWindowWidth), using whichever font @link DrawText itself
             * would use.
             */
            int LuaTextApi :: MeasureText( lua_State *pLuaState )  {

                const char  *szText = lua_tostring( pLuaState, 1 );
                int         nSize   = ( int ) lua_tointeger( pLuaState, 2 );
                int         nWidth  = LuaEngineUtil :: GetDrawSurface( pLuaState ) -> MeasureText( szText, nSize );

                lua_pushinteger( pLuaState, nWidth );

                return 1;
            }

            /**
             * @brief The engine's own fixed design/render resolution
             * width, in pixels - the coordinate space @link DrawText
             * itself draws in. Constant for the app's whole lifetime,
             * deliberately not the live/resizeable OS window size (see
             * IDrawSurface::GetWindowWidth).
             */
            int LuaTextApi :: GetWindowWidth( lua_State *pLuaState )  {

                lua_pushinteger( pLuaState, LuaEngineUtil :: GetDrawSurface( pLuaState ) -> GetWindowWidth() );

                return 1;
            }

            /**
             * @brief Same as @link GetWindowWidth, for height.
             */
            int LuaTextApi :: GetWindowHeight( lua_State *pLuaState )  {

                lua_pushinteger( pLuaState, LuaEngineUtil :: GetDrawSurface( pLuaState ) -> GetWindowHeight() );

                return 1;
            }

            /**
             * @brief Register the text/font Lua-callable functions.
             */
            void LuaTextApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "set_font", LuaTextApi :: SetFont );
                lua_register( pLuaState, "draw_text", LuaTextApi :: DrawText );
                lua_register( pLuaState, "measure_text", LuaTextApi :: MeasureText );
                lua_register( pLuaState, "screen_get_width", LuaTextApi :: GetWindowWidth );
                lua_register( pLuaState, "screen_get_height", LuaTextApi :: GetWindowHeight );
            }
        }
    }
}
