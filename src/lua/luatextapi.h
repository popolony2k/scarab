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

#ifndef __LUATEXTAPI_H__
#define __LUATEXTAPI_H__

extern "C"
{
  #include "lua.h"
}

#include "engine/fontpool.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Fonts and text Lua primitives (doc/MASTER_PLAN.md item C6, over sunlight I3
             * "fonts by handle"): font handles (load_font/release_font, kept in the engine's
             * FontPool), the active font (set_font -> IDrawSurface::SetFont), fonts for the map's
             * Tiled text objects (register_font/unregister_font -> TileMapRenderer::RegisterFont/
             * UnregisterFont, through IRendererProvider), and screen-space text
             * (draw_text/measure_text, with the active font or a given IFont). All game-side HUD
             * logic (what to draw, when, positioning, formatting) lives in Lua on top of these.
             * The screen size (screen_get_width/height) is in LuaAppApi.
             *
             * @luacategory{Text}
             * @luadoc
             * Fonts and screen-space text. A font is **loaded once** (`load_font`) into a handle,
             * and that one handle can be the font `draw_text`/`measure_text` use (`set_font`),
             * serve the map's Tiled text objects (`register_font`), and be passed to
             * `draw_text`/`measure_text` directly — one file, one load. `set_font` also still
             * takes a path, for a font that only `draw_text` needs. All HUD logic (what to draw,
             * when, positioning, formatting) lives in game-side Lua on top of these; to lay text out
             * against the screen's edges, use `screen_get_width`/`screen_get_height` (App page).
             * Every function here reports a failure as `nil, "message"` — it never raises a Lua
             * error.
             */
            class LuaTextApi  {

                static int LoadFont( lua_State *pLuaState );
                static int ReleaseFont( lua_State *pLuaState );
                static int SetFont( lua_State *pLuaState );
                static int RegisterFont( lua_State *pLuaState );
                static int UnregisterFont( lua_State *pLuaState );
                static int DrawText( lua_State *pLuaState );
                static int MeasureText( lua_State *pLuaState );

                public:

                // The Lua global LuaEngine::Init stashes the engine's FontPool in (a light userdata).
                static constexpr const char  *FONT_POOL_GLOBAL = "fontPoolPtr";

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUATEXTAPI_H__ */
