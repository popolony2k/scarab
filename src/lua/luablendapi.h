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

#ifndef __LUABLENDAPI_H__
#define __LUABLENDAPI_H__

extern "C"
{
  #include "lua.h"
}

#include <string>
#include "base/blendmode.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Blend-mode Lua primitives (doc/MASTER_PLAN.md item B1) - the BLEND_MODE_*
             * constants and the scoped mode around a script's own drawing, a thin wrapper over
             * IDrawSurface::BeginBlendMode/EndBlendMode (sunlight's I2). A sprite's own mode
             * lives with the sprite functions (LuaSpriteApi), which validate the mode here.
             *
             * @luacategory{Blend modes}
             * @luadoc
             * How what is drawn next combines with what is already drawn: lights and glows
             * (`BLEND_MODE_ADD`), shadows and tints (`BLEND_MODE_MULTIPLY`), darkening by a
             * colour (`BLEND_MODE_SUBTRACT`), or the default (`BLEND_MODE_NORMAL`). Each mode is
             * a formula on the colour, for a source colour `s` with alpha `a` drawn over what is
             * already there, `d`:
             *
             * | mode | formula | for |
             * |---|---|---|
             * | `BLEND_MODE_NORMAL` | `s·a + d·(1−a)` | the default |
             * | `BLEND_MODE_ADD` | `d + s·a` | lights, glows |
             * | `BLEND_MODE_MULTIPLY` | `d·(s·a + 1−a)` — a transparent pixel leaves `d` as is | shadows, tints |
             * | `BLEND_MODE_SUBTRACT` | `d − s·a` | darkening by a colour |
             *
             * In every mode an opaque pixel stays opaque. There are two ways to use a mode:
             * around what a script draws itself (`draw_begin_blend_mode`/`draw_end_blend_mode`,
             * on this page), and per sprite (`sprite_set_blend_mode`, on the sprite page).
             * Every function here reports a failure as `nil, "message"` — it never raises a
             * Lua error. **Games that use no mode are unchanged.**
             */
            class LuaBlendApi  {

                static int BeginBlendMode( lua_State *pLuaState );
                static int EndBlendMode( lua_State *pLuaState );

                static void RegisterEnums( lua_State *pLuaState );

                public:

                static bool ReadBlendMode( lua_State *pLuaState, int nIndex, SunLight :: Base :: BlendMode &mode, std :: string &strError );
                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUABLENDAPI_H__ */
