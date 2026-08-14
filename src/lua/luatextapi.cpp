/*
 * luatextapi.cpp
 *
 *  Created on: Aug 14, 2026
 *      Author: popolony2k
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
             * the file's own extension - see ITileMap::SetFont). Until
             * this is called, DrawText uses the engine's own built-in
             * default font.
             */
            int LuaTextApi :: SetFont( lua_State *pLuaState )  {

                const char  *szFilePath = lua_tostring( pLuaState, 1 );
                bool        bResult     = LuaEngineUtil :: GetTileMap( pLuaState ) -> SetFont( szFilePath );

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

                LuaEngineUtil :: GetTileMap( pLuaState ) -> DrawText( szText, nPosX, nPosY, nSize,
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
                int         nWidth  = LuaEngineUtil :: GetTileMap( pLuaState ) -> MeasureText( szText, nSize );

                lua_pushinteger( pLuaState, nWidth );

                return 1;
            }

            /**
             * @brief The engine's own fixed design/render resolution
             * width, in pixels - the coordinate space @link DrawText
             * itself draws in. Constant for the app's whole lifetime,
             * deliberately not the live/resizeable OS window size (see
             * ITileMap::GetWindowWidth).
             */
            int LuaTextApi :: GetWindowWidth( lua_State *pLuaState )  {

                lua_pushinteger( pLuaState, LuaEngineUtil :: GetTileMap( pLuaState ) -> GetWindowWidth() );

                return 1;
            }

            /**
             * @brief Same as @link GetWindowWidth, for height.
             */
            int LuaTextApi :: GetWindowHeight( lua_State *pLuaState )  {

                lua_pushinteger( pLuaState, LuaEngineUtil :: GetTileMap( pLuaState ) -> GetWindowHeight() );

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
