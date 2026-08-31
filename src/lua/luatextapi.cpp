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
             *
             * @luaname{set_font(path) -> success}
             * @luadoc
             * Load (or replace) the font used by `draw_text`/
             * `measure_text`, from a font file on disk. The format is
             * auto-detected purely by the file's own extension —
             * TrueType/OpenType (`.ttf`/`.otf`) and an AngelCode BMFont
             * atlas (`.fnt`) are both supported without this API (or the
             * caller) needing to know which — `draw_text`'s own code has
             * no format-specific logic at all.
             *
             * Until this is called for the first time (or if every call
             * so far has failed), `draw_text`/`measure_text` use the
             * engine's own built-in default font, so text can be drawn
             * with zero setup. A bad/missing path returns `false` and
             * leaves whatever font was previously active untouched — it
             * does not error or crash.
             * @luaexample
             * -- main.lua, early in bootstrap
             * set_font( BASE_PATH .. "fonts/msx-screen0.ttf" )
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
             *
             * @luaname{draw_text(text, x, y, size, r, g, b, a)}
             * @luadoc
             * Draw a line of text on screen, in screen space — no
             * camera/viewport transform of its own, same as the FPS
             * counter — using whichever font is currently active (see
             * `set_font`). `r`/`g`/`b`/`a` are each `0`-`255`.
             *
             * ```lua
             * draw_text( "Lives: 5", 20, 20, 24, 255, 255, 255, 255 )
             * ```
             *
             * Meant to be called every frame from a normal
             * `on_update`-style hook (e.g. via
             * `Enemies.register_update`, the same per-frame dispatch
             * every enemy/player module in Caravellius already uses) —
             * nothing here is "issue once and it persists"; a HUD
             * element only stays on screen for as long as something
             * keeps calling `draw_text` for it each frame.
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
             *
             * @luaname{measure_text(text, size) -> width}
             * @luadoc
             * Measure how wide a line of text would render, in pixels, at
             * a given font size — using the same font `draw_text` itself
             * would use. Meant for HUD layout (e.g. right-aligning a
             * score/lives readout against `screen_get_width()`) without
             * hardcoding assumed widths, since digit/character count can
             * change over a playthrough.
             * @luaexample
             * local text  = "Lives: " .. tostring( Lives.count )
             * local width = measure_text( text, 24 )
             * local x     = screen_get_width() - width - 20  -- flush to the right edge, with a 20px margin
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
             *
             * @luaname{screen_get_width() -> width}
             * @luagroup{screen_dimensions}
             * @luadoc
             * The engine's own fixed design/render resolution, in pixels
             * (`DISPLAY_W`/`DISPLAY_H` in `main.h` — `1260x920` today) —
             * the coordinate space `draw_text` and every other
             * screen-space draw call operate in. Constant for the app's
             * whole lifetime, set once at construction.
             *
             * **Deliberately not the same as the live OS window size** —
             * the engine always letterbox-scales this fixed resolution to
             * fit whatever the real window size ends up being (whether
             * from `app_set_fullscreen` or the player manually resizing
             * the window, see [app.md](app.md)). HUD layout should always measure
             * against these, not against anything that varies with the
             * actual window.
             */
            int LuaTextApi :: GetWindowWidth( lua_State *pLuaState )  {

                lua_pushinteger( pLuaState, LuaEngineUtil :: GetDrawSurface( pLuaState ) -> GetWindowWidth() );

                return 1;
            }

            /**
             * @brief Same as @link GetWindowWidth, for height.
             *
             * @luaname{screen_get_height() -> height}
             * @luagroup{screen_dimensions}
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
