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
#include <climits>
#include <memory>
#include <string>


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief The engine's FontPool, stashed by LuaEngine::Init as the light userdata global
             * FONT_POOL_GLOBAL. Pops what it pushed: the font functions read optional arguments
             * after resolving a handle, and a leftover value would sit in their place (found in C6).
             */
            static Engine :: FontPool* GetFontPool( lua_State *pLuaState )  {

                lua_getglobal( pLuaState, LuaTextApi :: FONT_POOL_GLOBAL );

                Engine :: FontPool  *pFontPool = static_cast<Engine :: FontPool *>( lua_touserdata( pLuaState, -1 ) );

                lua_pop( pLuaState, 1 );

                return pFontPool;
            }

            /**
             * @brief Resolve an optional font-handle argument (draw_text/measure_text's last one):
             * none or nil means "the active font", which is reported as a null pFont and true.
             * @return false (with strError set) for anything else that is not a live handle;
             */
            static bool ReadOptionalFontArg( lua_State *pLuaState, int nIndex,
                                             std :: shared_ptr<SunLight :: Font :: IFont> &pFont, std :: string &strError )  {

                pFont.reset();

                if( lua_isnoneornil( pLuaState, nIndex ) )
                    return true;

                if( lua_isinteger( pLuaState, nIndex ) )
                    pFont = GetFontPool( pLuaState ) -> Resolve( ( FontHandle ) lua_tointeger( pLuaState, nIndex ) );

                if( !pFont )  {
                    strError = "invalid font handle";
                    return false;
                }

                return true;
            }

            /**
             * @brief A required font-handle argument: a live handle from load_font.
             * @return The font, or nullptr (with strError set);
             */
            static std :: shared_ptr<SunLight :: Font :: IFont> ReadFontArg( lua_State *pLuaState, int nIndex, std :: string &strError )  {

                std :: shared_ptr<SunLight :: Font :: IFont>  pFont;

                if( lua_isinteger( pLuaState, nIndex ) )
                    pFont = GetFontPool( pLuaState ) -> Resolve( ( FontHandle ) lua_tointeger( pLuaState, nIndex ) );

                if( !pFont )
                    strError = "invalid font handle";

                return pFont;
            }


            /**
             * @brief A required, non-empty family name (register_font/unregister_font).
             * @return The name, or nullptr (with strError set);
             */
            static const char* ReadFamilyArg( lua_State *pLuaState, int nIndex, std :: string &strError )  {

                if( lua_type( pLuaState, nIndex ) != LUA_TSTRING || lua_rawlen( pLuaState, nIndex ) == 0 )  {
                    strError = "the family must be a non-empty string (as Tiled's fontfamily spells it)";
                    return nullptr;
                }

                return lua_tostring( pLuaState, nIndex );
            }

            /**
             * @luaname{load_font(path) -> font | nil, message}
             * @luagroup{font_handles}
             * @luaheading{Fonts}
             * @luadoc
             * Load a font file once and get a **font handle** for it (an opaque integer). The same
             * handle can then be the font `draw_text`/`measure_text` use (`set_font( font )`), be
             * registered for any number of the map's Tiled text-object families
             * (`register_font`), and be passed to `draw_text`/`measure_text` directly (their
             * optional last argument) — one file, one load. TrueType/OpenType (`.ttf`/`.otf`,
             * with Latin accents) and AngelCode BMFont (`.fnt`) are told apart by the extension.
             * Anchor the path on `APP_DIR` (see the BMFont note in `CLAUDE.md`).
             *
             * It needs a window, so the `renderer_create`-first rule applies (calling it first
             * creates the default renderer). A file that can't be loaded returns
             * `nil, "cannot load font '<path>'"`. `release_font` lets go of the handle when the
             * script no longer needs it; a font that is still active or registered keeps working.
             * @luaexample
             * local sans = assert( load_font( APP_DIR .. "resources/fonts/caravellius8x8.fnt" ) )
             * set_font( sans )                -- draw_text uses it ...
             * register_font( "Sans", sans )   -- ... and so do the map's "Sans" text objects
             */
            int LuaTextApi :: LoadFont( lua_State *pLuaState )  {

                if( lua_type( pLuaState, 1 ) != LUA_TSTRING )
                    return LuaEngineUtil :: PushError( pLuaState, "load_font expects a path" );

                const char                                    *szPath = lua_tostring( pLuaState, 1 );
                std :: shared_ptr<SunLight :: Font :: IFont>  pFont   = LuaEngineUtil :: GetDrawSurface( pLuaState ) -> LoadFont( szPath );

                if( !pFont )
                    return LuaEngineUtil :: PushError( pLuaState, std :: string( "cannot load font '" ) + szPath + "'" );

                FontHandle  handle = GetFontPool( pLuaState ) -> Add( std :: move( pFont ) );

                if( handle == INVALID_FONT_HANDLE )
                    return LuaEngineUtil :: PushError( pLuaState, "too many fonts loaded - release_font the ones no longer needed" );

                lua_pushinteger( pLuaState, ( lua_Integer ) handle );

                return 1;
            }

            /**
             * @luaname{release_font(font) -> true | nil, message}
             * @luagroup{font_handles}
             */
            int LuaTextApi :: ReleaseFont( lua_State *pLuaState )  {

                if( !lua_isinteger( pLuaState, 1 ) ||
                    !GetFontPool( pLuaState ) -> Release( ( FontHandle ) lua_tointeger( pLuaState, 1 ) ) )
                    return LuaEngineUtil :: PushError( pLuaState, "invalid font handle" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Select the font DrawText/MeasureText use: a path (load and select), a font
             * handle, or nil (the built-in font).
             *
             * @luaname{set_font(path | font | nil) -> true | nil, message}
             * @luadoc
             * Choose the font `draw_text`/`measure_text` use when they are not given one:
             * - a **font handle** from `load_font` — the same loaded font can also serve the map's
             *   text objects (`register_font`);
             * - a **path** — loads the file and selects it in one step, as before. That font
             *   belongs to the engine alone, and is freed when another font replaces it, so use
             *   `load_font` to share one font with text objects;
             * - **`nil`** — the engine's built-in font, which is also what is used until a first
             *   `set_font` succeeds.
             *
             * A failure leaves the active font as it was and returns `nil` and the reason:
             * `"cannot load font '<path>'"`, `"invalid font handle"`, or
             * `"set_font expects a path, a font or nil"`. It needs a window, like `load_font`.
             * @luaexample
             * set_font( APP_DIR .. "resources/fonts/pressstart2p-regular.ttf" )   -- load + select
             * local sans = load_font( APP_DIR .. "resources/fonts/caravellius8x8.fnt" )
             * set_font( sans )                                                    -- select a loaded font
             * set_font( nil )                                                     -- back to the built-in font
             */
            int LuaTextApi :: SetFont( lua_State *pLuaState )  {

                std :: string                                 strError;
                std :: shared_ptr<SunLight :: Font :: IFont>  pFont;
                SunLight :: DrawSurface :: IDrawSurface       *pSurface = LuaEngineUtil :: GetDrawSurface( pLuaState );

                if( lua_type( pLuaState, 1 ) == LUA_TSTRING )  {
                    const char  *szPath = lua_tostring( pLuaState, 1 );

                    pFont = pSurface -> LoadFont( szPath );

                    if( !pFont )
                        return LuaEngineUtil :: PushError( pLuaState, std :: string( "cannot load font '" ) + szPath + "'" );
                }
                else if( lua_isinteger( pLuaState, 1 ) )  {
                    pFont = ReadFontArg( pLuaState, 1, strError );

                    if( !pFont )
                        return LuaEngineUtil :: PushError( pLuaState, strError );
                }
                else if( !( lua_gettop( pLuaState ) >= 1 && lua_isnil( pLuaState, 1 ) ) )
                    return LuaEngineUtil :: PushError( pLuaState, "set_font expects a path, a font or nil" );

                // pFont is null here only for an explicit nil: the built-in font.
                if( !pSurface -> SetFont( pFont ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the engine refused the font (invalid, or loaded by another backend)" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{register_font(family, font, bold, italic) -> true | nil, message}
             * @luagroup{text_object_fonts}
             * @luaheading{Fonts for the map's text objects}
             * @luadoc
             * Make the map's **Tiled text objects** of one family and style draw with a loaded font.
             * A text object whose `fontfamily` (as Tiled spells it, eg. `"Sans"`) and bold/italic
             * flags match draws with `font`. `bold` and `italic` are optional booleans, default
             * `false`. A text object with no matching font draws `??????` instead (sunlight warns
             * once per family on stderr), so register every family a map uses.
             *
             * The font is shared, not copied: one loaded font can serve several families and
             * styles (eg. the regular file standing in for "Sans bold" too) and be the active font
             * at the same time. Registering the same family and style again replaces its font.
             * `unregister_font` undoes it. Errors return `nil` and the reason (a bad family, an
             * invalid font handle, a non-boolean flag).
             * @luaexample
             * local sans = assert( load_font( APP_DIR .. "resources/fonts/caravellius8x8.fnt" ) )
             * local bold = assert( load_font( APP_DIR .. "resources/fonts/caravellius8x8_bold.fnt" ) )
             * register_font( "Sans", sans )         -- "Sans" regular
             * register_font( "Sans", bold, true )   -- "Sans" bold
             */
            int LuaTextApi :: RegisterFont( lua_State *pLuaState )  {

                std :: string  strError;
                bool           bBold   = false;
                bool           bItalic = false;
                const char     *szFamily = ReadFamilyArg( pLuaState, 1, strError );

                if( szFamily == nullptr )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: Font :: IFont>  pFont = ReadFontArg( pLuaState, 2, strError );

                if( !pFont )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !LuaEngineUtil :: ReadOptionalBooleanArg( pLuaState, 3, "bold", false, bBold, strError ) ||
                    !LuaEngineUtil :: ReadOptionalBooleanArg( pLuaState, 4, "italic", false, bItalic, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !LuaEngineUtil :: GetRendererProvider( pLuaState ) -> RegisterFont( szFamily, pFont, bBold, bItalic, "register_font" ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the engine refused the font (invalid, or loaded by another backend)" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{unregister_font(font | family, bold, italic) -> true | nil, message}
             * @luagroup{text_object_fonts}
             * @luadoc
             * Undo `register_font`. Given a **font handle**, it removes that font from every family
             * and style it serves. Given a **family** (and optional `bold`/`italic`, default
             * `false`), it removes that one slot only. Text objects of a family left with no font
             * draw `??????` again. Returns `nil, "not registered"` if there was nothing to remove.
             * @luaexample
             * unregister_font( sans )                    -- every slot "sans" fills
             * unregister_font( "Sans", true, false )     -- only "Sans" bold
             */
            int LuaTextApi :: UnregisterFont( lua_State *pLuaState )  {

                std :: string                 strError;
                Engine :: IRendererProvider  *pProvider = LuaEngineUtil :: GetRendererProvider( pLuaState );
                bool                          bRemoved  = false;

                if( lua_type( pLuaState, 1 ) == LUA_TSTRING )  {
                    bool        bBold    = false;
                    bool        bItalic  = false;
                    const char  *szFamily = ReadFamilyArg( pLuaState, 1, strError );

                    if( szFamily == nullptr ||
                        !LuaEngineUtil :: ReadOptionalBooleanArg( pLuaState, 2, "bold", false, bBold, strError ) ||
                        !LuaEngineUtil :: ReadOptionalBooleanArg( pLuaState, 3, "italic", false, bItalic, strError ) )
                        return LuaEngineUtil :: PushError( pLuaState, strError );

                    bRemoved = pProvider -> UnregisterFont( szFamily, bBold, bItalic );
                }
                else  {
                    std :: shared_ptr<SunLight :: Font :: IFont>  pFont = ReadFontArg( pLuaState, 1, strError );

                    if( !pFont )
                        return LuaEngineUtil :: PushError( pLuaState, "unregister_font expects a font or a family" );

                    bRemoved = pProvider -> UnregisterFont( pFont );
                }

                if( !bRemoved )
                    return LuaEngineUtil :: PushError( pLuaState, "not registered" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Read draw_text/measure_text's text and size (arguments 1 and 2 of measure_text,
             * 1 and 4 of draw_text): the text is a string, or a number (converted the way Lua
             * converts one wherever it expects a string, so `draw_text( score, ... )` works), and
             * the size a whole number of pixels, at least 1.
             */
            static bool ReadTextArgs( lua_State *pLuaState, int nTextIndex, int nSizeIndex, std :: string &strText, int &nSize, std :: string &strError )  {

                lua_Integer  nValue = 0;

                if( lua_type( pLuaState, nTextIndex ) != LUA_TSTRING && lua_type( pLuaState, nTextIndex ) != LUA_TNUMBER )  {
                    strError = "argument 'text' must be a string";
                    return false;
                }

                // lua_tolstring converts a number argument in place; it is a copy of the caller's value
                size_t       nLength = 0;
                const char  *szText  = lua_tolstring( pLuaState, nTextIndex, &nLength );

                strText.assign( szText, nLength );

                if( !LuaEngineUtil :: ReadIntegerArgInRange( pLuaState, nSizeIndex, "size", 1, INT_MAX, nValue, strError ) )
                    return false;

                nSize = ( int ) nValue;

                return true;
            }

            /**
             * @brief Draw a line of text on screen, in screen space, with the active font (see
             * @link SetFont) or a given font handle.
             *
             * @luaname{draw_text(text, x, y, size, r, g, b, a, font) -> true | nil, message}
             * @luadoc
             * Draw a line of text on screen, in screen space — no
             * camera/viewport transform of its own, same as the FPS
             * counter. `r`/`g`/`b`/`a` are each `0`-`255`. It uses the
             * active font (`set_font`), or `font` — an optional last
             * argument, a handle from `load_font` — so a HUD can mix
             * several fonts without switching the active one. An invalid
             * `font` draws nothing and returns `nil, "invalid font handle"`;
             * so does any other wrong argument (`text` must be a string or a
             * number, `x`/`y` integers, `size` at least `1`). It returns
             * `true` when it draws.
             *
             * ```lua
             * draw_text( "Lives: 5", 20, 20, 24, 255, 255, 255, 255 )
             * draw_text( "Score", 20, 50, 24, 255, 220, 120, 255, titleFont )
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

                static const char * const  aszChannel[4] = { "r", "g", "b", "a" };

                std :: string                                 strError;
                std :: shared_ptr<SunLight :: Font :: IFont>  pFont;
                std :: string                                 strText;
                int                                           nSize        = 0;
                int                                           nPosX        = 0;
                int                                           nPosY        = 0;
                lua_Integer                                   anChannel[4] = { 0, 0, 0, 0 };

                if( !ReadTextArgs( pLuaState, 1, 4, strText, nSize, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "x", nPosX, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 3, "y", nPosY, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                for( int nArg = 0; nArg < 4; nArg++ )
                    if( !LuaEngineUtil :: ReadIntegerArgInRange( pLuaState, 5 + nArg, aszChannel[nArg], 0, UCHAR_MAX, anChannel[nArg], strError ) )
                        return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !ReadOptionalFontArg( pLuaState, 9, pFont, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                unsigned char  nRed   = ( unsigned char ) anChannel[0];
                unsigned char  nGreen = ( unsigned char ) anChannel[1];
                unsigned char  nBlue  = ( unsigned char ) anChannel[2];
                unsigned char  nAlpha = ( unsigned char ) anChannel[3];

                if( pFont )
                    pFont -> DrawText( strText.c_str(), nPosX, nPosY, nSize, SunLight :: Base :: stColor { nRed, nGreen, nBlue, nAlpha } );
                else
                    LuaEngineUtil :: GetDrawSurface( pLuaState ) -> DrawText( strText.c_str(), nPosX, nPosY, nSize,
                                                                           nRed, nGreen, nBlue, nAlpha );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Measure how wide a line of text would render, in
             * pixels, at a given font size - meant for HUD layout (e.g.
             * right-aligning a score/lives readout against @link
             * GetWindowWidth), with the active font or a given font handle.
             *
             * @luaname{measure_text(text, size, font) -> width | nil, message}
             * @luadoc
             * Measure how wide a line of text would render, in pixels, at
             * a given font size — with the font `draw_text` would use:
             * the active font, or `font`, an optional last argument (a
             * handle from `load_font`). Meant for HUD layout (e.g.
             * right-aligning a score/lives readout against
             * `screen_get_width()`) without hardcoding assumed widths,
             * since digit/character count can change over a playthrough.
             * An invalid `font` returns `nil, "invalid font handle"`, and a
             * wrong `text`/`size` is `nil, "message"` too.
             * @luaexample
             * local text  = "Lives: " .. tostring( Lives.count )
             * local width = measure_text( text, 24 )
             * local x     = screen_get_width() - width - 20  -- flush to the right edge, with a 20px margin
             */
            int LuaTextApi :: MeasureText( lua_State *pLuaState )  {

                std :: string                                 strError;
                std :: shared_ptr<SunLight :: Font :: IFont>  pFont;
                std :: string                                 strText;
                int                                           nSize = 0;

                if( !ReadTextArgs( pLuaState, 1, 2, strText, nSize, strError ) || !ReadOptionalFontArg( pLuaState, 3, pFont, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                int  nWidth = pFont ? pFont -> MeasureText( strText.c_str(), nSize )
                                    : LuaEngineUtil :: GetDrawSurface( pLuaState ) -> MeasureText( strText.c_str(), nSize );

                lua_pushinteger( pLuaState, nWidth );

                return 1;
            }

            /**
             * @brief Register the text/font Lua-callable functions.
             */
            void LuaTextApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "load_font", LuaTextApi :: LoadFont );
                lua_register( pLuaState, "release_font", LuaTextApi :: ReleaseFont );
                lua_register( pLuaState, "set_font", LuaTextApi :: SetFont );
                lua_register( pLuaState, "register_font", LuaTextApi :: RegisterFont );
                lua_register( pLuaState, "unregister_font", LuaTextApi :: UnregisterFont );
                lua_register( pLuaState, "draw_text", LuaTextApi :: DrawText );
                lua_register( pLuaState, "measure_text", LuaTextApi :: MeasureText );
            }
        }
    }
}
